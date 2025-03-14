// -*- c-basic-offset: 4; indent-tabs-mode: nil -*- 
#include "constant_cca_erasure.h"
#include <iostream>
#include <math.h>

////////////////////////////////////////////////////////////////
//  PACER
////////////////////////////////////////////////////////////////

// TODO NOW: our CCA will not switch modes, so we can get rid of things relevant to that
ConstantErasureCcaPacer::ConstantErasureCcaPacer(ConstantErasureCcaSrc& src, EventList& event_list, simtime_picosec interpacket_delay)
    : EventSource(event_list,"constant_cca_pacer"), _src(&src), _interpacket_delay(interpacket_delay) {
    _last_send = 0;
    _next_send = 0;
}

void
ConstantErasureCcaPacer::schedule_send(simtime_picosec delay) {
    // cout << _src->nodename() << " schedule_send " << timeAsUs(eventlist().now()) << endl;
    // cout << "Current next send: " << timeAsUs(_next_send) << " delay " << timeAsUs(delay) << endl;
    _interpacket_delay = delay;
    simtime_picosec previous_next_send = _next_send;
    simtime_picosec new_next_send = _last_send + _interpacket_delay + rand() % (_interpacket_delay/10);
    if (new_next_send <= eventlist().now()) {
        // Tricky!  We're going in to pacing mode, but it's more than
        // the pacing delay since we last sent.  Presumably the best
        // thing is to immediately send, and then pacing will kick in
        // next time round.
        _next_send = eventlist().now();
        doNextEvent();
        // cout << "Setting next send to now" << endl;
        return;
    }
    if (previous_next_send > eventlist().now()) {
        // cout << "Avoiding overwriting next send" << endl;
        return;
         // don't overwrite next_send if it's already in the future
    }
    // cout << "Setting next send to " << timeAsUs(new_next_send) << endl;
    _next_send = new_next_send;
    eventlist().sourceIsPending(*this, _next_send);
}

bool 
ConstantErasureCcaPacer::allow_send() {
    simtime_picosec now = eventlist().now();
    if (_last_send == 0 && _src->_packets_sent == 0) {  
        // first packet, so we can send
        return true;
    }
    return now >= _interpacket_delay + _last_send; 
}

void
ConstantErasureCcaPacer::cancel() {
    _interpacket_delay = 0;
    _next_send = 0;
    eventlist().cancelPendingSource(*this);
}

// called when we're in window-mode to update the send time so it's always correct if we
// then go into paced mode
void
ConstantErasureCcaPacer::just_sent() {
    // cout<< _src->nodename() << " just_sent " << timeAsUs(eventlist().now()) << endl;
    _last_send = eventlist().now();
}

void
ConstantErasureCcaPacer::doNextEvent() {
    // cout << _src->nodename() << " doNextEvent " << timeAsUs(eventlist().now()) << "(next send is " << timeAsUs(_next_send) << ")" << endl;
    assert(eventlist().now() == _next_send);
    // _src->send_next_packet();
    _src->send_packets();
    _last_send = eventlist().now();
    //cout << "sending paced packet" << endl;

    if (_src->pacing_delay() > 0) {
        // schedule the next packet send
        // cout << _src->nodename() << " calling schedule send 1" << endl;
        schedule_send(_src->pacing_delay());
        //cout << "rescheduling send " << timeAsUs(_src->_pacing_delay) << "us" << endl;
    } else {
        // the src is no longer in pacing mode, but we didn't get a
        // cancel.  A bit odd, but drop out of pacing.  Should we have
        // sent here?  Could add the check before sending?
        _interpacket_delay = 0;
        _next_send = 0;
    }
}

////////////////////////////////////////////////////////////////
//  CONSTANT CCA SRC
////////////////////////////////////////////////////////////////

ConstantErasureCcaSrc::ConstantErasureCcaSrc(EventList &eventlist, uint32_t addr, simtime_picosec pacing_delay, TrafficLogger* pkt_logger)
    : EventSource(eventlist, "constant_erasure_cca_src"), _pacer(*this, eventlist, pacing_delay), _flow(pkt_logger)
{
    _addr = addr;
    _packets_sent = 0;
    _current_seqno = 0;
    _bytes_acked = 0;
    _established = false;

    _pacing_delay = pacing_delay;  // 
    _deferred_send = false; // true if we wanted to send, but no scheduler said no.

    // PLB init
    _plb_interval = timeFromSec(1);  // disable PLB til we've seen some traffic
    _path_index = 0;  
    _last_good_path = eventlist.now();
    

    _mss = Packet::data_packet_size();
    _scheduler = NULL;
    _flow_size = ((uint64_t)1)<<63;
    _stop_time = 0;
    _stopped = false;
    _app_limited = -1;
    _completion_time = 0;

    // PLB init
    _plb = false; // enable using enable_plb()
    _spraying = false;

    _nodename = "consterasurecca_src_" + std::to_string(get_id());
}

