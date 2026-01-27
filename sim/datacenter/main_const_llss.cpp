// -*- c-basic-offset: 4; indent-tabs-mode: nil -*-        
#include "../config.h"
#include <sstream>

#include <iostream>
#include <string.h>
#include <math.h>
#include "../network.h"
#include "../randomqueue.h"
#include "shortflows.h"
#include "../pipe.h"
#include "../eventlist.h" 
#include "../logfile.h"
#include "../loggers.h"
#include "../clock.h"
#include "../constant_cca.h"
#include "../compositequeue.h"
//#include "firstfit.h"
#include "topology.h"
#include "connection_matrix.h"
//#include "vl2_topology.h"

#include "fat_tree_topology_drb.h"
#include "failure_manager_drb.h"
//#include "generic_topology.h"
//#include "oversubscribed_fat_tree_topology.h"
//#include "multihomed_fat_tree_topology.h"
//#include "star_topology.h"
//#include "bcube_topology.h"
#include <list>

// Simulation params

#define PRINT_PATHS 0

#define PERIODIC 0
#include "main.h"

uint32_t RTT = 1; // this is per link delay in us; identical RTT microseconds = 0.001 ms
#define DEFAULT_NODES 128
#define DEFAULT_QUEUE_SIZE 8

//FirstFit* ff = NULL;
//uint32_t subflow_count = 1;

//#define SWITCH_BUFFER (SERVICE * RTT / 1000)
#define USE_FIRST_FIT 0
#define FIRST_FIT_INTERVAL 100

extern void set_enable_bgp(bool b);

enum NetRouteStrategy {SOURCE_ROUTE= 0, ECMP = 1, ADAPTIVE_ROUTING = 2, ECMP_ADAPTIVE = 3, RR = 4, RR_ECMP = 5};

enum HostLBStrategy {NOLB = 0, SPRAY = 1, PLB = 2, SPRAY_ADAPTIVE = 3};

EventList eventlist;


void exit_error(char* progr) {
    cout << "Usage " << progr << " [UNCOUPLED(DEFAULT)|COUPLED_INC|FULLY_COUPLED|COUPLED_EPSILON] [epsilon][COUPLED_SCALABLE_TCP" << endl;
    exit(1);
}

