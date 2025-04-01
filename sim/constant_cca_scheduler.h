// -*- c-basic-offset: 4; indent-tabs-mode: nil -*- 
#ifndef CONSTCCAQUEUE_H
#define CONSTCCAQUEUE_H

/*
 * A fair queue for swift (and other similar srcs) scheduling
 */

#include <list>
#include "config.h"
#include "eventlist.h"
#include "network.h"
#include "queue.h"
//#include "swift.h"

class ConstScheduledSrc {
public:
    virtual void send_callback() = 0;
};

class ConstBaseScheduler : public BaseQueue {
 public:
    ConstBaseScheduler(linkspeed_bps bitrate, EventList &eventlist, QueueLogger* logger);
    virtual void enqueue(Packet& pkt) = 0;
    virtual Packet* dequeue() = 0;
    virtual Packet* next_packet() = 0;
    virtual void doNextEvent();

    // start serving the item at the head of the queue
    virtual void beginService(); 

    // wrap up serving the item at the head of the queue
    virtual void completeService(); 

    virtual void receivePacket(Packet& pkt);
    inline bool empty() const {return _pkt_count == 0;}
    virtual mem_b queuesize() const {return _pkt_count;}
    virtual mem_b maxsize() const {return 0;}
    inline simtime_picosec drainTime(Packet *pkt) { 
        return (simtime_picosec)(pkt->size() * _ps_per_byte); 
    }
    void add_src(int32_t flowid, ConstScheduledSrc* src);
    int src_queuesize(int32_t flowid) {return _queue_counts[flowid];}
    map <flowid_t, int32_t> _queue_counts; 
 protected:
    uint32_t _pkt_count;
     // map from flow id to packet count
    map <flowid_t, ConstScheduledSrc*> _srcs; // map from flow id to SubflowSrc for callbacks
};

class ConstFifoScheduler : public ConstBaseScheduler {
 public:
    ConstFifoScheduler(linkspeed_bps bitrate, EventList &eventlist, QueueLogger* logger);
    virtual void enqueue(Packet& pkt);
    virtual Packet* next_packet();
    virtual Packet* dequeue();
 protected:
    list<Packet*> _queue;
};


class ConstFairScheduler : public ConstBaseScheduler{
 public:
    ConstFairScheduler(linkspeed_bps bitrate, EventList &eventlist, QueueLogger* logger);
    virtual void enqueue(Packet& pkt);
    virtual Packet* next_packet();
    virtual Packet* dequeue();
 protected:
    map<flowid_t, list<Packet*>*> _queue_map;  // map flow id to pull queue
    Packet *_next_packet; // when we start dequeuing a packet, it must
                          // not be able to change, so we store it in
                          // _next_packet rather than the map
    bool queue_exists(const Packet& pkt);
    list<Packet*>* find_queue(const Packet& pkt);
    list<Packet*>* create_queue(const Packet& pkt);
    map<flowid_t, list<Packet*>*>::iterator _current_queue;
};

#endif
