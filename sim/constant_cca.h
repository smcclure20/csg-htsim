// -*- c-basic-offset: 4; indent-tabs-mode: nil -*- 
#ifndef CONSTANTCCA_H
#define CONSTANTCCA_H

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

class ConstantCcaSrc;
class ConstantCcaSink;
class ConstantCcaRtxTimerScanner;
class ConstBaseScheduler;

class ConstantCcaPacer : public EventSource {
public:
    ConstantCcaPacer(ConstantCcaSubflowSrc& src, EventList& eventlist, simtime_picosec interpacket_delay);
    bool is_pending() const {return _interpacket_delay > 0;}  // are we pacing?
    void schedule_send(simtime_picosec delay);  // schedule a paced packet "delay" picoseconds after the last packet was sent
    void cancel();     // cancel pacing
    void just_sent();  // called when we've just sent a packet, even if it wasn't paced
    void doNextEvent();
    bool allow_send();
private:
    ConstantCcaSubflowSrc* _src;
    simtime_picosec _interpacket_delay; // the interpacket delay, or zero if we're not pacing
    simtime_picosec _last_send;  // when the last packet was sent (always set, even when we're not pacing)
    simtime_picosec _next_send;  // when the next scheduled packet should be sent
};

class ConstantCcaSrc : public EventSource {
    friend class ConstantCcaSink;
    friend class ConstantCcaRtxTimerScanner;
public:
    ConstantCcaSrc(ConstantCcaRtxTimerScanner& rtx_scanner, EventList &eventlist, uint32_t addr, simtime_picosec pacing_delay, TrafficLogger* pkt_logger);
    virtual void connect(ConstantCcaSink& sink, simtime_picosec startTime, uint32_t no_of_subflows, uint32_t destination); 
    void startflow();

    void doNextEvent();
    void update_dsn_ack(ConstantCcaAck::seq_t ds_ackno);
    // virtual void receivePacket(Packet& pkt);

    void set_flowsize(uint64_t flow_size_in_bytes) {
        _flow_size = flow_size_in_bytes + mss();
        cout << "Setting flow size to " << _flow_size << endl;
    }

    void set_stoptime(simtime_picosec stop_time) {
        _stop_time = stop_time;
        cout << "Setting stop time to " << timeAsSec(_stop_time) << endl;
    }

    bool more_data_available() const;

    vector<ConstantCcaSubflowSrc*>& subflows() {return _subs;}

    ConstantCcaPacket::seq_t get_next_dsn() {
        ConstantCcaPacket::seq_t dsn = _highest_dsn_sent + 1;
        _highest_dsn_sent += mss();
        return dsn;
    }

    ConstantCcaPacket::seq_t highest_dsn_ack() {return _highest_dsn_ack;}

    bool check_stoptime();
    
    void set_cwnd(uint32_t cwnd);

    // add paths for PLB
    void enable_plb() {_plb = true;}
    void set_paths(vector<const Route*>* rt);
    void permute_paths();
    bool _plb;
    inline bool plb() const {return _plb;}
    uint16_t _plb_threshold_ecn;
    void set_plb_threshold_ecn(int threshold) { _plb_threshold_ecn = threshold; }

    // packet switching
    bool _spraying;
    void set_spraying() {_spraying = true;}
    inline bool spraying() const {return _spraying;}


    // should really be private, but loggers want to see:
    uint64_t _highest_dsn_sent;  //seqno is in bytes - data sequence number, for MPSwift
    uint64_t _flow_size;
    simtime_picosec _stop_time;
    bool _stopped;

    // stats
    simtime_picosec _completion_time;
    simtime_picosec _start_time;
    ConstantCcaPacket::seq_t _highest_dsn_ack;

    uint16_t _mss;
    inline uint16_t mss() const {return _mss;}

    // paths for PLB or MPSwift
    vector<const Route*> _paths;

    ConstantCcaSink* _sink;
    uint32_t _destination;
    uint32_t _addr;

    int queuesize(uint32_t flow_id);

    simtime_picosec _pacing_delay;
    void set_pacing_delay(simtime_picosec delay) { _pacing_delay = delay; }

    bool _fr_disabled = false;
    void disable_fast_recovery() {_fr_disabled = true;}

    int _dupack_threshold = 3;
    void set_dupack_thresh(int threshold) {_dupack_threshold = threshold;}

    uint32_t drops();
    uint32_t rtos();
    uint32_t total_dupacks();
    uint32_t packets_sent();

private:
    // Housekeeping
    TrafficLogger* _traffic_logger;
    ConstBaseScheduler* _scheduler;
    ConstantCcaRtxTimerScanner* _rtx_timer_scanner;

    // Mechanism
    void clear_timer(uint64_t start,uint64_t end);

    // list of subflows
    vector<ConstantCcaSubflowSrc*> _subs;
};



class ConstantCcaSubflowSrc : public EventSource, public PacketSink, public ConstScheduledSrc {
    friend class ConstantCcaSrc;
public:
    ConstantCcaSubflowSrc(ConstantCcaSrc& src, TrafficLogger* pktlogger, int subflow_id, simtime_picosec pacing_delay);
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