int main(int argc, char **argv) {
    Clock c(timeFromSec(5 / 100.), eventlist);
    uint32_t cwnd = 15, no_of_nodes = DEFAULT_NODES;
    mem_b queuesize = DEFAULT_QUEUE_SIZE;
    linkspeed_bps linkspeed = speedFromMbps((double)HOST_NIC);
    stringstream filename(ios_base::out);
    stringstream flowfilename(ios_base::out);
    uint32_t packet_size = 4178;
    uint32_t no_of_subflows = 1;
    simtime_picosec tput_sample_time = timeFromUs((uint32_t)12);
    simtime_picosec endtime = timeFromMs(1.2);
    char* tm_file = NULL;
    char* topo_file = NULL;
    NetRouteStrategy route_strategy = SOURCE_ROUTE;
    HostLBStrategy host_lb = NOLB;
    int link_failures = 0;
    double failure_pct = 0.1; // failed links have 10% bandwidth
    FailType failure_type = BLACK_HOLE_DROP;
    bool binary_failure = false;
    int binary_failures = 0;
    simtime_picosec fail_time = timeInf;
    simtime_picosec route_recovery_time = timeInf;
    simtime_picosec weight_recovery_time = timeInf;
    std::vector<FailureManager*> failure_managers = {};
    int plb_ecn = 0;
    queue_type queue_type = ECN;
    double rate_coef = 1.0;
    double ecn_thres = 0.6;
    bool rts = false;
    int flaky_links = 0;
    simtime_picosec latency = 0;
    bool disable_fr = false;
    bool sack = false;
    int ooo_limit = 0;
    int dupack_thresh = 3;
    bool get_pkt_delay = false;

    int i = 1;
    filename << "None";
    flowfilename << "flowlog.csv";

    while (i<argc) {
        if (!strcmp(argv[i],"-o")){
            filename.str(std::string());
            filename << argv[i+1] << ".out";
            flowfilename.str(std::string());
            flowfilename << argv[i+1] << "-flows.csv";
            i++;
        } else if (!strcmp(argv[i],"-of")){
            flowfilename.str(std::string());
            flowfilename << argv[i+1];
            i++;
        } else if (!strcmp(argv[i],"-nodes")){
            no_of_nodes = atoi(argv[i+1]);
            i++;
        } else if (!strcmp(argv[i],"-tm")){
            tm_file = argv[i+1];
            cout << "traffic matrix input file: "<< tm_file << endl;
            i++;
        } else if (!strcmp(argv[i],"-topo")){
            topo_file = argv[i+1];
            cout << "topology input file: "<< topo_file << endl;
            i++;
        } else if (!strcmp(argv[i],"-cwnd")){
            cwnd = atoi(argv[i+1]);
            i++;
        } else if (!strcmp(argv[i],"-linkspeed")){
            // linkspeed specified is in Mbps
            linkspeed = speedFromMbps(atof(argv[i+1]));
            i++;
        } else if (!strcmp(argv[i],"-end")){
            endtime = timeFromUs(atof(argv[i+1]));
            i++;
        } else if (!strcmp(argv[i],"-q")){
            queuesize = atoi(argv[i+1]);
            i++;
        } else if (!strcmp(argv[i],"-mtu")){
            packet_size = atoi(argv[i+1]);
            i++;
        } else if (!strcmp(argv[i],"-subflows")){
            no_of_subflows = atoi(argv[i+1]);
            i++;
        } else if (!strcmp(argv[i],"-fails")){
            link_failures = atoi(argv[i+1]);
            i++;
        } else if (!strcmp(argv[i],"-failpct")){
            failure_pct = stod(argv[i+1]);
            i++;
        }   else if (!strcmp(argv[i],"-ftype")){
            if (!strcmp(argv[i+1], "drop")) {
                failure_type = BLACK_HOLE_DROP;
            }else if (!strcmp(argv[i+1], "queue")) {
                failure_type = BLACK_HOLE_QUEUE;
            } else {
                exit_error(argv[0]);
            }
            i++;
        }  else if (!strcmp(argv[i],"-ftime")){
            fail_time = timeFromUs(atof(argv[i+1]));
            route_recovery_time = timeFromUs(atof(argv[i+2]));
            weight_recovery_time = timeFromUs(atof(argv[i+3]));
            i+=3;
        } else if (!strcmp(argv[i],"-plbecn")){
            plb_ecn = atoi(argv[i+1]);
            i++;
        }  else if (!strcmp(argv[i],"-ecnthres")){
            ecn_thres = atoi(argv[i+1]);
            FatTreeSwitchDRB::set_ecn_threshold(ecn_thres);
            i++;
        } else if (!strcmp(argv[i],"-trim")){
            queue_type = COMPOSITE;
        } else if (!strcmp(argv[i],"-trimces")){
            queue_type = COMPOSITE_ECN;
        } else if (!strcmp(argv[i],"-trimced")){
            queue_type = COMPOSITE_ECN_DEF;
        } else if (!strcmp(argv[i],"-trimcel")){
            queue_type = COMPOSITE_ECN_LB;
        } else if (!strcmp(argv[i],"-trimcomp")){
            queue_type = ECN_BIG;
        } else if (!strcmp(argv[i],"-rts")){
            rts = true;
        } else if (!strcmp(argv[i],"-flakylinks")){
            flaky_links = atoi(argv[i+1]);
            i++;
        } else if (!strcmp(argv[i],"-lat")){
            latency = timeFromUs(stod(argv[i+1]));
            i++;
        } else if (!strcmp(argv[i],"-nofr")){
            disable_fr = true;
        } else if (!strcmp(argv[i],"-sack")){
            sack = true;
            ooo_limit = atoi(argv[i+1]);
            i++;
        } else if (!strcmp(argv[i],"-dup")){
            dupack_thresh = atoi(argv[i+1]);
            i++;
        }  else if (!strcmp(argv[i],"-pktdelay")){
            get_pkt_delay = true;
        } else if (!strcmp(argv[i],"-tsample")){
            tput_sample_time = timeFromUs((uint32_t)atoi(argv[i+1]));
            i++;            
        } else if (!strcmp(argv[i],"-ratecoef")){
            rate_coef = stod(argv[i+1]);
            i++;
        } else if (!strcmp(argv[i],"-hostlb")){
            if (!strcmp(argv[i+1], "spray")) {
                host_lb = SPRAY;
            } else if (!strcmp(argv[i+1], "plb")) {
                host_lb = PLB;
            } else if (!strcmp(argv[i+1], "sprayad")) {
                host_lb = SPRAY_ADAPTIVE;
            } else {
                exit_error(argv[0]);
            }
            i++;
        } else if (!strcmp(argv[i],"-strat")){
            if (!strcmp(argv[i+1], "ecmp")) {
                FatTreeSwitchDRB::set_strategy(FatTreeSwitchDRB::ECMP);
                route_strategy = ECMP;
            } else if (!strcmp(argv[i+1], "pkt_ar")) {
                FatTreeSwitchDRB::set_strategy(FatTreeSwitchDRB::ADAPTIVE_ROUTING);
                route_strategy = ADAPTIVE_ROUTING;
            } else if (!strcmp(argv[i+1], "fl_ar")) {
                FatTreeSwitchDRB::set_strategy(FatTreeSwitchDRB::ADAPTIVE_ROUTING);
                FatTreeSwitchDRB::set_ar_sticky(FatTreeSwitchDRB::PER_FLOWLET);
                route_strategy = ADAPTIVE_ROUTING;
            } else if (!strcmp(argv[i+1], "ecmp_ar")) {
                FatTreeSwitchDRB::set_strategy(FatTreeSwitchDRB::ECMP_ADAPTIVE);
                route_strategy = ECMP_ADAPTIVE;
            } else if (!strcmp(argv[i+1], "rr")) {
                FatTreeSwitchDRB::set_strategy(FatTreeSwitchDRB::RR);
                route_strategy = RR;
            } else if (!strcmp(argv[i+1], "rr_ecmp")) {
                FatTreeSwitchDRB::set_strategy(FatTreeSwitchDRB::RR_ECMP);
                route_strategy = RR_ECMP;
            } else if (!strcmp(argv[i+1], "llss")) {
                FatTreeSwitchDRB::set_strategy(FatTreeSwitchDRB::PER_POD_IWRR); 
                route_strategy = ECMP; // Use ECMP enum for main logic as it's just a switch strategy
            } else if (!strcmp(argv[i+1], "llss_perm")) {
                FatTreeSwitchDRB::set_strategy(FatTreeSwitchDRB::PER_POD_IWRR_PERMUTE); 
                route_strategy = ECMP; // Use ECMP enum for main logic as it's just a switch strategy
            } else if (!strcmp(argv[i+1], "fib_llss")) {
                FatTreeSwitchDRB::set_strategy(FatTreeSwitchDRB::FIB_LLSS);
                route_strategy = ECMP;
            } else if (!strcmp(argv[i+1], "fib_llss_perm")) {
                FatTreeSwitchDRB::set_strategy(FatTreeSwitchDRB::FIB_LLSS_PERMUTE);
                route_strategy = ECMP;
            }  else {
                exit_error(argv[0]);
            }
            i++;
        } else {
            exit_error(argv[i]);
        }
        i++;
    }
    Packet::set_packet_size(packet_size);
    eventlist.setEndtime(endtime);

    queuesize = queuesize*Packet::data_packet_size();
    srand(time(NULL));
    srandom(time(NULL));
      
    cout << "requested nodes " << no_of_nodes << endl;
    cout << "cwnd " << cwnd << endl;
    cout << "mtu " << packet_size << endl;
    cout << "hoststrat " << host_lb << endl;
    cout << "strategy " << route_strategy << endl;
    cout << "subflows " << no_of_subflows << endl;
    cout << "pkt size " << Packet::data_packet_size() << endl;

    if (host_lb == PLB && queue_type == COMPOSITE) {
        cout << "PLB and composite queueing not supported (for now)" << endl;
        exit(1);
    }

    if (failure_pct == -1.0) {  // signifies failure that is routed around (vs. still in route tables with 0.0)
        binary_failure = true; 
        binary_failures= link_failures;
        link_failures = 0;
    }
      
    
    // Log of per-flow stats
    cout << "Logging flows to " << flowfilename.str() << endl;  
    std::ofstream flowlog(flowfilename.str().c_str());
    if (!flowlog){
        cout << "Can't open for writing flow log file!"<<endl;
        exit(1);
    }

    
    ConstantCcaSrc* sender;
    ConstantCcaSink* sink;

    Route* routeout, *routein;

    ConstantCcaRtxTimerScanner rtxScanner(timeFromUs(0.01), eventlist);
   

    // TODO: need to update failure stuff to match const erase
    FatTreeTopologyDRB* top = new FatTreeTopologyDRB(no_of_nodes, linkspeed, queuesize, 
                                               NULL, &eventlist, NULL, queue_type, CONST_SCHEDULER, link_failures, failure_pct, rts, latency, flaky_links, timeFromUs(100.0), timeFromUs(10.0), false);
    // if (flaky_links > 0) {
    //     top->set_flaky_links(flaky_links, timeFromUs(100.0), timeFromUs(10.0)); // todo: parameterize this
    // }
    if (binary_failure) {
        for (int i=0; i< binary_failures; i++){
            failure_managers.push_back(new FailureManager(top, eventlist, fail_time, route_recovery_time, weight_recovery_time, failure_type));
            failure_managers[i]->setFailedLink(FatTreeSwitchDRB::AGG, (int)(i / (top->no_of_pods()/2)), i%(top->no_of_pods()/2)); // TODO: move this into the failure manager rather than here
        }
    }


    
    no_of_nodes = top->no_of_nodes();
    cout << "actual nodes " << no_of_nodes << endl;

    // Permutation connections
    ConnectionMatrix* conns = new ConnectionMatrix(no_of_nodes);

    if (tm_file){
        cout << "Loading connection matrix from  " << tm_file << endl;

        if (!conns->load(tm_file))
            exit(-1);
    }
    else {
        cout << "Loading connection matrix from standard input" << endl;        
        conns->load(cin);
    }

    if (conns->N != no_of_nodes){
        cout << "Connection matrix number of nodes is " << conns->N << " while I am using " << no_of_nodes << endl;
        exit(-1);
    }
    
    vector<connection*>* all_conns;

    // used just to print out stats data at the end
    list <const Route*> routes;
    
    list <ConstantCcaSrc*> srcs;
    list <ConstantCcaSink*> sinks;
    // initialize all sources/sinks

    uint32_t connID = 0;
    all_conns = conns->getAllConnections();
    uint32_t connCount = all_conns->size();

    for (uint32_t c = 0; c < all_conns->size(); c++){
        connection* crt = all_conns->at(c);
        uint32_t src = crt->src;
        uint32_t dest = crt->dst;
        if (src == dest) {
            cout << "Self-loop: " << src << endl;
            exit(1);
        }
        
        connID++;
        // TODO: set the number of subflows here (if -subflows == 0) such that the rate  is low enough for a certain dupack threshold
        if (no_of_subflows == 0) {
            int num_queues = 4;
            // int num_queues = net_paths[src][dest]->at(0)->hop_count() - 2;
            // int needed_competing_flows;
            // if (num_queues == 0) {
            //     needed_competing_flows = 1;
            // } else{
            int needed_competing_flows = ceil((num_queues * 8) / dupack_thresh);
            // }
            int flows_per_link = int((double)all_conns->size() / no_of_nodes);
            no_of_subflows = std::max(1, (int)ceil((needed_competing_flows)/flows_per_link));
            cout << "Setting number of subflows to " << no_of_subflows << endl;
        }

        double rate = (linkspeed / ((double)all_conns->size() / no_of_nodes)) / (packet_size * 8); // assumes all nodes have the same number of connections
        simtime_picosec interpacket_delay = timeFromSec(1. / (rate * rate_coef)); //+ rand() % (2*(no_of_nodes-1)); // just to keep them not perfectly in sync
        sender = new ConstantCcaSrc(rtxScanner, eventlist, src, interpacket_delay, NULL);  

        if (disable_fr) {
            sender->disable_fast_recovery(); // no fast recovery when we have packet trimming
        }
        if (sack) {
            sender->set_sack();
            sender->set_ooo_limit(ooo_limit);
        }
                        
        if (crt->size>0){
            sender->set_flowsize(crt->size);
        } 
                
        if (host_lb == PLB) {
            sender->enable_plb();
            sender->set_plb_threshold_ecn(plb_ecn);
        } else if (host_lb == SPRAY) {
            sender->set_spraying();
        } else if (host_lb == SPRAY_ADAPTIVE) {
            sender->set_spraying();
            sender->set_adaptive();
        }
        if (dupack_thresh != 3) {
            // int k = (4*no_of_nodes )^ (1/3);
            // if (src % (k/2) ==  dst % (k/2)) { // TODO: make this depend on how many hops away the two are
            //     sender->set_dupack_thresh();
            // } else {
            //     sender->set_dupack_thresh(dupack_thresh);
            // }
            sender->set_dupack_thresh(dupack_thresh);
        }
        srcs.push_back(sender);
        sink = new ConstantCcaSink();
        sinks.push_back(sink);

        sender->setName("constcca_" + ntoa(src) + "_" + ntoa(dest));

        sink->setName("constcca_sink_" + ntoa(src) + "_" + ntoa(dest));

          
        if (route_strategy != SOURCE_ROUTE) {
            routeout = top->get_tor_route(src);
            routein = top->get_tor_route(dest);
        } else {
            printf("Source routing not currently supported");
            exit(1);
        }

        sender->connect(*sink, (uint32_t)crt->start + rand()%(interpacket_delay), no_of_subflows, dest, *routeout, *routein);
        sender->set_cwnd(cwnd*Packet::data_packet_size());
        // sender->set_paths(net_paths[src][dest]);

        if (route_strategy != SOURCE_ROUTE) {
            for (int i = 0; i < no_of_subflows; i++) {
                ConstantCcaSubflowSrc* sub = sender->subflows()[i];
                ConstantCcaSubflowSink* sub_sink = sink->_subs[i];
                top->add_host_port(src, sub->flow().flow_id(), sub);
                top->add_host_port(dest, sub->flow().flow_id(), sub_sink);
                // cout << "Added subflow " << sub->flow().flow_id() << " from " << src << " to " << dest << endl;
            }
        }
    }
    //    ShortFlows* sf = new ShortFlows(2560, eventlist, net_paths,conns,lg, &swiftRtxScanner);

    cout << "Loaded " << connID << " connections in total\n";
    simtime_picosec thres = timeFromUs(2000.0);

    // GO!
    cout << "Starting simulation" << endl;
    simtime_picosec checkpoint = timeFromUs(100.0);
    while (eventlist.doNextEvent()) {
        if (eventlist.now() > checkpoint) {
            simtime_picosec now = eventlist.now();
            
            cout << "Simulation time " << timeAsUs(eventlist.now()) << endl;
            checkpoint += timeFromUs(100.0);
            if (endtime == 0) {
                // Iterate through sources to see if they have completed the flows
                bool all_done = true;
                list <ConstantCcaSrc*>::iterator src_i;
                for (src_i = srcs.begin(); src_i != srcs.end(); src_i++) {
                    if ((*src_i)->highest_dsn_ack() < (*src_i)->_flow_size) {
                        all_done = false;
                        break;
                    }
                }
                if (all_done) {
                    cout << "All flows completed" << endl;
                    break;
                }
            }
        }
    }

    cout << "Done" << endl;


    /*for (uint32_t i = 0; i < 10; i++)
        cout << "Hop " << i << " Count " << counts[i] << endl; */
    list <ConstantCcaSrc*>::iterator src_i;
    // for (src_i = swift_srcs.begin(); src_i != swift_srcs.end(); src_i++) {
    //     cout << "Src, sent: " << (*src_i)->_highest_dsn_sent << "[rtx: " << (*src_i)->_subs[0]. << "] nacks: " << (*src_i)->_nacks_received << " pulls: " << (*src_i)->_pulls_received << " paths: " << (*src_i)->_paths.size() << endl;
    // }
    if (get_pkt_delay) {
        flowlog << "Flow ID,Drops,Spurious Retransmits,Completion Time,RTOs,ReceivedBytes,NACKs,DupACKs,PacketsSent,SACKRTXs,PktDelay" << endl;
        for (src_i = srcs.begin(); src_i != srcs.end(); src_i++) {
            ConstantCcaSink* sink = (*src_i)->_sink;
            simtime_picosec time = (*src_i)->_completion_time > 0 ? (*src_i)->_completion_time - (*src_i)->_start_time: 0;
            flowlog << (*src_i)->_addr << "-" << (*src_i)->_destination << "," << (*src_i)->drops() << "," << sink->spurious_retransmits() << "," <<
                time << "," << (*src_i)->rtos() << "," << sink->cumulative_ack() << "," << sink->nacks_sent() << "," << (*src_i)->total_dupacks() << "," << 
                (*src_i)->packets_sent()  << "," << (*src_i)->sack_rtxs() << "," << (*src_i)->pkt_delay() << endl;
        }
    } else {
        flowlog << "Flow ID,Drops,Spurious Retransmits,Completion Time,RTOs,ReceivedBytes,NACKs,DupACKs,PacketsSent,SACKRTXs" << endl;
        for (src_i = srcs.begin(); src_i != srcs.end(); src_i++) {
            ConstantCcaSink* sink = (*src_i)->_sink;
            simtime_picosec time = (*src_i)->_completion_time > 0 ? (*src_i)->_completion_time - (*src_i)->_start_time: 0;
            flowlog << (*src_i)->_addr << "-" << (*src_i)->_destination << "," << (*src_i)->drops() << "," << sink->spurious_retransmits() << "," <<
                time << "," << (*src_i)->rtos() << "," << sink->cumulative_ack() << "," << sink->nacks_sent() << "," << (*src_i)->total_dupacks() << "," << 
                (*src_i)->packets_sent()  << "," << (*src_i)->sack_rtxs()<< endl;
        }
    }   
    flowlog.close();
    /*
    uint64_t total_rtt = 0;
    cout << "RTT Histogram";
    for (uint32_t i = 0; i < 100000; i++) {
        if (SwiftSrc::_rtt_hist[i]!= 0) {
            cout << i << " " << SwiftSrc::_rtt_hist[i] << endl;
            total_rtt += SwiftSrc::_rtt_hist[i];
        }
    }
    cout << "RTT CDF";
    uint64_t cum_rtt = 0;
    for (uint32_t i = 0; i < 100000; i++) {
        if (SwiftSrc::_rtt_hist[i]!= 0) {
            cum_rtt += SwiftSrc::_rtt_hist[i];
            cout << i << " " << double(cum_rtt)/double(total_rtt) << endl;
        }
    }
    */
}
