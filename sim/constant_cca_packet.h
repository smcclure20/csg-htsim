// -*- c-basic-offset: 4; indent-tabs-mode: nil -*-        
#ifndef CONSTCCAPACKET_H
#define CONSTCCAPACKET_H

#include <list>
#include "network.h"



// SwiftPacket and SwiftAck are subclasses of Packet.
// They incorporate a packet database, to reuse packet objects that are no longer needed.
// Note: you never construct a new SwiftPacket or SwiftAck directly; 
// rather you use the static method newpkt() which knows to reuse old packets from the database.

class ConstantCcaPacket : public Packet {
public:
    typedef uint64_t seq_t;

    inline static ConstantCcaPacket* newpkt(PacketFlow &flow, const Route &route, 
                                      seq_t seqno, int size, uint32_t dst, uint32_t src, uint32_t pathid) {
        ConstantCcaPacket* p = _packetdb.allocPacket();
        p->set_route(flow,route,size,seqno+size-1); // The Swift sequence number is the first byte of the packet; I will ID the packet by its last byte.
        p->_type = CONSTCCA;
        p->_seqno = seqno;
        p->_syn = false;
        p->set_dst(dst);
        p->set_src(src);
        p->_pathid = pathid;
        p->_direction = NONE;
        return p;
    }

    inline static ConstantCcaPacket* new_syn_pkt(PacketFlow &flow, const Route &route, 
                                           seq_t seqno, int size, uint32_t dst, uint32_t src, uint32_t pathid) {
        ConstantCcaPacket* p = newpkt(flow,route,seqno,size,dst,src,pathid);
        p->_syn = true;
        return p;
    }

    void free() {_packetdb.freePacket(this);}
    virtual ~ConstantCcaPacket(){}
    inline seq_t seqno() const {return _seqno;}
    inline simtime_picosec ts() const {return _ts;}
    inline void set_ts(simtime_picosec ts) {_ts = ts;}
    virtual PktPriority priority() const {return Packet::PRIO_LO;}  // change this if you want to use swift with priority queues

protected:
    seq_t _seqno;
    bool _syn;
    simtime_picosec _ts;
    static PacketDB<ConstantCcaPacket> _packetdb;
};

class ConstantCcaAck : public Packet {
public:
    typedef ConstantCcaPacket::seq_t seq_t;

    inline static ConstantCcaAck* newpkt(PacketFlow &flow, const Route &route, 
                                   seq_t seqno, seq_t ackno,
                                   simtime_picosec ts_echo, uint32_t dst, uint32_t src, uint32_t pathid) {
        ConstantCcaAck* p = _packetdb.allocPacket();
        p->set_route(flow,route,ACKSIZE,ackno);
        p->_type = CONSTCCAACK;
        p->_seqno = seqno;
        p->_ackno = ackno;
        p->_ts_echo = ts_echo;
        p->set_src(src);
        p->set_dst(dst);
        p->_pathid = pathid;
        p->_direction = NONE;
        return p;
    }

    void free() {_packetdb.freePacket(this);}
    inline seq_t seqno() const {return _seqno;}
    inline seq_t ackno() const {return _ackno;}
    inline seq_t ds_ackno() const {return _ds_ackno;}
    inline simtime_picosec ts_echo() const {return _ts_echo;}
    virtual PktPriority priority() const {return Packet::PRIO_HI;}

    virtual ~ConstantCcaAck(){}
    const static int ACKSIZE=40;
protected:
    seq_t _seqno;
    seq_t _ackno;
    seq_t _ds_ackno;
    simtime_picosec _ts_echo;
    static PacketDB<ConstantCcaAck> _packetdb;
};

#endif
