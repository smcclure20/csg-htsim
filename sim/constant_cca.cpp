// -*- c-basic-offset: 4; indent-tabs-mode: nil -*- 
#include "constant_cca.h"
#include <iostream>
#include <math.h>

////////////////////////////////////////////////////////////////
//  PACER
////////////////////////////////////////////////////////////////

// TODO NOW: our CCA will not switch modes, so we can get rid of things relevant to that
ConstantCcaPacer::ConstantCcaPacer(ConstantCcaSubflowSrc& src, EventList& event_list, simtime_picosec interpacket_delay)
    : EventSource(event_list,"constant_cca_pacer"), _src(&src), _interpacket_delay(interpacket_delay) {
    _last_send = eventlist().now();
    _next_send = 0;
}

void
ConstantCcaPacer::schedule_send(simtime_picosec delay) {
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
ConstantCcaPacer::allow_send() {
    simtime_picosec now = eventlist().now();
    if (_last_send == 0 && _src->_highest_sent == 0) {
        // first packet, so we can send
        return true;
    }
    return now >= _interpacket_delay + _last_send; 
}

void
ConstantCcaPacer::cancel() {
    _interpacket_delay = 0;
    _next_send = 0;
    eventlist().cancelPendingSource(*this);
}

// called when we're in window-mode to update the send time so it's always correct if we
// then go into paced mode
void
ConstantCcaPacer::just_sent() {
    _last_send = eventlist().now();
}

void
ConstantCcaPacer::doNextEvent() {
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

ConstantCcaSubflowSrc::ConstantCcaSubflowSrc(ConstantCcaSrc& src, TrafficLogger* pktlogger, int subflow_id, simtime_picosec pacing_delay)
    : EventSource(src.eventlist(), "swift_subflow_src"), _flow(pktlogger), _src(src), _pacer(*this, src.eventlist(), pacing_delay)
{
    _highest_sent = 0;
    _packets_sent = 0;
    _established = false;
    _highest_sent_abs = 0;

    _last_acked = 0;
    _dupacks = 0;
    _rtt = 0;
    _rto = timeFromMs(1);
    _min_rto = timeFromUs((uint32_t)100);
    _max_rtt = 0;
    _mdev = 0;
    _recoverq = 0;
    _in_fast_recovery = false;
    _inflate = 0;
    _drops = 0;
    _rto_count = 0;
    _total_dupacks = 0;

    _retransmit_cnt = 0;
    _pacing_delay = pacing_delay;  // 
    _deferred_send = false; // true if we wanted to send, but no scheduler said no.

    // PLB init
    _plb_interval = timeFromMs(10);  // disable PLB til we've seen some traffic
    _path_index = 0;  
    _last_good_path = src.eventlist().now();
    _plb_threshold_ecn = src._plb_threshold_ecn;

    _path_qualities.assign(src._ev_count, 0.0);
    _path_skipped.assign(src._ev_count, false);
    
    _rtx_timeout_pending = false;
    _RFC2988_RTO_timeout = timeInf;

    _mss = Packet::data_packet_size();
    _const_cwnd = 100 * _mss; 
    _maxcwnd = 0xffffffff;//200*_mss;

    _min_cwnd = 10 * _mss;  // guess - if we go less than 10 bytes, we probably get into rounding 
    _max_cwnd = 1000 * _mss;  // maximum cwnd we can use.  Guess - how high should we allow cwnd to go?  Presumably something like B*target_delay?

    _nodename = "constcca_subflowsrc_" + std::to_string(get_id()) + "_" + std::to_string(subflow_id);

    _min_rtt = timeFromMs(1000);

    // _pending_retransmit = 0;
    _nack_rtxs = 0;

    deferred_retransmit = false;

    _repeated_nack_rtxs = 0;
}

void
ConstantCcaSubflowSrc::update_rtt(simtime_picosec delay) {
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
        _min_rtt = min(_min_rtt, delay);
        _max_rtt = max(_max_rtt, delay);
        // _dupack_threshold = (int)((_max_rtt - _min_rtt) / _pacing_delay) + 1;
        // if (_dupack_threshold < 3) {
        //     _dupack_threshold = 3;
        // }
    }

    if (_rto <_min_rto)
        _rto = _min_rto;
}

void
ConstantCcaSubflowSrc::adjust_cwnd(simtime_picosec delay, ConstantCcaAck::seq_t ackno) {
    //cout << "adjust_cwnd delay " << timeAsUs(delay) << endl;
    _prev_cwnd = _const_cwnd;
    simtime_picosec now = eventlist().now(); 

    //compute rtt
    update_rtt(delay);

    _const_cwnd = ((double)_rtt / (double)_pacing_delay) * mss();
    if (_const_cwnd < _min_cwnd) {
        _const_cwnd = _min_cwnd;
    }
}

void
ConstantCcaSubflowSrc::handle_ack(ConstantCcaAck::seq_t ackno) {
    simtime_picosec now = eventlist().now();
    if (ackno > _last_acked) { // a brand new ack
        _RFC2988_RTO_timeout = now + _rto;// RFC 2988 5.3
    
        if (ackno >= _highest_sent) {
            
            _highest_sent = ackno;
            //cout << timeAsUs(now) << " " << nodename() << " highest_sent now  " << _highest_sent << endl;
            _RFC2988_RTO_timeout = timeInf;// RFC 2988 5.2
        }

        if (!_in_fast_recovery) {
            // best behaviour: proper ack of a new packet, when we were expecting it.
            // clear timers.  swift has already calculated new cwnd.
            _last_acked = ackno;
            _dupacks = 0;
            send_packets();
            return;
        }
        // We're in fast recovery, i.e. one packet has been
        // dropped but we're pretending it's not serious
        if (ackno >= _recoverq) { 
            // got ACKs for all the "recovery window": resume
            // normal service

            //uint32_t flightsize = _highest_sent - ackno;
            //_cwnd = min(_ssthresh, flightsize + _mss);
            // in NewReno, we'd reset the cwnd here, but I think in swift we continue with the delay-adjusted cwnd.

            _inflate = 0;
            _last_acked = ackno;
            _dupacks = 0;
            _in_fast_recovery = false;

            _retransmit_cnt = 0;
            send_packets();
            return;
        }
        // In fast recovery, and still getting ACKs for the
        // "recovery window"
        // This is dangerous. It means that several packets
        // got lost, not just the one that triggered FR.
        uint32_t new_data = ackno - _last_acked;
        if (new_data < _inflate) {
            _inflate -= new_data;
        } else {
            _inflate = 0;
        }
        _last_acked = ackno;
        _inflate += mss();
        retransmit_packet();
        send_packets();
        return;
    }
    if (_src._fr_disabled) {
        // fast recovery disabled
        return;
    }
    // It's a dup ack
    _total_dupacks++;
    if (_in_fast_recovery) { // still in fast recovery; hopefully the prodigal ACK is on it's way
        _inflate += mss();
        if (_inflate > _const_cwnd) {
            // this is probably bad
            _inflate = _const_cwnd;
            // cout << "hit inflate limit" << endl;
        }
        send_packets();
        return;
    }
    // Not yet in fast recovery. What should we do instead?
    _dupacks++;

    if (_dupacks!=_src._dupack_threshold)  { // not yet serious worry
        send_packets();
        return;
    }
    // _dupacks==3
    if (_last_acked < _recoverq) {  
        /* See RFC 3782: if we haven't recovered from timeouts
           etc. don't do fast recovery */
        return;
    }

    if (_last_acked == _highest_sent) { 
        // We have gotten the dupacks because we haven't sent anything else (in pacing mode)
        return;
    }
  
    // begin fast recovery
  
    //only count drops in CA state
    _drops++;
    retransmit_packet();
    _in_fast_recovery = true;
    _recoverq = _highest_sent; // _recoverq is the value of the
    // first ACK that tells us things
    // are back on track
}


int 
ConstantCcaSubflowSrc::send_packets() {
    // if (_last_acked >= _src._flow_size && _src._completion_time == 0){ // should this be in receive packet instead?
    //     _src._completion_time = eventlist().now();
    //     // _pacer.cancel();
    //     cout << "Flow " << _name << " finished at " << timeAsUs(eventlist().now()) << " total bytes " << _last_acked<< endl;
    // }

    // if (deferred_retransmit) { // should also check pacer? How else could this be called?
    //     retransmit_packet();
    //     deferred_retransmit = false;
    //     return 1;
    // }

    if (_pending_retransmit.size() > 0) { // should also check pacer? How else could this be called?
        // Only do a pending retransmit if it's not already been acked
        while (_pending_retransmit.size() > 0 && *_pending_retransmit.begin() <= _last_acked && *_pending_retransmit.begin()-1 <= _highest_sent) {
            _pending_retransmit.erase(_pending_retransmit.begin());
        }
        if (_pending_retransmit.size() > 0 && *_pending_retransmit.begin() > _last_acked && *_pending_retransmit.begin()-1 <= _highest_sent) {
            uint64_t seqno = *_pending_retransmit.begin();
            retransmit_packet(seqno);
            return 1;
        }
        
    }
    // maybe should just rewrite this. What should the logic be?
    // Only send if the pacer allows it and the window has room
    uint32_t c = _const_cwnd + _inflate; // TODO: get rid of the cwnd at all
    int sent_count = 0;
    // cout << eventlist().now() << " " << nodename() << " cwnd " << _swift_cwnd << " + " << _inflate << endl;
    if (!_established){
        //send SYN packet and wait for SYN/ACK
        Packet * p  = ConstantCcaPacket::new_syn_pkt(_flow, *(_route), 0, 1, _src._destination, _src._addr, _pathid);
        _highest_sent = 0;

        p->sendOn();
        _pacer.just_sent();

        if(_RFC2988_RTO_timeout == timeInf) {// RFC2988 5.1
            _RFC2988_RTO_timeout = eventlist().now() + _rto;
        }        
        //cout << "Sending SYN, waiting for SYN/ACK" << endl;
        return sent_count;
    }

    while ((_last_acked + c >= _highest_sent + mss()) && (_src.more_data_available() || ((_highest_sent <= _highest_sent_abs)))) {
        if (!_pacer.allow_send()) {
            // cout << nodename() << " calling schedule send 3" << endl;
            // _pacer.schedule_send(_pacing_delay);
            return sent_count;
        }

        bool sent = send_next_packet();
        if (sent) {
            sent_count++;
        } else {
            break;
        }

        if(_RFC2988_RTO_timeout == timeInf) {// RFC2988 5.1
            _RFC2988_RTO_timeout = eventlist().now() + _rto;
            //cout << timeAsUs(eventlist().now()) << " " << nodename() << " RTO at " << timeAsUs(_RFC2988_RTO_timeout) << "us" << endl;
        }
    }
    return sent_count;
}

bool
ConstantCcaSubflowSrc::send_next_packet() {
    // ask the scheduler if we can send now
    if (_src.queuesize(_flow.flow_id()) > 2) {
        // no, we can't send.  We'll be called back when we can.
        _deferred_send = true;
        return false;
    }
    _deferred_send = false;

    uint64_t seqno;
    if (_src.more_data_available()) { // todo: clean this up
        seqno = _highest_sent + 1;
    } else if ((_highest_sent <= _highest_sent_abs) ) { //((_highest_sent <= _recoverq) && (_src._highest_dsn_ack < _src._flow_size)) {
        seqno = _highest_sent + 1;
    } else {
        // cout << timeAsUs(eventlist().now()) << " " << nodename() << " no more data" << endl;
        return false;
    }

    ConstantCcaPacket::seq_t dsn;
    if (!_dsn_map.empty() && _dsn_map.count(_highest_sent + 1) > 0) {
        dsn = _dsn_map[_highest_sent+1];
        if (dsn == 0) {
            cout << "highest_sent " << _highest_sent << " last_acked " << _last_acked << " ,highest in map: " << (uint64_t) prev(_dsn_map.end())->first << endl;
            assert(false);
        }
    } else {
        dsn = _src._highest_dsn_sent+1;
        _dsn_map[_highest_sent+1] = dsn;
        _src._highest_dsn_sent += mss();
        _highest_sent_abs = _highest_sent; // it can be possible for neither hsent or recoverq to be the actual highest sent (multiple losses before recovering), so we need this
    }

    ConstantCcaPacket* p = ConstantCcaPacket::newpkt(_flow, *_route, seqno, dsn, mss(), _src._destination, _src._addr, _pathid);
    // cout << timeAsUs(eventlist().now()) << " " << nodename() << " sent " << _highest_sent+1 << "-" << _highest_sent+mss() << endl;
    _highest_sent += mss();  
    _packets_sent++;
    _repeated_nack_rtxs = 0;

    p->set_ts(eventlist().now());
    p->sendOn();
    _pacer.just_sent();

    if (_src.spraying()) { // switch path on every packet
        move_path_flow_label();
    }

    return true;
}

void
ConstantCcaSubflowSrc::send_callback() {
    if (_deferred_send == false) {
        // no need to be here
        return;
    }

    _deferred_send = false;
    // We had previously wanted to send but queue said no.  Now it says yes.
    send_packets();
}

void 
ConstantCcaSubflowSrc::retransmit_packet() {
    if (!_established){
        assert(_highest_sent == 1);

        Packet* p  = ConstantCcaPacket::new_syn_pkt(_flow, *_route, 1, 1, _src._destination, _src._addr, _pathid);
        p->sendOn();

        cout << "Resending SYN, waiting for SYN/ACK" << endl;
        return;        
    }
    if (_last_acked >= _src._flow_size) {
        return;
    }
    // if (!_pacer.allow_send()) {
    //     deferred_retransmit = true;
    //     return;
    // }
    // cout << timeAsUs(eventlist().now()) << " " << nodename() << " retransmit_packet " << endl;
    // cout << timeAsUs(eventlist().now()) << " sending seqno " << _last_acked+1 << endl;
    ConstantCcaPacket::seq_t dsn = _dsn_map[_last_acked+1];
    ConstantCcaPacket* p = ConstantCcaPacket::newpkt(_flow, *_route, _last_acked+1, dsn, mss(), _src._destination, _src._addr, _pathid); // TODO: should probably increment the path label after sending here too 

    p->set_ts(eventlist().now());
    p->sendOn();
    _pacer.just_sent();

    _packets_sent++;

    if(_RFC2988_RTO_timeout == timeInf) {// RFC2988 5.1 // the fact that with NACKs this broke concerns me - are we getting worse performance? I thought this was necessary since otherwise you won't timeout if you were first in fast retransmit, right? Let's test
        _RFC2988_RTO_timeout = eventlist().now() + _rto;
    }
}

void 
ConstantCcaSubflowSrc::retransmit_packet(uint64_t seqno) {
    // if (_repeated_nack_rtxs > 1 ) {
    //     _pacer.cancel();
    //     _nack_backoff = _nack_backoff * 2;
    //     if (_nack_backoff > timeFromMs(1)) {
    //         _nack_backoff = timeFromMs(1);
    //     }
    //     _pacer.schedule_send(_nack_backoff);
    // } else {
    //     _nack_backoff = _pacing_delay;
    // }
    if (!_pacer.allow_send()) {
        _pending_retransmit.insert(seqno);
        return;
    }
    _repeated_nack_rtxs++;
    ConstantCcaPacket::seq_t dsn = _dsn_map[seqno];
    ConstantCcaPacket* p = ConstantCcaPacket::newpkt(_flow, *_route, seqno, dsn, mss(), _src._destination, _src._addr, _pathid);

    p->set_ts(eventlist().now());
    p->sendOn();

    _packets_sent++;
    _pending_retransmit.erase(seqno);
    _nack_rtxs++;
    _pacer.just_sent();
    // _pending_retransmit = 0; // maybe this should be a list?

    if(_RFC2988_RTO_timeout == timeInf) {// RFC2988 5.1
        _RFC2988_RTO_timeout = eventlist().now() + _rto;
    }
}

void
ConstantCcaSubflowSrc::receivePacket(Packet& pkt) 
{
    if (pkt.header_only()) {
        ConstantCcaPacket* dp = (ConstantCcaPacket*)(&pkt);
        ConstantCcaPacket::seq_t seqno = dp->seqno();
        if (seqno > _last_acked && seqno-1 <= _highest_sent) {
            // this is a packet that we haven't gotten an ACK for yet
            retransmit_packet(seqno);
        }
        
        dp->free();
        return;
    }
    simtime_picosec ts_echo;
    ConstantCcaAck *p = (ConstantCcaAck*)(&pkt);
    ConstantCcaAck::seq_t ackno = p->ackno();
    ConstantCcaAck::seq_t ds_ackno = p->ds_ackno();
    // _acks_received.push_back(ackno);

    if (p->is_nack()) {
        // cout << nodename() << " received NACK " << ackno << endl;
        // potential additional check: if we have already retransmitted this packet, don't retransmit again?
        if (ackno > _last_acked && ackno-1 <= _highest_sent) {
            // this is a NACK for a packet that we haven't gotten an ACK for yet
            retransmit_packet(ackno);
        }
        
        p->free();
        return;
    }
  
  
    ts_echo = p->ts_echo();
    uint32_t pathid = p->pathid();
    p->free();

    // cout << timeAsUs(eventlist().now()) << " " << nodename() << " recvack  " << ackno << " ts_echo " << timeAsUs(ts_echo) << endl;
    //cout << timeAsUs(eventlist().now()) << " " << nodename() << " highest_sent was  " << _sub->_highest_sent << endl;

    if (ackno < _last_acked) {
        //cout << "O seqno" << ackno << " last acked "<< _sub->_last_acked;
        return;
    }

    if (ackno==0){
        //assert(!_sub->_established);
        _established = true;
    } else if (ackno>0 && !_established) {
        cout << "Should be _established " << ackno << endl;
        assert(false);
    }

    assert(ackno >= _last_acked);  // no dups or reordering allowed in this simple simulator TODO: This should break 
    simtime_picosec delay = eventlist().now() - ts_echo;
    adjust_cwnd(delay, ackno);


    if (_src.plb()) {
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
        if (total_marks >= _plb_threshold_ecn) {
            // PLB (simple version)
            if (now - _last_good_path > _plb_interval) {
                // cout << "moving " << timeAsUs(now) << " last_good " << timeAsUs(_last_good_path) << " RTT " << timeAsUs(_rtt) << endl;
                _last_good_path = now;
                move_path_flow_label();
            }
        }
    }
    if (_src.adaptive()) {
        bool ecn_mark = pkt.flags() & ECN_CE;
        _path_qualities[pathid] = (ecn_mark * 0.25) + (_path_qualities[pathid] * 0.75);// (high is bad)
        // _path_qualities[_pathid] = (delay * 0.25) + (_path_qualities[_pathid] * 0.75); // can also do this based on ecn
    }

    _src.update_dsn_ack(ds_ackno);

    handle_ack(ackno);
}

void
ConstantCcaSubflowSrc::rtx_timer_hook(simtime_picosec now, simtime_picosec period) {
    //cout << timeAsUs(eventlist().now()) << " " << nodename() << " rtx_timer_hook" << endl;
    if (now <= _RFC2988_RTO_timeout || _RFC2988_RTO_timeout==timeInf) 
        return;

    if (_highest_sent == 0) 
        return;

    if (_src._highest_dsn_ack >= _src._flow_size) 
        return;

    // cout << timeAsUs(eventlist().now()) << " " << nodename() << " At " << now/(double)1000000000<< " RTO " << _rto/1000000000 << " MDEV " 
    //      << _mdev/1000000000 << " RTT "<< _rtt/1000000000 << " SEQ " << _last_acked / mss() << " HSENT "  << _highest_sent << " LASTACKED " << _last_acked 
    //      << " CWND "<< _const_cwnd/mss() << " FAST RECOVERY? " <<         _in_fast_recovery << " Flow ID " 
    //      <<  get_id() << endl;

    _rto_count++;

    // here we can run into phase effects because the timer is checked
    // only periodically for ALL flows but if we keep the difference
    // between scanning time and real timeout time when restarting the
    // flows we should minimize them !
    if(!_rtx_timeout_pending) {
        _rtx_timeout_pending = true;

        // check the timer difference between the event and the real value
        simtime_picosec too_late = now - (_RFC2988_RTO_timeout);
 
        // careful: we might calculate a negative value if _rto suddenly drops very much
        // to prevent overflow but keep randomness we just divide until we are within the limit
        while(too_late > period) too_late >>= 1;

        // carry over the difference for restarting
        simtime_picosec rtx_off = (period - too_late)/200;
 
        eventlist().sourceIsPendingRel(*this, rtx_off);

        //reset our rtx timerRFC 2988 5.5 & 5.6

        _rto *= 2;
        //if (_rto > timeFromMs(1000))
        //  _rto = timeFromMs(1000);
        _RFC2988_RTO_timeout = now + _rto;
    }
}

void ConstantCcaSubflowSrc::doNextEvent() {
    if (!_established) {
        //cout << "Establishing connection" << endl;
        _src.startflow();
        return;
    }
    if(_src._highest_dsn_ack >= _src._flow_size) {
        _RFC2988_RTO_timeout = timeInf;
        return;
    }
    if(_rtx_timeout_pending) {
        // _rto_times.push_back(eventlist().now());
        _rtx_timeout_pending = false;

        _in_fast_recovery = false;
        _recoverq = _highest_sent;

        if (_established)
            _highest_sent = _last_acked + mss();

        _dupacks = 0;
        _retransmit_cnt++;
        if (_retransmit_cnt >= _rtx_reset_threshold) {
            _const_cwnd = _min_cwnd;
        } 

        if (_const_cwnd < _min_cwnd) {
            _const_cwnd = _min_cwnd;
        }

        retransmit_packet();
    }
}

void 
ConstantCcaSubflowSrc::connect(ConstantCcaSink& sink, const Route& routeout, const Route& routein, ConstBaseScheduler* scheduler) {
    _subflow_sink = sink.connect(_src, *this, routein);
    Route* new_route = routeout.clone(); // make a copy, as we may be switching routes and don't want to append the sink more than once
    new_route->push_back(_subflow_sink);
    _route = new_route;
    _flow.set_id(get_id()); // identify the packet flow with the source that generated it
    cout << "connect, flow id is now " << _flow.get_id() << endl;
    // cout << "connect, flow id (flow_id()) is now " << _flow.flow_id() << endl;
    assert(scheduler);
    scheduler->add_src(_flow.flow_id(), this);
}

void
ConstantCcaSubflowSrc::move_path_flow_label() {
    // cout << timeAsUs(eventlist().now()) << " " << nodename() << " td move_path\n";
    _pathid++;
    if (_src.adaptive()) {
        _pathid = _pathid % _src._ev_count;
        double sum = std::accumulate(_path_qualities.begin(), _path_qualities.end(), 0.0);
        double average = sum / _path_qualities.size();
        if (_path_qualities[_pathid] > average) {
            // cout << "bad path " << _pathid << endl;
            _pathid = (_pathid + 1) % _src._ev_count;
            _path_skipped[_pathid] = true;
        } else {
            _path_skipped[_pathid] = false;
        }
    }
}

void
ConstantCcaSubflowSrc::move_path(bool permit_cycles) {
    cout << timeAsUs(eventlist().now()) << " " << nodename() << " td move_path\n";
    if (_src._paths.size() == 0) {
        cout << nodename() << " cant move_path\n";
        return;
    }
    _path_index++;
    if (!permit_cycles) {
        // if we've moved paths so often we've run out of paths, I want to know
        assert(_path_index < _src._paths.size()); 
    } else if (_path_index >= _src._paths.size()) {
        _path_index = _path_index % _src._paths.size();
    }
    Route* new_route = _src._paths[_path_index]->clone();
    new_route->push_back(_subflow_sink);
    _route = new_route;
}

void
ConstantCcaSubflowSrc::reroute(const Route &routeout) {
    Route* new_route = routeout.clone();
    new_route->push_back(_subflow_sink);
    _route = new_route;
}

////////////////////////////////////////////////////////////////
//  CONSTANT CCA SRC
////////////////////////////////////////////////////////////////

ConstantCcaSrc::ConstantCcaSrc(ConstantCcaRtxTimerScanner& rtx_scanner, EventList &eventlist, uint32_t addr, simtime_picosec pacing_delay, TrafficLogger* pkt_logger)
    : EventSource(eventlist,"constcca"),  _traffic_logger(pkt_logger), _rtx_timer_scanner(&rtx_scanner)
{
    _mss = Packet::data_packet_size();
    _scheduler = NULL;
    _flow_size = ((uint64_t)1)<<63;
    _stop_time = 0;
    _stopped = false;
    _highest_dsn_sent = 0;
    _completion_time = 0;
    _highest_dsn_ack = 0;
    _start_time = 0;
    // PLB init
    _plb = false; // enable using enable_plb()
    _spraying = false;
    _fr_disabled = false;
    _adaptive = false;
    _ev_count = 32;

    _addr = addr;
    _pacing_delay = pacing_delay;
}

void 
ConstantCcaSrc::connect(ConstantCcaSink& sink, simtime_picosec starttime, uint32_t no_of_subflows, uint32_t destination, const Route& routeout, const Route& routein) {
    _destination = destination;
    _sink=&sink;
    // Route* new_route = routeout.clone(); // make a copy, as we may be switching routes and don't want to append the sink more than once
    // new_route->push_back(_sink);
    _scheduler = dynamic_cast<ConstBaseScheduler*>(routeout.at(0));
    for (uint32_t i = 0; i < no_of_subflows; i++) {
        ConstantCcaSubflowSrc* subflow = new ConstantCcaSubflowSrc(*this, _traffic_logger, _subs.size(), _pacing_delay * no_of_subflows);
        _subs.push_back(subflow);
        // assert(_paths.size() > 0);
        // if (_paths.size() < no_of_subflows) {
        //     cout << "More subflows requested than paths available - there will be duplicates\n";
        // }
        // const Route* routeout = _paths[i%_paths.size()];
        // const Route* routeback = _paths[i%_paths.size()]->reverse();
        // assert(routeback);
        subflow->connect(sink, routeout, routein, _scheduler);
        _rtx_timer_scanner->registerSubflow(subflow);
    }
    _start_time = starttime;
    eventlist().sourceIsPending(*this,starttime);
    // cout << "starttime " << timeAsUs(starttime) << endl;
}

void ConstantCcaSrc::update_dsn_ack(ConstantCcaAck::seq_t ds_ackno) {
    //cout << "Flow " << _name << " dsn ack " << ds_ackno << endl;
    _highest_dsn_ack = max(_highest_dsn_ack, ds_ackno);
    if (ds_ackno >= _flow_size && _completion_time == 0){
        _completion_time = eventlist().now();
        for (size_t i = 0; i < _subs.size(); i++) {
            _subs[i]->_pacer.cancel();
        }
        cout << "Flow " << _name << " finished at " << timeAsUs(eventlist().now()) << " total bytes " << ds_ackno << endl;
    }
}

int
ConstantCcaSrc::queuesize(uint32_t flow_id) {
    return _scheduler->src_queuesize(flow_id);
}

bool
ConstantCcaSrc::check_stoptime() {
    if (_stop_time && eventlist().now() >= _stop_time) {
        _stopped = true;
        _stop_time = 0;
    }
    if (_stopped) {
        return true;
    }
    return false;
}

void
ConstantCcaSrc::set_cwnd(uint32_t cwnd) {
    for (size_t i = 0; i < _subs.size(); i++) {
        _subs[i]->_const_cwnd = cwnd;
    }
}

void
ConstantCcaSrc::set_paths(vector<const Route*>* rt_list){
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
    // permute_paths();
    for (size_t i = 0; i < _subs.size(); i++) {
        _subs[i]->_path_index = 0;
        _subs[i]->reroute(*_paths[i]);
    }
}

void
ConstantCcaSrc::permute_paths() {
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
ConstantCcaSrc::startflow() {
    for (size_t i = 0; i < _subs.size(); i++) {
        if (_subs[i]->_established)
            continue; // don't start twice
        _subs[i]->_established = true; // send data from the start
        //cout << eventlist().now() << " " << _subs[i]->nodename() << " (sub" << i << ") started, cwnd " << _subs[i]->_swift_cwnd << endl;
        _subs[i]->send_packets();
        _subs[i]->_pacer.schedule_send(_subs[i]->_pacing_delay); // todo: should probably deliberately offset the subflows
    }
}

bool ConstantCcaSrc::more_data_available() const {
    if (_stop_time && eventlist().now() >= _stop_time) {
        return false;
    }
    return _highest_dsn_sent + mss() <= _flow_size;
}

void ConstantCcaSrc::doNextEvent() {
    // cout << "Starting flow" << endl;
    startflow();
}

uint32_t 
ConstantCcaSrc::drops() {
    uint32_t total = 0;
    for (int i = 0; i < (int)_subs.size(); i++) {
        total += _subs[i]->drops();
    }
    return total;
}

uint32_t 
ConstantCcaSrc::rtos() {
    uint32_t total = 0;
    for (int i = 0; i < (int)_subs.size(); i++) {
        total += _subs[i]->rtos();
    }
    return total;
}

uint32_t 
ConstantCcaSrc::total_dupacks() {
    uint32_t total = 0;
    for (int i = 0; i < (int)_subs.size(); i++) {
        total += _subs[i]->total_dupacks();
    }
    return total;
}

uint32_t
ConstantCcaSrc::packets_sent() {
    uint32_t total = 0;
    for (int i = 0; i < (int)_subs.size(); i++) {
        total += _subs[i]->packets_sent();
    }
    return total;
}

////////////////////////////////////////////////////////////////
// SINK
////////////////////////////////////////////////////////////////

ConstantCcaSink::ConstantCcaSink() 
    : DataReceiver("ConstantCcaSink"), _cumulative_data_ack(0), _buffer_logger(NULL)
{
    _src = NULL;
    _nodename = "constantccasink";
}

ConstantCcaSubflowSink*
ConstantCcaSink::connect(ConstantCcaSrc& src, ConstantCcaSubflowSrc& subflow_src, const Route& route_back) {
    _src = &src;
    ConstantCcaSubflowSink* subflow_sink = new ConstantCcaSubflowSink(*this);
    _subs.push_back(subflow_sink);
    subflow_sink->connect(subflow_src, route_back);
    return subflow_sink;
}

void
ConstantCcaSink::receivePacket(Packet& pkt) {
    ConstantCcaPacket *p = (ConstantCcaPacket*)(&pkt);
    ConstantCcaPacket::seq_t dsn = p->dsn();
    int size = p->size(); // TODO: the following code assumes all packets are the same size
    //cout << "ConstantCcaSink received dsn " << dsn << endl;
    if (dsn == _cumulative_data_ack+1) {
        _cumulative_data_ack = dsn + size - 1;
        while (!_dsn_received.empty() && (*(_dsn_received.begin()) == _cumulative_data_ack+1) ) {
            _dsn_received.erase(_dsn_received.begin());
            _cumulative_data_ack+= size;
        }
    } else if (dsn < _cumulative_data_ack+1) {
        // cout << "Dup DSN received!\n";
    } else {
        // hole in sequence space
        _dsn_received.insert(dsn);
    }
}

uint32_t 
ConstantCcaSink::drops() {
    uint32_t total;
    for (int i = 0; i < (int)_subs.size(); i++) {
        total += _subs[i]->drops();
    }
    return total;
}

uint32_t 
ConstantCcaSink::spurious_retransmits() {
    uint32_t total = 0;
    for (int i = 0; i < (int)_subs.size(); i++) {
        total += _subs[i]->spurious_retransmits();
    }
    return total;
}

uint32_t 
ConstantCcaSink::nacks_sent() {
    uint32_t total = 0;
    for (int i = 0; i < (int)_subs.size(); i++) {
        total += _subs[i]->nacks_sent();
    }
    return total;
}

////////////////////////////////////////////////////////////////
//  SUBFLOW SINK
////////////////////////////////////////////////////////////////

ConstantCcaSubflowSink::ConstantCcaSubflowSink(ConstantCcaSink& sink) 
    : DataReceiver("ConstantCcaSubflowSink"), _sink(sink)
{
    _cumulative_ack = 0;
    _packets = 0;
    _spurious_retransmits = 0;
    _drops = 0;
    _nacks_sent = 0;
    _subflow_src = NULL;
    _nodename = "constantccasubflowsink";
}

void
ConstantCcaSubflowSink::connect(ConstantCcaSubflowSrc& src, const Route& route) {
    _subflow_src = &src;
    Route *rt = route.clone();
    rt->push_back(&src);
    _route = rt;
    _cumulative_ack = 0;
    _spurious_retransmits = 0;
}

void
ConstantCcaSubflowSink::receivePacket(Packet& pkt) {
    ConstantCcaPacket *p = (ConstantCcaPacket*)(&pkt);
    if (p->is_header()) {
        // Do not update anything, just reply to the sender
        uint32_t src = p->src();
        uint32_t dst = p->dst();
        uint32_t pathid = p->pathid();
        uint64_t seqno = p->seqno();
        uint64_t dsn = p->dsn();
        simtime_picosec ts = p->ts();
        p->free();
        // cout << "Sending NACK for packet " << seqno << " to " << _src->nodename() << endl;
        send_nack(ts, src, dst, pathid, seqno, dsn);
        return;
    }
    int size = p->size(); // TODO: the following code assumes all packets are the same size
    simtime_picosec ts = p->ts();
    _packets+= p->size();
    uint32_t seqno = p->seqno();
    if (seqno == _cumulative_ack+1) { // it's the next expected seq no
        _cumulative_ack = seqno + size - 1;
        // are there any additional received packets we can now ack?
        while (!_received.empty() && (*(_received.begin()) == _cumulative_ack+1) ) {
            _received.erase(_received.begin());
            _cumulative_ack += size;
        }
    } else if (seqno < _cumulative_ack+1) {
        // it is before the next expected sequence - must be a spurious retransmit.
        // We want to see if this happens - it generally shouldn't
        // cout << "Spurious retransmit received!\n";
        _spurious_retransmits++;
    } else {
        // it's not the next expected sequence number
        _received.insert(seqno);
    }

    _sink.receivePacket(pkt);

    uint32_t src = p->src();
    uint32_t dst = p->dst();
    uint32_t pathid = p->pathid();
    p->free();

    // whatever the cumulative ack does (eg filling holes), the echoed TS is always from
    // the packet we just received
    send_ack(ts, src, dst, pathid);
}

void 
ConstantCcaSubflowSink::send_ack(simtime_picosec ts, uint32_t ack_dst, uint32_t ack_src, uint32_t pathid) {
    const Route* rt = _route;
    ConstantCcaAck *ack = ConstantCcaAck::newpkt(_subflow_src->flow(), *rt, 0, _cumulative_ack, _sink._cumulative_data_ack, ts, ack_dst, ack_src, pathid);

    ack->sendOn();
}

void 
ConstantCcaSubflowSink::send_nack(simtime_picosec ts, uint32_t ack_dst, uint32_t ack_src, uint32_t pathid, uint64_t seqno, uint64_t dsn) {
    const Route* rt = _route;
    ConstantCcaAck *nack = ConstantCcaAck::newnack(_subflow_src->flow(), *rt, 0, seqno, dsn, ts, ack_dst, ack_src, pathid);

    nack->sendOn();
    _nacks_sent++;
}

////////////////////////////////////////////////////////////////
//  RETRANSMISSION TIMER
////////////////////////////////////////////////////////////////

ConstantCcaRtxTimerScanner::ConstantCcaRtxTimerScanner(simtime_picosec scanPeriod, EventList& eventlist)
    : EventSource(eventlist,"RtxScanner"), _scanPeriod(scanPeriod){
    eventlist.sourceIsPendingRel(*this, _scanPeriod);
}

void 
ConstantCcaRtxTimerScanner::registerSubflow(ConstantCcaSubflowSrc* subflow_src) {
    _srcs.push_back(subflow_src);
}


void ConstantCcaRtxTimerScanner::doNextEvent() {
    simtime_picosec now = eventlist().now();
    for (auto it = _srcs.begin(); it != _srcs.end(); it++) {
        ConstantCcaSubflowSrc* src = *it;
        src->rtx_timer_hook(now, _scanPeriod);
    }
    eventlist().sourceIsPendingRel(*this, _scanPeriod);
}