    virtual void connect(ConstantCcaSink& sink, const Route& routeout, const Route& routein, ConstBaseScheduler* scheduler); // for packet switching

    uint32_t _maxcwnd;
    uint32_t drops() {return _drops;};

    uint32_t rtos() {return _rto_count;};
    uint32_t total_dupacks() {return _total_dupacks;};
    uint32_t packets_sent() const { return _packets_sent; }

    ConstantCcaSubflowSink* _subflow_sink;

    uint64_t _highest_sent;  //seqno is in bytes
    uint64_t _packets_sent;

    int send_packets();

    uint16_t _mss;
    uint16_t mss() const {return _mss;}

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

    map <ConstantCcaPacket::seq_t, ConstantCcaPacket::seq_t> _dsn_map;  // map of subflow seqno to data seqno

    bool _fr_disabled; // fast recovery disabled

    uint32_t _min_cwnd;
    uint32_t _max_cwnd;

    uint32_t _const_cwnd;  // congestion window controlled by CCA
    uint32_t _prev_cwnd;
    uint64_t _last_acked; // ack number of the last packet we received a cumulative ack for
    uint16_t _dupacks;
    uint32_t _retransmit_cnt;
    uint32_t _rtx_reset_threshold; // constant

    simtime_picosec _pacing_delay;  

    // PLB stuff
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

    // Account for NACK-based retransmissions when pacing 
    std::set<uint64_t> _pending_retransmit;
    bool deferred_retransmit;

    uint32_t _repeated_nack_rtxs;
    simtime_picosec _nack_backoff;

    ConstantCcaSrc& _src;
    ConstantCcaPacer _pacer;

private:
    void retransmit_packet();
    void retransmit_packet(uint64_t seqno);
    bool _deferred_send;  // set if we tried to send and the scheduler said no.
    simtime_picosec _plb_interval;
    string _nodename;

    std::vector<uint32_t> _acks_received;
    uint32_t _nack_rtxs;
    std::vector<simtime_picosec> _rto_times;
};


/**********************************************************************************/
/** SINK                                                                 **/
/**********************************************************************************/

class ConstantCcaSink : public PacketSink, public DataReceiver {
    friend class ConstantCcaSrc;
    friend class ConstantCcaSubflowSrc;
public:
    ConstantCcaSink();

    void receivePacket(Packet& pkt);
    ConstantCcaAck::seq_t _cumulative_data_ack; // seqno of the last DSN byte in the packet we have
                                          // cumulatively acked
    virtual const string& nodename() { return _nodename; }

    set <ConstantCcaPacket::seq_t> _dsn_received; // use a map because multipath packets will arrive out of order

    ConstantCcaSrc* _src;
    uint64_t cumulative_ack() { return _cumulative_data_ack;};
    uint32_t drops();
    uint32_t spurious_retransmits();
    uint32_t nacks_sent();

    vector <ConstantCcaSubflowSink*> _subs; // public so the logger can see
private:
    // Connectivity, called by Src
    ConstantCcaSubflowSink* connect(ConstantCcaSrc& src, ConstantCcaSubflowSrc& subflow_src, const Route& route);
    string _nodename;
    ReorderBufferLogger* _buffer_logger;
};

class ConstantCcaSubflowSink : public PacketSink, public DataReceiver {
    friend class ConstantCcaSubflowSrc;
    friend class ConstantCcaSink;
public:
    ConstantCcaSubflowSink(ConstantCcaSink& sink);

    void receivePacket(Packet& pkt);
    ConstantCcaAck::seq_t _cumulative_ack; // seqno of the last byte in the packet we have
    uint64_t cumulative_ack() {return _cumulative_ack;}

    uint64_t _packets;

    // stats
    uint32_t _spurious_retransmits;
    inline uint32_t spurious_retransmits() const { return _spurious_retransmits; }
    uint32_t _drops;
    inline uint32_t drops() {return _drops;}
    uint32_t _nacks_sent;
    inline uint32_t nacks_sent() const { return _nacks_sent; }

    set<ConstantCcaAck::seq_t> _received; 
    virtual const string& nodename() { return _nodename; }


    ConstantCcaSubflowSrc* _subflow_src;
    ConstantCcaSink& _sink;

    uint32_t spurious_retransmits();

private:
    // Connectivity
    void connect(ConstantCcaSubflowSrc& src, const Route& route);
    const Route* _route;
    string _nodename;

    // Mechanism
    void send_ack(simtime_picosec ts, uint32_t ack_dst, uint32_t ack_src, uint32_t pathid);
    void send_nack(simtime_picosec ts, uint32_t ack_dst, uint32_t ack_src, uint32_t pathid, uint64_t seqno, uint64_t dsn);
};

class ConstantCcaRtxTimerScanner : public EventSource {
public:
    ConstantCcaRtxTimerScanner(simtime_picosec scanPeriod, EventList& eventlist);
    void doNextEvent();
    void registerSubflow(ConstantCcaSubflowSrc &subflow_src);
private:
    simtime_picosec _scanPeriod;
    std::vector<ConstantCcaSubflowSrc*> _srcs;
};

#endif
