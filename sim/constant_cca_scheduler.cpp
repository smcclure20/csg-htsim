// -*- c-basic-offset: 4; indent-tabs-mode: nil -*- 
#include <math.h>
#include "constant_cca_scheduler.h"
#include "constant_cca_packet.h"


// We have one Swift Scheduler per sending host.  Multiple Swift flows
// share the same scheduler, and multiple subflows in the same MP
// Swift flow share the same scheduler.
//
// The Scheduler maintains a (small) output queue, which can be Fifo
// or Fair, and can callback to the sending swift when another packet
// is needed.  This avoids builing a large IP output queue.  This is a
// rough model of what happens when a NIC maintains an RDMA request
// queue, but only generates the data to send using DMA when the
// network can send.

ConstBaseScheduler::ConstBaseScheduler(linkspeed_bps bitrate, EventList &eventlist, QueueLogger* logger)
    : BaseQueue(bitrate, eventlist, logger), _pkt_count(0) {
}

void
ConstBaseScheduler::add_src(int32_t flow_id, ConstScheduledSrc* src) {
    // cout << "add_subflow " << flow_id << " src " << src << endl;
    // make sure we don't add the same flow_id more than once
    assert(_queue_counts.find(flow_id) == _queue_counts.end());
    
    _queue_counts[flow_id] = 0;
    _srcs[flow_id] = src;
}

void
ConstBaseScheduler::receivePacket(Packet & pkt) {
    int flow_id = pkt.flow_id();
    //cout << "recv_packet " << this << " flow_id " << flow_id << " count " << _pkt_count << " flow_count " << _queue_counts[flow_id] << endl;
    if (pkt.type() == SWIFT) {
      assert(_srcs.find(flow_id) != _srcs.end());
      _queue_counts[flow_id]++;
    }
    enqueue(pkt);
    //cout << "recv_packet2 " << this << " count " << _pkt_count << endl;
    if (_pkt_count == 1) {
        beginService();
    }
}

void
ConstBaseScheduler::beginService() {
    /* schedule the next dequeue event */
    //cout << "begin " << this << endl;
    assert(!empty());
    Packet* p = next_packet();
    if (p->type() == SWIFT) {
        // swift timestamps are updated when the packet is sent by the
        // NIC, not when they leave the swift CC module
        ConstantCcaPacket *sp = static_cast<ConstantCcaPacket*>(p);
        sp->set_ts(eventlist().now());
    }
    eventlist().sourceIsPendingRel(*this, drainTime(p));
}

void
ConstBaseScheduler::doNextEvent()
{
    completeService();
}

void
ConstBaseScheduler::completeService()
{
    //cout << "comp_svc " << this << endl;
    /* dequeue the packet */
    Packet* pkt = dequeue();
    int flow_id = pkt->flow_id();
    packet_type ptype = pkt->type();
    pkt->flow().logTraffic(*pkt, *this, TrafficLogger::PKT_DEPART);
    if (_logger) _logger->logQueue(*this, QueueLogger::PKT_SERVICE, *pkt);

    /* tell the packet to move on to the next pipe */
    pkt->sendOn();

    if (!empty()) {
        /* schedule the next dequeue event */
        beginService();
    }

    // request more packets
    if (ptype == SWIFT) {
      _queue_counts[flow_id]--;
      _srcs[flow_id]->send_callback();
    }
}


/************************************************************************/
/* FiFo Scheduler                                                       */
/************************************************************************/
 
ConstFifoScheduler::ConstFifoScheduler(linkspeed_bps bitrate, EventList &eventlist, QueueLogger* logger)
    : ConstBaseScheduler(bitrate, eventlist, logger) {
}

void
ConstFifoScheduler::enqueue(Packet& pkt) {
    _queue.push_front(&pkt);
    _pkt_count++;
    assert(_pkt_count == _queue.size());
}

Packet*
ConstFifoScheduler::next_packet() {
    return _queue.back();
}

Packet*
ConstFifoScheduler::dequeue() {
    assert (_pkt_count > 0);
    Packet* packet = _queue.back();
    _queue.pop_back();
    _pkt_count--;
    assert(_pkt_count == _queue.size());
    return packet;
}


ConstFairScheduler::ConstFairScheduler(linkspeed_bps bitrate, EventList &eventlist, QueueLogger* logger)
    : ConstBaseScheduler(bitrate, eventlist, logger)  {
    _current_queue = _queue_map.begin();
    _next_packet = NULL;
}


void
ConstFairScheduler::enqueue(Packet& pkt) {
    // cout << "enqueue " << this << endl;
    list <Packet*>* pkt_queue;
    if (queue_exists(pkt)) {
        pkt_queue = find_queue(pkt);
    }  else {
        pkt_queue = create_queue(pkt);
    }
    //we add packets to the front,remove them from the back
    pkt_queue->push_front(&pkt);
    _pkt_count++;
}

Packet*
ConstFairScheduler::next_packet() {
    // cout << "next_packet " << this << endl;
    assert(_pkt_count > 0);
    assert(!_next_packet);
    while (1) {
        if (_current_queue == _queue_map.end())
            _current_queue = _queue_map.begin();
        list <Packet*>* pkt_queue = _current_queue->second;
        _current_queue++;
        if (!pkt_queue->empty()) {
            //we add packets to the front,remove them from the back
            Packet* packet = pkt_queue->back();
            pkt_queue->pop_back();
            _next_packet = packet; 
            return packet;
        }
        // there are packets queued, so we'll eventually find a queue
        // that lets this terminate
    }
}

Packet* 
ConstFairScheduler::dequeue() {
    // cout << "dequeue " << this << endl;
    assert(_next_packet);  // we expect a call to next_packet() first
    Packet *p = _next_packet;
    _next_packet = NULL;
    assert(_pkt_count > 0);
    _pkt_count--;
    return p;
}

bool 
ConstFairScheduler::queue_exists(const Packet& pkt) {
    typename map <flowid_t, list<Packet*>*>::iterator i;
    i = _queue_map.find(pkt.flow_id());
    if (i == _queue_map.end())
        return false;
    return true;
}

list<Packet*>* 
ConstFairScheduler::find_queue(const Packet& pkt) {
    typename map <flowid_t, list<Packet*>*>::iterator i;
    i = _queue_map.find(pkt.flow_id());
    if (i == _queue_map.end())
        return 0;
    return i->second;
}

list<Packet*>* 
ConstFairScheduler::create_queue(const Packet& pkt) {
    list<Packet*>* new_queue = new(list<Packet*>);
    _queue_map.insert(pair<int32_t, list<Packet*>*>(pkt.flow_id(), new_queue));
    return new_queue;
}