int 
ConstantErasureCcaSrc::send_packets() {
    int sent_count = 0;
    if(is_complete()) {
        if (_completion_time == 0) {
            _completion_time = eventlist().now();
        }
        return 0;
    } else { 
        if (_pacer.allow_send()) {
            bool sent = send_next_packet();
            if (sent) {
                sent_count++;
            }
        }
    }
    return sent_count;
}

bool
ConstantErasureCcaSrc::send_next_packet() {
    // ask the scheduler if we can send now
    if (queuesize(_flow.flow_id()) > 2) {
        // no, we can't send.  We'll be called back when we can.
        _deferred_send = true;
        return false;
    }
    _deferred_send = false;

    ConstantCcaPacket* p = ConstantCcaPacket::newpkt(_flow, *_route, _current_seqno, mss(), _destination, _addr, _pathid);
    _packets_sent++;  
    _current_seqno += mss();
    
    p->set_ts(eventlist().now());
    p->sendOn();
    _pacer.just_sent();

    if (spraying()) { // switch path on every packet
        move_path_flow_label();
    }

    return true;
}

void
ConstantErasureCcaSrc::send_callback() {
    if (_deferred_send == false) {
        // no need to be here
        return;
    }

    _deferred_send = false;
    // We had previously wanted to send but queue said no.  Now it says yes.
    send_packets();
}


void
ConstantErasureCcaSrc::receivePacket(Packet& pkt) 
{
    ConstantCcaAck *p = (ConstantCcaAck*)(&pkt);
    simtime_picosec ts_echo = p->ts_echo();
    simtime_picosec rtt = eventlist().now() - ts_echo;
    _bytes_acked = p->ackno();
    p->free();

    // if ((_current_seqno >= _flow_size) && (!is_complete())) {
    //     // now only send packets on receiving an ACK
    //     if (_pacer.allow_send()) {
    //         send_next_packet();
    //     } else {
    //         _pacer.schedule_send(_pacing_delay);
    //         _deferred_send = true;
    //         return;
    //     }
    // }

    if (plb()) {
        simtime_picosec now = eventlist().now();
        _ecn_marks.emplace_back(pkt.flags() & ECN_CE);
        if (_ecn_marks.size() > 10) {
            _ecn_marks.pop_front();
        }
        int total_marks = 0;
        for (auto& m : _ecn_marks) {
            total_marks += m;
        }
        if (total_marks < _plb_threshold_ecn) {
            // not enough marks to be a problem
            _last_good_path = now;
            _plb_interval = random()%(2*rtt) + 5*rtt;
        }
        // simtime_picosec td = _min_rtt * 1.2; // TODO: Find a good threshold for "congestion"
        // if (delay <= td) {
        //     // good delay!
        //     _last_good_path = now;
        //     _plb_interval = random()%(2*_rtt) + 5*_rtt;
        // }

        // PLB (simple version)
        if (now - _last_good_path > _plb_interval) {
            cout << "moving " << timeAsUs(now) << " last_good " << timeAsUs(_last_good_path) << " RTT " << timeAsUs(rtt) << endl;
            _last_good_path = now;
            move_path_flow_label();
        }
    }
}


void ConstantErasureCcaSrc::doNextEvent() {
    if (!_established) {
        //cout << "Establishing connection" << endl;
        startflow();
        return;
    }
}

void 
ConstantErasureCcaSrc::connect(ConstantErasureCcaSink& sink, simtime_picosec startTime, uint32_t destination, const Route& routeout, const Route& routein) {
    _sink = &sink;
    _destination = destination;
    Route* new_route = routeout.clone(); // make a copy, as we may be switching routes and don't want to append the sink more than once
    new_route->push_back(_sink);
    _route = new_route;
    _flow.set_id(get_id()); // identify the packet flow with the source that generated it
    cout << "connect, flow id is now " << _flow.get_id() << endl;
    // cout << "connect, flow id (flow_id()) is now " << _flow.flow_id() << endl;
    _scheduler = dynamic_cast<ConstBaseScheduler*>(routeout.at(0));
    _scheduler->add_src(_flow.flow_id(), this);
    _sink->connect(*this, routein);
    eventlist().sourceIsPending(*this,startTime);
    _start_time = startTime;
}

