// -*- c-basic-offset: 4; indent-tabs-mode: nil -*- 
#include "constant_cca_erasure_drb.h"
#include <iostream>
#include <math.h>
#include <vector>
#include <algorithm>

////////////////////////////////////////////////////////////////
//  PACER
////////////////////////////////////////////////////////////////

// TODO NOW: our CCA will not switch modes, so we can get rid of things relevant to that
ConstantErasureCcaPacerDRB::ConstantErasureCcaPacerDRB(ConstantErasureCcaSrcDRB& src, EventList& event_list, simtime_picosec interpacket_delay)
    : EventSource(event_list,"constant_cca_pacer"), _src(&src), _interpacket_delay(interpacket_delay) {
    _last_send = 0;
    _next_send = 0;
}

void
ConstantErasureCcaPacerDRB::schedule_send(simtime_picosec delay) {
    // cout << _src->nodename() << " schedule_send " << timeAsUs(eventlist().now()) << endl;
    // cout << "Current next send: " << timeAsUs(_next_send) << " delay " << timeAsUs(delay) << endl;
    _interpacket_delay = delay;
    simtime_picosec previous_next_send = _next_send;
    // simtime_picosec new_next_send = _last_send + _interpacket_delay + rand() % (_interpacket_delay/10);
    simtime_picosec jitter = rand() % 1 == 0 ? -1 * rand() % (_interpacket_delay/100) : rand() % (_interpacket_delay/100);
    simtime_picosec new_next_send = _last_send + _interpacket_delay + jitter;
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
ConstantErasureCcaPacerDRB::allow_send() {
    simtime_picosec now = eventlist().now();
    if (_last_send == 0 && _src->_packets_sent == 0) {  
        // first packet, so we can send
        return true;
    }
    return now >= _interpacket_delay + _last_send; 
}

void
ConstantErasureCcaPacerDRB::cancel() {
    _interpacket_delay = 0;
    _next_send = 0;
    eventlist().cancelPendingSource(*this);
}

// called when we're in window-mode to update the send time so it's always correct if we
// then go into paced mode
void
ConstantErasureCcaPacerDRB::just_sent() {
    // cout<< _src->nodename() << " just_sent " << timeAsUs(eventlist().now()) << endl;
    _last_send = eventlist().now();
}

void
ConstantErasureCcaPacerDRB::doNextEvent() {
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

ConstantErasureCcaSrcDRB::ConstantErasureCcaSrcDRB(EventList &eventlist, uint32_t addr, simtime_picosec pacing_delay, TrafficLogger* pkt_logger, FatTreeTopologyDRB* top)
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

    _adaptive= false;
    // _ev_count = 32;
    // _path_qualities.assign(_ev_count, 0.0);
    // _path_skipped.assign(_ev_count, false);
    _pathid = 0;
    _ev_timers = {};
    _uec_mp = new UecMpReps(16000, false, true);
    

    _mss = Packet::data_packet_size();
    _scheduler = NULL;
    _flow_size = ((uint64_t)1)<<63;
    _stop_time = 0;
    _stopped = false;
    _app_limited = -1;
    _completion_time = 0;
    _inflight = 0;
    _approx_lost = 0;
    _highest_ack = 0;
    _final_bdp_time = 0;

    _total_pkt_delay = 0;
    _total_packets = 0;

    _plb = false; // enable using enable_plb()
    _spraying = false;
    _drb = false; // Initialize DRB flag
    _drb_state_initialized = false;

    _timer_start = timeInf;
    _min_rto = timeFromUs((uint32_t)2);
    _rtt = 0;
    _rto = timeFromMs(1);
    _mdev = 0;

    _nodename = "consterasurecca_src_" + std::to_string(get_id());

    _top = top; // Initialize FatTreeTopologyDRB pointer
}

void ConstantErasureCcaSrcDRB::set_drb() {
    _drb = true;
}

const Route* ConstantErasureCcaSrcDRB::get_drb_route(uint32_t destination_id, bool is_ack) {
    // Initialize pointers for this destination if not already done
    if (!_drb_state_initialized) {
        // Get a sample route to determine its characteristics
        // We use pointer 0 temporarily to get the route structure

        // Determine the modulo value based on source/destination location in topology
        // In a fat-tree, routing depends on whether src and dest are in same/different pods and ToRs
        uint32_t data_modulo = 1;
        uint32_t ack_modulo = 1;

        // Get topology parameters
        uint32_t hosts_per_tor = _top->tor_switches_per_pod();  // K/2
        uint32_t hosts_per_pod = hosts_per_tor * _top->tor_switches_per_pod();  // (K/2) * (K/2) = K^2/4

        // Determine which pod and ToR each host belongs to
        uint32_t src_pod = _addr / hosts_per_pod;
        uint32_t src_tor_in_pod = (_addr % hosts_per_pod) / hosts_per_tor;

        uint32_t dst_pod = destination_id / hosts_per_pod;
        uint32_t dst_tor_in_pod = (destination_id % hosts_per_pod) / hosts_per_tor;

        if (src_pod == dst_pod) {
            if (src_tor_in_pod == dst_tor_in_pod) {
                // Same ToR: 3-hop route (SRC->ToR->DST), no load balancing needed
                data_modulo = 1;
                ack_modulo = 1;
            } else {
                // Same pod, different ToR: 5-hop route, balance across aggregation switches per pod
                data_modulo = _top->get_drb_path_count(_addr, destination_id);
                ack_modulo = _top->get_drb_path_count(destination_id, _addr); // Reverse direction for ACK
            }
        } else {
            // Different pods: 7-hop route, balance across core switches
            data_modulo = _top->get_drb_path_count(_addr, destination_id);
            ack_modulo = _top->get_drb_path_count(destination_id, _addr); // Reverse direction for ACK
        }

        // Initialize with random values within the appropriate ranges
        uint32_t random_data_ptr = rand() % (data_modulo > 0 ? data_modulo : 1);
        uint32_t random_ack_ptr = rand() % (ack_modulo > 0 ? ack_modulo : 1);

        // Ensure they are different if possible
        if (ack_modulo > 1 && random_data_ptr == random_ack_ptr) {
            uint32_t offset = 1 + (rand() % (ack_modulo - 1));
            random_ack_ptr = (random_ack_ptr + offset) % ack_modulo;
        }

        _drb_state = {
            random_data_ptr,
            random_ack_ptr,
            0,  // data_count (starts at 0, will be incremented per packet)
            0,  // ack_count (starts at 0, will be incremented per packet)
            data_modulo,
            ack_modulo
        };
        _drb_state_initialized = true;
        _drb_data_routes.resize(data_modulo, nullptr);
        _drb_ack_routes.resize(ack_modulo, nullptr);
    }

    // Get current pointer value based on packet count and modulo
    DrbPointers& drb_ptr = _drb_state;
    uint32_t modulo = is_ack ? drb_ptr.ack_modulo : drb_ptr.data_modulo;
    uint32_t count = is_ack ? drb_ptr.ack_count : drb_ptr.data_count;

    // Calculate the actual pointer value: initial + offset (mod modulo)
    uint32_t current_rr_index = (is_ack ? drb_ptr.initial_ack_ptr : drb_ptr.initial_data_ptr);
    if (modulo > 1) {
        current_rr_index = (current_rr_index + count) % modulo;
    }

    // Call topology to get the DRB path for this pointer value
    const Route* route = _top->get_drb_selected_path(_addr, destination_id, current_rr_index, is_ack);

    return route;
}

int 
ConstantErasureCcaSrcDRB::send_packets() {
    int sent_count = 0;
    if(is_complete()) {
        if (_completion_time == 0) {
            _completion_time = eventlist().now();
            _pacer.cancel();
        }
        return 0;
    } else if (_current_seqno - _approx_lost >= _flow_size) { // final bdp (inflight+bytes_acked) (_current_seqno - _approx_lost >= _flow_size) 
        if (_final_bdp_time == 0) {
            _final_bdp_time = eventlist().now();
            // _lost_offset = _current_seqno - _approx_lost;
            return 0;
        }
         if (!_assume_lossless && _final_bdp_time != 0 && eventlist().now() - _final_bdp_time > _rtt) { // timeout on final bdp. send again (will not stop again until final ack received - TODO make it wait again)
            // std::cout << "Allow send  " << _pacer.allow_send() << std::endl;
            if (_pacer.allow_send()) {
                bool sent = send_next_packet();
                if (sent) {
                    sent_count++;
                }
            }
        }
        return 0;
    } else {
        // This is a hacky placement for now that works because the pacer will 
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
ConstantErasureCcaSrcDRB::send_next_packet() {
    // ask the scheduler if we can send now
    if (queuesize(_flow.flow_id()) > 2) {
        // no, we can't send.  We'll be called back when we can.
        _deferred_send = true;
        return false;
    }
    _deferred_send = false;

    if (adaptive()) {
        // _pathid++;
        _pathid = _uec_mp->nextEntropy(_current_seqno, 0); // current_cwnd is unused
        while (_ev_timers.find(_pathid) != _ev_timers.end() && eventlist().now() - _ev_timers[_pathid] > _rto) {
            // simtime_picosec time_since_send = eventlist().now() - _ev_timers[_pathid];
            // std::cout << "Timeout discovered at " << eventlist().now() << " with rto " << _rto  << "(time since send: " << time_since_send << ") on ev " << _pathid << " on flow " << _addr << " -> " << _destination <<  std::endl;
            _uec_mp->processEv(_pathid, UecMultipath::PATH_TIMEOUT);
            _ev_timers.erase(_pathid);
            // uint32_t oldpathid = _pathid;
            _pathid = _uec_mp->nextEntropy(_current_seqno, 0);
        }
        // std::cout << "New EV " << _pathid << std::endl;
    }

    // Calculate DRB pointer for this packet if DRB is enabled
    uint32_t drb_ptr_value = 0;
    if (_drb) {
        DrbPointers& drb_ptr = _drb_state;
        uint32_t data_modulo = drb_ptr.data_modulo;
        drb_ptr_value = drb_ptr.initial_data_ptr;
        if (data_modulo > 1) {
            drb_ptr_value = (drb_ptr_value + drb_ptr.data_count) % data_modulo;
        }

        // Update route for this packet based on its pointer value
        // Check cache first
        if (_drb_data_routes[drb_ptr_value] == nullptr) {
            const Route* drb_route = _top->get_drb_selected_path(_addr, _destination, drb_ptr_value, false);
            Route* new_route = drb_route->clone();
            new_route->push_back(_sink);
            _drb_data_routes[drb_ptr_value] = new_route;
        }
        _route = _drb_data_routes[drb_ptr_value];
        
        // Increment data packet count for this destination
        drb_ptr.data_count++;
    }


    ConstantCcaPacket* p = ConstantCcaPacket::newpkt(_flow, *_route, _current_seqno, 0, mss(), _destination, _addr, _pathid);
    _packets_sent++;  
    _current_seqno += mss();
    
    p->set_ts(eventlist().now());
    p->sendOn();
    _pacer.just_sent();


    if (spraying()) { // switch path on every packet
        move_path_flow_label();
    }

    if (plb() && _timer_start == timeInf) {
        _timer_start = eventlist().now();
    } 
    else if (plb() && eventlist().now() - _timer_start > _rto) { // If no packets have gotten through, reroute. NOTE: may have weird effects if already rerouted on _plb_interval stuff
        _last_good_path = eventlist().now();
        move_path_flow_label();
        _timer_start = eventlist().now();
    } 
    
    if (adaptive() && _ev_timers.find(_pathid) == _ev_timers.end()) {
        _ev_timers[_pathid] = eventlist().now();    
    }

    return true;
}

void 
ConstantErasureCcaSrcDRB::check_ev_timers(simtime_picosec now) {
    for (std::map<uint32_t, simtime_picosec>::iterator it = _ev_timers.begin(); it != _ev_timers.end(); ++it) {
        if (now - it->second > _rto) {
            _uec_mp->processEv(it->first, UecMultipath::PATH_TIMEOUT);
        }
    }
}

void
ConstantErasureCcaSrcDRB::send_callback() {
    if (_deferred_send == false) {
        // no need to be here
        return;
    }

    _deferred_send = false;
    // We had previously wanted to send but queue said no.  Now it says yes.
    send_packets();
}

void
ConstantErasureCcaSrcDRB::update_rtt(simtime_picosec delay) {
    // calculate TCP-like RTO.  Not clear this is right for Swift
    if (delay!=0){
        if (_rtt>0){
            uint64_t abs;
            if (delay > _rtt)
                abs = delay - _rtt;
            else
                abs = _rtt - delay;

            _mdev = 3 * _mdev / 4 + abs/4;
            _rtt = 7*_rtt/8 + delay/8;
            _rto = _rtt + 4*_mdev;
        } else {
            _rtt = delay;
            _mdev = delay/2;
            _rto = _rtt + 4*_mdev;
        }
    }

    if (_rto <_min_rto)
        _rto = _min_rto;
}


void
ConstantErasureCcaSrcDRB::receivePacket(Packet& pkt) 
{
    ConstantCcaAck *p = (ConstantCcaAck*)(&pkt);
    simtime_picosec ts_echo = p->ts_echo();
    simtime_picosec rtt = eventlist().now() - ts_echo;
    update_rtt(rtt);
    uint32_t pathid = p->pathid();
    // _bytes_acked = p->ds_ackno();
    _bytes_acked = max(p->ds_ackno(), _bytes_acked); //if acks get reordered, this might not always be the highest
    ConstantCcaPacket::seq_t seqno = p->ackno(); // echoes the sequence number this is an ack for 
    _highest_ack = max((uint64_t)seqno, _highest_ack);
    _total_packets++;
    _total_pkt_delay += rtt;
    p->free();

    _inflight = _current_seqno - _highest_ack; // This is an approximation (may be more due to reordering)
    _approx_lost = (_highest_ack - 1) - _bytes_acked;
    // If have sent everything, need to check that the num lost is not increasing

    _timer_start = timeInf; // reset timer on receiving an ACK TODO: change to only update if ACK number has increased? not sure if that makes sense in this setup
    // Potential change: per-EV timers
    if (adaptive()) {
        _ev_timers.erase(pathid);
    }

    if (is_complete() && _completion_time == 0) {
        _completion_time = eventlist().now();
        _pacer.cancel();
    }

    // if (((_flow_size - _bytes_acked)/mss() >= (uint64_t)(rtt / _pacing_delay)) && (!is_complete())) {
    //     // Sent full flow size, but not sure if the receiver will need more
    //     _final_bdp = true;
    //     _final_bdp_time = eventlist().now();
    //     // Only send more if we are not done in an rtt 

    // }



    // if ((_current_seqno >= _flow_size) && (!is_complete())) {
    //     // now only send packets on receiving an ACK
    //     if (_pacer.allow_send()) {
    //         send_next_packet();
    //     } else {
    //         _send_allowed = true;
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
            _plb_interval = random()%(2*_pacing_delay) + 10*_pacing_delay;
        }
        // simtime_picosec td = _min_rtt * 1.2; // TODO: Find a good threshold for "congestion"
        // if (delay <= td) {
        //     // good delay!
        //     _last_good_path = now;
        //     _plb_interval = random()%(2*_rtt) + 5*_rtt;
        // }

        // PLB (simple version)
        if (now - _last_good_path > _plb_interval) {
            // cout << "moving " << timeAsUs(now) << " last_good " << timeAsUs(_last_good_path) << " RTT " << timeAsUs(rtt) << endl;
            _last_good_path = now;
            move_path_flow_label();
        }
    }
    if (adaptive()) {
        UecMultipath::PathFeedback feedback = pkt.flags() & ECN_CE ? UecMultipath::PATH_ECN : UecMultipath::PATH_GOOD;
        _uec_mp->processEv(pathid, feedback);
        // bool ecn_mark = pkt.flags() & ECN_CE;
        // _path_qualities.at(pathid) = (ecn_mark * 0.25) + (_path_qualities.at(pathid) * 0.75);// (high is bad)
    }
}


void ConstantErasureCcaSrcDRB::doNextEvent() {
    if (!_established) {
        //cout << "Establishing connection" << endl;
        startflow();
        return;
    }
}

void 
ConstantErasureCcaSrcDRB::connect(ConstantErasureCcaSinkDRB& sink, simtime_picosec startTime, uint32_t destination, const Route& routeout, const Route& routein) {
    _sink = &sink;
    _destination = destination;
    // For DRB, use the topology to select the route based on pointer value
    if (_drb) {
        // Initialize DRB pointers and get initial route
        const Route* drb_route = get_drb_route(destination, false);  // false = data packet
        
        // Use the cache logic to set the initial _route
        uint32_t initial_ptr = _drb_state.initial_data_ptr;
        Route* new_route = drb_route->clone();
        new_route->push_back(_sink);
        _drb_data_routes[initial_ptr] = new_route;
        _route = new_route;

        // Increment packet count so first packet (sent via send_packets->send_next_packet) will use next pointer value
        // This ensures pointer cycling starts from the second packet, not the first
        _drb_state.data_count++;
    } else {
        Route* new_route = routeout.clone(); // make a copy, as we may be switching routes and don't want to append the sink more than once
        new_route->push_back(_sink);
        _route = new_route;
    }

    _flow.set_id(get_id()); // identify the packet flow with the source that generated it
    // cout << "connect, flow id is now " << _flow.get_id() << endl;
    // cout << "connect, flow id (flow_id()) is now " << _flow.flow_id() << endl;
    _scheduler = dynamic_cast<ConstBaseScheduler*>(routeout.at(0));
    _scheduler->add_src(_flow.flow_id(), this);
    _sink->connect(*this, routein);
    eventlist().sourceIsPending(*this,startTime);
    _start_time = startTime;
}

void
ConstantErasureCcaSrcDRB::move_path_flow_label() {
    // cout << timeAsUs(eventlist().now()) << " " << nodename() << " td move_path\n";
    _pathid++;
    // if (adaptive()) {
    //     _pathid = _pathid % _ev_count;
    //     if (_packets_sent < _ev_count +1) {
    //         return;
    //     }
    //     std::vector<double> copy = _path_qualities;
    //     std::sort(copy.begin(), copy.end());
    //     double upperquartile = copy[(int)(_ev_count * 8 / 10)];
    //     double median = copy[(int)(_ev_count * 5 / 10)];

    //     double sum = std::accumulate(_path_qualities.begin(), _path_qualities.end(), 0.0);
    //     double average = sum / _path_qualities.size();
    //     double sq_sum = std::inner_product(_path_qualities.begin(), _path_qualities.end(), _path_qualities.begin(), 0.0, std::plus<double>(), [average](double x, double y){return std::pow(x - average, 2);});
    //     double variance = sq_sum / _path_qualities.size();
    //     double sd = std::sqrt(variance);

    //     while (_path_qualities[_pathid] > upperquartile &&  _path_qualities[_pathid] > average + 2 * sd) { // if path was skipped, should give it another try?
    //         // cout << "bad path " << _pathid << endl;
    //         _path_skipped[_pathid] = true;
    //         _path_qualities[_pathid] =  0.125 + 0.75 * _path_qualities[_pathid]; // this way, if you get an ECN mark every time you try it, that is worse than getting an ECN mark half of the time
    //         _pathid = (_pathid + 1) % _ev_count;
    //     }
    //     _path_skipped[_pathid] = false;
        
    // }
}

void
ConstantErasureCcaSrcDRB::reroute(const Route &routeout) {
    Route* new_route = routeout.clone();
    new_route->push_back(_sink);
    _route = new_route;
}

int
ConstantErasureCcaSrcDRB::queuesize(int flow_id) {
    return _scheduler->src_queuesize(flow_id);
}

bool
ConstantErasureCcaSrcDRB::check_stoptime() {
    if (_stop_time && eventlist().now() >= _stop_time) {
        _stopped = true;
        _stop_time = 0;
    }
    if (_stopped) {
        return true;
    }
    return false;
}

void ConstantErasureCcaSrcDRB::set_app_limit(int pktps) {
    _app_limited = pktps;
    send_packets();
}

void 
ConstantErasureCcaSrcDRB::startflow() {
    _established = true; // send data from the start
    send_packets();
    _pacer.schedule_send(_pacing_delay);
}

bool ConstantErasureCcaSrcDRB::is_complete() {
    return _bytes_acked >= flow_size();
}


////////////////////////////////////////////////////////////////
//  SINK
////////////////////////////////////////////////////////////////

ConstantErasureCcaSinkDRB::ConstantErasureCcaSinkDRB() 
    : DataReceiver("ConstantErasureCcaSinkDRB")
{
    _bytes_received = 0;
    _src = NULL;
    _nodename = "constanterasureccasink";
}

void
ConstantErasureCcaSinkDRB::connect(ConstantErasureCcaSrcDRB& src, const Route& route) {
    _src = &src;
    Route *rt = route.clone();
    rt->push_back(&src);
    _route = rt;
}

void
ConstantErasureCcaSinkDRB::receivePacket(Packet& pkt) {
    ConstantCcaPacket *p = (ConstantCcaPacket*)(&pkt);
    _bytes_received += p->size();
    ConstantCcaPacket::seq_t seqno = p->seqno();

    // For DRB, dynamically select route for ACK based on pointer value
    const Route* ack_route = _route;
    uint32_t ack_ptr_value = 0;

    if (_src->_drb) {

        if (_src->_drb_state_initialized) {
            ConstantErasureCcaSrcDRB::DrbPointers& drb_ptr = _src->_drb_state;

            // Calculate current ACK pointer based on packet count
            uint32_t ack_modulo = drb_ptr.ack_modulo;
            ack_ptr_value = drb_ptr.initial_ack_ptr;
            if (ack_modulo > 1) {
                ack_ptr_value = (ack_ptr_value + drb_ptr.ack_count) % ack_modulo;
            }

            // Check cache for ACK route
            if (_src->_drb_ack_routes[ack_ptr_value] == nullptr) {
                // ACKs go from p->dst() (current destination, which is the sink) to p->src() (the original source)
                const Route* drb_route = _src->_top->get_drb_selected_path(p->dst(), p->src(), ack_ptr_value, true);
                Route* new_route = drb_route->clone();
                new_route->push_back(_src);
                _src->_drb_ack_routes[ack_ptr_value] = new_route;
                
                // drb_route from get_drb_selected_path(..., true) is a new object, delete it
                delete drb_route;
            }
            ack_route = _src->_drb_ack_routes[ack_ptr_value];
            
            // Increment ACK packet count for this destination
            drb_ptr.ack_count++;
        }
    }

    // if (_bytes_received >= _src->flow_size()) {
        ConstantCcaAck* newAck = ConstantCcaAck::newpkt(p->flow(), *ack_route, 
                                    0, seqno + p->size() + 1, _bytes_received,
                                    p->ts(), p->src(), p->dst(), p->pathid());
        newAck->set_direction(NONE);

        newAck->sendOn();
    // }
    p->free();
}



