// -*- c-basic-offset: 4; indent-tabs-mode: nil -*- 
#ifndef CONSTANTERASURECCA_H
#define CONSTANTERASURECCA_H

/*
 * A source that sends at a constant sending rate with erasure coding
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
#include "uec_mp.h"

#define timeInf 0

class ConstantErasureCcaSrc;
class ConstantErasureCcaSink;
class ConstErasureBaseScheduler;

class ConstantErasureCcaPacer : public EventSource {
public:
    ConstantErasureCcaPacer(ConstantErasureCcaSrc& src, EventList& eventlist, simtime_picosec interpacket_delay);
    bool is_pending() const {return _interpacket_delay > 0;}  // are we pacing?
    void schedule_send(simtime_picosec delay);  // schedule a paced packet "delay" picoseconds after the last packet was sent
    void cancel();     // cancel pacing
    void just_sent();  // called when we've just sent a packet, even if it wasn't paced
    void doNextEvent();
    bool allow_send();
private:
    ConstantErasureCcaSrc* _src;
    simtime_picosec _interpacket_delay; // the interpacket delay, or zero if we're not pacing
    simtime_picosec _last_send;  // when the last packet was sent (always set, even when we're not pacing)
    simtime_picosec _next_send;  // when the next scheduled packet should be sent
};


class ConstantErasureCcaSrc : public EventSource, public PacketSink, public ConstScheduledSrc {
    friend class ConstantErasureCcaSink;
public:
    ConstantErasureCcaSrc(EventList &eventlist, uint32_t addr, simtime_picosec pacing_delay, TrafficLogger* pkt_logger);
    virtual const string& nodename() { return _nodename; }
    virtual void receivePacket(Packet& pkt);
    void move_path_flow_label();
    void move_path(bool permit_cycles = false);
    void reroute(const Route &route);
    void doNextEvent();
    inline simtime_picosec pacing_delay() const {return _pacing_delay;}
    PacketFlow& flow() {return _flow;}

    void set_pacing_delay(simtime_picosec delay) { _pacing_delay = delay; }

    bool send_next_packet();
    virtual void send_callback();  // called by scheduler when it has more space

    virtual void connect(ConstantErasureCcaSink& sink, simtime_picosec startTime, uint32_t destination, const Route& routeout, const Route& routein); // for packet switching
    void startflow();

    void set_flowsize(uint64_t flow_size_in_bytes, int erasure_k) {
        _flow_size = flow_size_in_bytes + mss() + erasure_k * mss();
        // cout << "Setting flow size to " << _flow_size << endl;
    }

    void set_stoptime(simtime_picosec stop_time) { // can remove?
        _stop_time = stop_time;
        cout << "Setting stop time to " << timeAsSec(_stop_time) << endl;
    }

    bool is_complete();

    uint64_t packets_acked() {return _bytes_acked;}

    bool check_stoptime(); // can remove?
    
    void set_cwnd(uint32_t cwnd);

    // add paths for PLB
    void enable_plb() {_plb = true;}
    void set_paths(vector<const Route*>* rt);
    void permute_paths();
    bool _plb;
    inline bool plb() const {return _plb;}
    void set_plb_threshold_ecn(int threshold) { _plb_threshold_ecn = threshold; }
    // timeouts for plb
    simtime_picosec _timer_start;

    // packet switching
    bool _spraying;
    void set_spraying() {_spraying = true;}
    inline bool spraying() const {return _spraying;}
    bool _adaptive;
    void set_adaptive() {_adaptive = true;}
    inline bool adaptive() const {return _adaptive;}
    UecMpReps* _uec_mp;
    int _ev_count;
    // std::vector<double> _path_qualities; //indexed by path id, ewma of ecn marks or rtt or something
    // std::vector<bool> _path_skipped;
    std::map<uint32_t, simtime_picosec> _ev_timers;
    void check_ev_timers(simtime_picosec now);


    uint64_t _flow_size;
    inline uint64_t flow_size() const {return _flow_size;}
    simtime_picosec _stop_time;
    bool _stopped;
    int32_t _app_limited;
    uint64_t _packets_sent;
    uint64_t _current_seqno;
    uint64_t _approx_lost;

    // stats
    simtime_picosec _completion_time;
    simtime_picosec _start_time;
    simtime_picosec _total_pkt_delay;
    uint64_t _total_packets;

    simtime_picosec average_pkt_delay() {return _total_pkt_delay / _total_packets;}

    uint16_t _mss;
    inline uint16_t mss() const {return _mss;}

    // paths for PLB or MPSwift
    vector<const Route*> _paths;

    //round trip time estimate
    simtime_picosec _rtt, _rto, _min_rto, _mdev;
    void update_rtt(simtime_picosec delay);

    ConstantErasureCcaSink* _sink;
    uint32_t _destination;
    uint32_t _addr;
    void set_app_limit(int pktps);

    int queuesize(int flow_id);

    int send_packets();

protected:
    // connection state
    bool _established;

    uint64_t _bytes_acked;

    simtime_picosec _pacing_delay;  // inter-packet pacing when cwnd < 1 pkt.

    // PLB stuff
    int _decrease_count;
    simtime_picosec _last_good_path;
    std::deque<bool> _ecn_marks;
    int _plb_threshold_ecn;

    // Flow stuff
    uint32_t _pathid;

    // Connectivity
    PacketFlow _flow;
    uint32_t _path_index;
    const Route* _route;
    ConstantErasureCcaPacer _pacer;

private:
    bool _deferred_send;  // set if we tried to send and the scheduler said no.
    simtime_picosec _plb_interval;
    string _nodename;

    uint32_t _inflight;
    uint64_t _highest_ack;
    simtime_picosec _final_bdp_time;

    // Housekeeping
    ConstBaseScheduler* _scheduler;
};


/**********************************************************************************/
/** SINK                                                                 **/
/**********************************************************************************/

class ConstantErasureCcaSink : public PacketSink, public DataReceiver {
    friend class ConstantErasureCcaSrc;
public:
    ConstantErasureCcaSink();

    void receivePacket(Packet& pkt);

    uint64_t _bytes_received;
    uint64_t cumulative_ack(){return _bytes_received;}
    uint32_t drops() {return 0;}

    virtual const string& nodename() { return _nodename; }

    ConstantErasureCcaSrc* _src;

private:
    // Connectivity
    void connect(ConstantErasureCcaSrc& src, const Route& route);
    const Route* _route;
    string _nodename;
};

class ConstEraseRtxTimerScanner : public EventSource {
public:
    ConstEraseRtxTimerScanner(simtime_picosec scanPeriod, EventList& eventlist);
    void doNextEvent();
    void registerFlow(ConstantErasureCcaSrc* src);
private:
    simtime_picosec _scanPeriod;
    std::vector<ConstantErasureCcaSrc*> _srcs;
};

#endif
