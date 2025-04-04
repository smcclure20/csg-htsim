// -*- c-basic-offset: 4; indent-tabs-mode: nil -*- 
#ifndef CONSTANTCCAO_H
#define CONSTANTCCAO_H

/*
 * A source that sends at a constant sending rate
 */

#include <list>
#include <set>
#include <deque>
#include "config.h"
#include "network.h"
#include "eventlist.h"
#include "sent_packets.h"
#include "constant_cca_packet.h"
#include "constant_cca_scheduler.h"
#include "ecn.h"

//#define MODEL_RECEIVE_WINDOW 1

#define timeInf 0

class ConstantCcaOSrc;
class ConstantCcaOSink;
class ConstantCcaORtxTimerScanner;
class ConstBaseScheduler;

class ConstantCcaOPacer : public EventSource {
public:
    ConstantCcaOPacer(ConstantCcaOSrc& src, EventList& eventlist, simtime_picosec interpacket_delay);
    bool is_pending() const {return _interpacket_delay > 0;}  // are we pacing?
    void schedule_send(simtime_picosec delay);  // schedule a paced packet "delay" picoseconds after the last packet was sent
    void cancel();     // cancel pacing
    void just_sent();  // called when we've just sent a packet, even if it wasn't paced
    void doNextEvent();
    bool allow_send();
private:
    ConstantCcaOSrc* _src;
    simtime_picosec _interpacket_delay; // the interpacket delay, or zero if we're not pacing
    simtime_picosec _last_send;  // when the last packet was sent (always set, even when we're not pacing)
    simtime_picosec _next_send;  // when the next scheduled packet should be sent
};


class ConstantCcaOSrc : public EventSource, public PacketSink, public ConstScheduledSrc {
    friend class ConstantCcaOSink;
    friend class ConstantCcaORtxTimerScanner;
public:
    ConstantCcaOSrc(ConstantCcaORtxTimerScanner& rtx_scanner, EventList &eventlist, uint32_t addr, simtime_picosec pacing_delay, TrafficLogger* pkt_logger);
    virtual const string& nodename() { return _nodename; }
    virtual void receivePacket(Packet& pkt);
    void update_rtt(simtime_picosec delay);
    void adjust_cwnd(simtime_picosec delay, ConstantCcaAck::seq_t ackno);
    void handle_ack(ConstantCcaAck::seq_t ackno);
    void move_path_flow_label();
    void move_path(bool permit_cycles = false);
    void reroute(const Route &route);
    void doNextEvent();
    void rtx_timer_hook(simtime_picosec now, simtime_picosec period);
    inline simtime_picosec pacing_delay() const {return _pacing_delay;}
    PacketFlow& flow() {return _flow;}

    void set_pacing_delay(simtime_picosec delay) { _pacing_delay = delay; }

    bool send_next_packet();
    virtual void send_callback();  // called by scheduler when it has more space
    uint64_t last_acked() const {return _last_acked;}

    virtual void connect(ConstantCcaOSink& sink, simtime_picosec startTime, uint32_t destination, const Route& routeout, const Route& routein); // for packet switching
    void startflow();

    void set_flowsize(uint64_t flow_size_in_bytes) {
        _flow_size = flow_size_in_bytes + mss();
        cout << "Setting flow size to " << _flow_size << endl;
    }

    void set_stoptime(simtime_picosec stop_time) {
        _stop_time = stop_time;
        cout << "Setting stop time to " << timeAsSec(_stop_time) << endl;
    }

    bool more_data_available() const;

    bool check_stoptime();
    
    void set_cwnd(uint32_t cwnd);

    // add paths for PLB
    void enable_plb() {_plb = true;}
    void set_paths(vector<const Route*>* rt);
    void permute_paths();
    bool _plb;
    inline bool plb() const {return _plb;}
    void set_plb_threshold_ecn(int threshold) { _plb_threshold_ecn = threshold; }

    // packet switching
    bool _spraying;
    void set_spraying() {_spraying = true;}
    inline bool spraying() const {return _spraying;}


    // should really be private, but loggers want to see:
    uint64_t _flow_size;
    simtime_picosec _stop_time;
    bool _stopped;
    uint32_t _maxcwnd;
    int32_t _app_limited;
    uint32_t drops() {return _drops;};

    uint32_t rtos() {return _rto_count;};
    uint32_t total_dupacks() {return _total_dupacks;};

    // stats
    simtime_picosec _completion_time;
    simtime_picosec _start_time;

    uint16_t _mss;
    inline uint16_t mss() const {return _mss;}

    // paths for PLB or MPSwift
    vector<const Route*> _paths;