void
ConstantErasureCcaSrc::move_path_flow_label() {
    // cout << timeAsUs(eventlist().now()) << " " << nodename() << " td move_path\n";
    _pathid++;
}

void
ConstantErasureCcaSrc::move_path(bool permit_cycles) {
    cout << timeAsUs(eventlist().now()) << " " << nodename() << " td move_path\n";
    if (_paths.size() == 0) {
        cout << nodename() << " cant move_path\n";
        return;
    }
    _path_index++;
    if (!permit_cycles) {
        // if we've moved paths so often we've run out of paths, I want to know
        assert(_path_index < _paths.size()); 
    } else if (_path_index >= _paths.size()) {
        _path_index = _path_index % _paths.size();
    }
    Route* new_route = _paths[_path_index]->clone();
    new_route->push_back(_sink);
    _route = new_route;
}

void
ConstantErasureCcaSrc::reroute(const Route &routeout) {
    Route* new_route = routeout.clone();
    new_route->push_back(_sink);
    _route = new_route;
}

int
ConstantErasureCcaSrc::queuesize(int flow_id) {
    return _scheduler->src_queuesize(flow_id);
}

bool
ConstantErasureCcaSrc::check_stoptime() {
    if (_stop_time && eventlist().now() >= _stop_time) {
        _stopped = true;
        _stop_time = 0;
    }
    if (_stopped) {
        return true;
    }
    return false;
}

void ConstantErasureCcaSrc::set_app_limit(int pktps) {
    _app_limited = pktps;
    send_packets();
}

void
ConstantErasureCcaSrc::set_paths(vector<const Route*>* rt_list){
    size_t no_of_paths = rt_list->size();
    _paths.resize(no_of_paths);
    for (size_t i=0; i < no_of_paths; i++){
        Route* rt_tmp = new Route(*(rt_list->at(i)));
        if (!_scheduler) {
            _scheduler = dynamic_cast<ConstBaseScheduler*>(rt_tmp->at(0));
            assert(_scheduler);
        } else {
            // sanity check all paths share the same scheduler.  If we ever want to use multiple NICs, this will need fixing
            assert(_scheduler == dynamic_cast<ConstBaseScheduler*>(rt_tmp->at(0)));
        }
        rt_tmp->set_path_id(i, rt_list->size());
        _paths[i] = rt_tmp;
    }
    permute_paths();
    _path_index = 0;
}

void
ConstantErasureCcaSrc::permute_paths() {
    // Fisher-Yates shuffle
    size_t len = _paths.size();
    for (size_t i = 0; i < len; i++) {
        size_t ix = random() % (len - i);
        const Route* tmppath = _paths[ix];
        _paths[ix] = _paths[len-1-i];
        _paths[len-1-i] = tmppath;
    }
}

void 
ConstantErasureCcaSrc::startflow() {
    _established = true; // send data from the start
    send_packets();
    _pacer.schedule_send(_pacing_delay);
}

bool ConstantErasureCcaSrc::is_complete() {
    return _bytes_acked >= flow_size();
}


////////////////////////////////////////////////////////////////
//  SINK
////////////////////////////////////////////////////////////////

ConstantErasureCcaSink::ConstantErasureCcaSink() 
    : DataReceiver("ConstantErasureCcaSink")
{
    _bytes_received = 0;
    _src = NULL;
    _nodename = "constanterasureccasink";
}

void
ConstantErasureCcaSink::connect(ConstantErasureCcaSrc& src, const Route& route) {
    _src = &src;
    Route *rt = route.clone();
    rt->push_back(&src);
    _route = rt;
}

void
ConstantErasureCcaSink::receivePacket(Packet& pkt) {
    ConstantCcaPacket *p = (ConstantCcaPacket*)(&pkt);
    _bytes_received += p->size();

    // if (_bytes_received >= _src->flow_size()) {
        ConstantCcaAck* newAck = ConstantCcaAck::newpkt(p->flow(), *_route, 
                                    0, _bytes_received,
                                    p->ts(), p->src(), p->dst(), p->pathid());

        newAck->sendOn();
    // }
    p->free();
}