    ConstantCcaOSink* _sink;
    uint32_t _destination;
    uint32_t _addr;
    void set_app_limit(int pktps);

    int queuesize(uint32_t flow_id);

    uint64_t _highest_sent;  //seqno is in bytes
    uint64_t _packets_sent;

    int send_packets();

    void disable_fast_recovery() {_fr_disabled = true;}

    int _dupack_threshold = 3;
    void set_dupack_thresh(int threshold) {_dupack_threshold = threshold;}

protected:
    // connection state
    bool _established;

    //round trip time estimate
    simtime_picosec _rtt, _rto, _min_rto, _mdev;
    simtime_picosec _min_rtt, _max_rtt;

    // stuff needed for reno-like fast recovery
    uint32_t _inflate; // how much we're currently extending cwnd based off dup ack arrivals.
    uint64_t _recoverq;
    bool _in_fast_recovery;

    bool _fr_disabled; // fast recovery disabled

    uint32_t _min_cwnd;
    uint32_t _max_cwnd;

    uint32_t _const_cwnd;  // congestion window controlled by CCA
    uint32_t _prev_cwnd;
    uint64_t _last_acked; // ack number of the last packet we received a cumulative ack for
    uint16_t _dupacks;
    uint32_t _retransmit_cnt;
    uint32_t _rtx_reset_threshold; // constant

    bool _can_decrease;  // limit backoff to once per RTT
    simtime_picosec _last_decrease; //when we last decreased
    simtime_picosec _pacing_delay;  // inter-packet pacing when cwnd < 1 pkt.

    // PLB stuff
    int _decrease_count;
    simtime_picosec _last_good_path;
    std::deque<bool> _ecn_marks;
    int _plb_threshold_ecn;

    uint32_t _drops;
    simtime_picosec _RFC2988_RTO_timeout;
    bool _rtx_timeout_pending;
    uint32_t _rto_count;
    uint32_t _total_dupacks;

    // Flow stuff
    uint32_t _pathid;

    // Connectivity
    PacketFlow _flow;
    uint32_t _path_index;
    const Route* _route;
    ConstantCcaOPacer _pacer;

    // Account for NACK-based retransmissions when pacing 
    std::set<uint64_t> _pending_retransmit;
    bool deferred_retransmit;

    uint32_t _repeated_nack_rtxs;
    simtime_picosec _nack_backoff;

private:
    void retransmit_packet();
    void retransmit_packet(uint64_t seqno);
    bool _deferred_send;  // set if we tried to send and the scheduler said no.
    simtime_picosec _plb_interval;
    string _nodename;

    // Housekeeping
    ConstBaseScheduler* _scheduler;
    ConstantCcaORtxTimerScanner* _rtx_timer_scanner;

    // Mechanism
    void clear_timer(uint64_t start,uint64_t end);

    std::vector<uint32_t> _acks_received;
    uint32_t _nack_rtxs;
    std::vector<simtime_picosec> _rto_times;
};


/**********************************************************************************/
/** SINK                                                                 **/
/**********************************************************************************/

class ConstantCcaOSink : public PacketSink, public DataReceiver {
    friend class ConstantCcaOSrc;
public:
    ConstantCcaOSink();

    void receivePacket(Packet& pkt);
    ConstantCcaAck::seq_t _cumulative_ack; // seqno of the last byte in the packet we have
    uint64_t cumulative_ack();
    uint32_t _drops;
    uint32_t drops() {return _drops;}

    // cumulatively acked
    uint64_t _packets;

    // stats
    uint32_t _spurious_retransmits;
    uint32_t _nacks_sent;

    set<ConstantCcaAck::seq_t> _received; 
    virtual const string& nodename() { return _nodename; }


    ConstantCcaOSrc* _src;

    uint32_t spurious_retransmits();

private:
    // Connectivity
    void connect(ConstantCcaOSrc& src, const Route& route);
    const Route* _route;
    string _nodename;

    // Mechanism
    void send_ack(simtime_picosec ts, uint32_t ack_dst, uint32_t ack_src, uint32_t pathid);
    void send_nack(simtime_picosec ts, uint32_t ack_dst, uint32_t ack_src, uint32_t pathid, uint64_t seqno);
};

class ConstantCcaORtxTimerScanner : public EventSource {
public:
    ConstantCcaORtxTimerScanner(simtime_picosec scanPeriod, EventList& eventlist);
    void doNextEvent();
    void addSrc(ConstantCcaOSrc* src) { _srcs.push_back(src); }
private:
    simtime_picosec _scanPeriod;
    std::vector<ConstantCcaOSrc*> _srcs;
};

#endif