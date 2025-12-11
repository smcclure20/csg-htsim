// -*- c-basic-offset: 4; indent-tabs-mode: nil -*-        
#include "config.h"
#include <sstream>

#include <iostream>
#include <string.h>
#include <math.h>
#include "network.h"
#include "randomqueue.h"
#include "shortflows.h"
#include "pipe.h"
#include "eventlist.h" 
#include "logfile.h"
#include "loggers.h"
#include "clock.h"
#include "constant_cca_erasure.h"
#include "compositequeue.h"
//#include "firstfit.h"
#include "topology.h"
#include "connection_matrix.h"
//#include "vl2_topology.h"

#include "fat_tree_topology.h"
#include "failure_manager.h"
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
    uint32_t packet_size = 4000;
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
    simtime_picosec fail_recover_time = timeInf;
    simtime_picosec route_return_time = timeInf;
    simtime_picosec weight_return_time = timeInf;
    std::vector<FailureManager*> failure_managers = {};
    int plb_ecn = 0;
    double ecn_thres = 0.6;
    queue_type queue_type = ECN;
    double rate_coef = 1.0;
    bool rts = false;
    int k = 3;
    int flaky_links = 0;
    simtime_picosec latency = 0;

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
        }  else if (!strcmp(argv[i],"-ftype")){
            if (!strcmp(argv[i+1], "drop")) {
                failure_type = BLACK_HOLE_DROP;
            }else if (!strcmp(argv[i+1], "queue")) {
                failure_type = BLACK_HOLE_QUEUE;
            } else {
                exit_error(argv[0]);
            }
            i++;
        } else if (!strcmp(argv[i],"-ftime")){
            fail_time = timeFromUs(atof(argv[i+1]));
            route_recovery_time = timeFromUs(atof(argv[i+2]));
            weight_recovery_time = timeFromUs(atof(argv[i+3]));
            i+=3;
        } else if (!strcmp(argv[i],"-fdonetime")){
            fail_recover_time = timeFromUs(atof(argv[i+1]));
            route_return_time = timeFromUs(atof(argv[i+2]));
            weight_return_time = timeFromUs(atof(argv[i+3]));
            i+=3;
        } else if (!strcmp(argv[i],"-plbecn")){
            plb_ecn = atoi(argv[i+1]);
            i++;
        } else if (!strcmp(argv[i],"-ecnthres")){
            ecn_thres = atoi(argv[i+1]);
            FatTreeSwitch::set_ecn_threshold(ecn_thres);
            i++;
        } else if (!strcmp(argv[i],"-trim")){ // TODO: Get rid of these parameters which would be incoherent in this setting
            queue_type = COMPOSITE_ECN;
        } else if (!strcmp(argv[i],"-rts")){
            rts = true;
        } else if (!strcmp(argv[i],"-flakylinks")){
            flaky_links = atoi(argv[i+1]);
            i++;
        } else if (!strcmp(argv[i],"-lat")){
            latency = atoi(argv[i+1]);
            i++;
        } else if (!strcmp(argv[i],"-tsample")){
            tput_sample_time = timeFromUs((uint32_t)atoi(argv[i+1]));
            i++;            
        } else if (!strcmp(argv[i],"-ratecoef")){
            rate_coef = stod(argv[i+1]);
            i++;
        } else if (!strcmp(argv[i],"-k")){
            k = stoi(argv[i+1]);
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
                FatTreeSwitch::set_strategy(FatTreeSwitch::ECMP);
                route_strategy = ECMP;
            }else if (!strcmp(argv[i+1], "pkt_ar")) {
                FatTreeSwitch::set_strategy(FatTreeSwitch::ADAPTIVE_ROUTING);
                route_strategy = ADAPTIVE_ROUTING;
            } else if (!strcmp(argv[i+1], "fl_ar")) {
                FatTreeSwitch::set_strategy(FatTreeSwitch::ADAPTIVE_ROUTING);
                FatTreeSwitch::set_ar_sticky(FatTreeSwitch::PER_FLOWLET);
                route_strategy = ADAPTIVE_ROUTING;
            } else if (!strcmp(argv[i+1], "ecmp_ar")) {
                FatTreeSwitch::set_strategy(FatTreeSwitch::ECMP_ADAPTIVE);
                route_strategy = ECMP_ADAPTIVE;
            } else if (!strcmp(argv[i+1], "rr")) {
                FatTreeSwitch::set_strategy(FatTreeSwitch::RR);
                route_strategy = RR;
            } else if (!strcmp(argv[i+1], "rr_ecmp")) {
                FatTreeSwitch::set_strategy(FatTreeSwitch::RR_ECMP);
                route_strategy = RR_ECMP;
            } else {
                exit_error(argv[0]);
            }
            i++;
        } else {
            exit_error(argv[i]);
        }
        i++;
    }
    if (failure_pct == -1.0) {  // signifies failure that is routed around (vs. still in route tables with 0.0)
        binary_failure = true; 
        binary_failures= link_failures;
        link_failures = 0;
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
      
    
    // Log of per-flow stats
    cout << "Logging flows to " << flowfilename.str() << endl;  
    std::ofstream flowlog(flowfilename.str().c_str());
    if (!flowlog){
        cout << "Can't open for writing flow log file!"<<endl;
        exit(1);
    }
    
    ConstantErasureCcaSrc* sender;
    ConstantErasureCcaSink* sink;

    Route* routeout, *routein;
   

    FatTreeTopology* top = new FatTreeTopology(no_of_nodes, linkspeed, queuesize, 
                                                NULL, &eventlist, NULL, queue_type, CONST_SCHEDULER, 
                                                link_failures, failure_pct, rts, latency, 
                                                flaky_links, timeFromUs(100.0), timeFromUs(10.0), 
                                                false);
    // if (flaky_links > 0) {
    //     top->set_flaky_links(flaky_links, timeFromUs(100.0), timeFromUs(10.0)); // todo: parameterize this
    // }
    if (binary_failure) {
        for (int i=0; i< binary_failures; i++){
            failure_managers.push_back(new FailureManager(top, eventlist, fail_time, route_recovery_time, weight_recovery_time, failure_type));
            failure_managers[i]->setFailedLink(FatTreeSwitch::AGG, (int)(i / (top->no_of_pods()/2)), i%(top->no_of_pods()/2)); // TODO: move this into the failure manager rather than here
            if (fail_recover_time != timeInf) {
                failure_managers[i]->setReturnTime(fail_recover_time, route_return_time, weight_return_time);
            }
        }
    }
    

    
    no_of_nodes = top->no_of_nodes();
    cout << "actual nodes " << no_of_nodes << endl;

    // vector<const Route*>*** net_paths;
    // net_paths = new vector<const Route*>**[no_of_nodes];

    // int* is_dest = new int[no_of_nodes];
    
    // for (uint32_t i=0; i<no_of_nodes; i++){
    //     is_dest[i] = 0;
    //     net_paths[i] = new vector<const Route*>*[no_of_nodes];
    //     for (uint32_t j = 0; j<no_of_nodes; j++)
    //         net_paths[i][j] = NULL;
    // }

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
    
    list <ConstantErasureCcaSrc*> srcs;
    list <ConstantErasureCcaSink*> sinks;
    // initialize all sources/sinks

    uint32_t connID = 0;
    all_conns = conns->getAllConnections();
    uint32_t connCount = all_conns->size();

    // ConstEraseRtxTimerScanner rtxScanner(timeFromUs(0.01), eventlist);

    for (uint32_t c = 0; c < all_conns->size(); c++){
        connection* crt = all_conns->at(c);
        uint32_t src = crt->src;
        uint32_t dest = crt->dst;
        
        connID++;
        // if (!net_paths[src][dest]) {
        //     vector<const Route*>* paths = top->get_paths(src,dest);
        //     net_paths[src][dest] = paths;
        //     for (uint32_t p = 0; p < paths->size(); p++) {
        //         routes.push_back((*paths)[p]);
        //     }
        // }
        // if (!net_paths[dest][src]) {
        //     vector<const Route*>* paths = top->get_paths(dest,src);
        //     net_paths[dest][src] = paths;
        // }

        double rate = (linkspeed / ((double)all_conns->size() / no_of_nodes)) / (packet_size * 8); // assumes all nodes have the same number of connections
        simtime_picosec interpacket_delay = timeFromSec(1. / (rate * rate_coef)); //+ rand() % (2*(no_of_nodes-1)); // just to keep them not perfectly in sync
        sender = new ConstantErasureCcaSrc(eventlist, src, interpacket_delay, NULL);  
        // rtxScanner.registerFlow(sender);
        // sender->set_cwnd(cwnd*Packet::data_packet_size());

        // if (queue_type == COMPOSITE_ECN) {
        //     sender->disable_fast_recovery(); // no fast recovery when we have packet trimming
        // }
                        
        if (crt->size>0){
            sender->set_flowsize(crt->size, k);
        } 
                
        if (host_lb == PLB) {
            sender->enable_plb();
            sender->set_plb_threshold_ecn(plb_ecn);
        } else if (host_lb == SPRAY) {
            sender->set_spraying();
        }  else if (host_lb == SPRAY_ADAPTIVE) {
            // sender->set_spraying();
            sender->set_adaptive();
        }
        srcs.push_back(sender);
        sink = new ConstantErasureCcaSink();
        sinks.push_back(sink);

        sender->setName("constcca_" + ntoa(src) + "_" + ntoa(dest));

        sink->setName("constcca_sink_" + ntoa(src) + "_" + ntoa(dest));

          
        if (route_strategy != SOURCE_ROUTE) {
            routeout = top->get_tor_route(src);
            routein = top->get_tor_route(dest);
        } 
        else {
            printf("Source routing not currently supported");
            exit(1);
        }
        
        
        // simtime_picosec offset = (interpacket_delay/connCount) * (rand()%(connCount-1));
        // simtime_picosec starttime = crt->start + offset;
        // cout << "starttime " << timeAsUs(starttime) << endl;
        sender->connect(*sink, (uint32_t)crt->start + rand()%(interpacket_delay), dest, *routeout, *routein);
        // sender->set_paths(net_paths[src][dest]);

        if (route_strategy != SOURCE_ROUTE) {
            top->add_host_port(src, sender->flow().flow_id(), sender);
            top->add_host_port(dest, sender->flow().flow_id(), sink);
        }
    }
    //    ShortFlows* sf = new ShortFlows(2560, eventlist, net_paths,conns,lg, &swiftRtxScanner);

    cout << "Loaded " << connID << " connections in total\n";

    // top->populate_all_routes(); // Do not lazily find routes since we will need all routes for later changes

    // GO!
    cout << "Starting simulation" << endl;
    simtime_picosec checkpoint = timeFromUs(100.0);
    while (eventlist.doNextEvent()) {
        if (eventlist.now() > checkpoint) {
            cout << "Simulation time " << timeAsUs(eventlist.now()) << endl;
            checkpoint += timeFromUs(100.0);
            if (endtime == 0) {
                // Iterate through sinks to see if they have completed the flows
                bool all_done = true;
                list <ConstantErasureCcaSink*>::iterator sink_i;
                for (sink_i = sinks.begin(); sink_i != sinks.end(); sink_i++) {
                    if ((*sink_i)->_src->_completion_time == 0) {
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
    list <ConstantErasureCcaSrc*>::iterator src_i;
    // for (src_i = swift_srcs.begin(); src_i != swift_srcs.end(); src_i++) {
    //     cout << "Src, sent: " << (*src_i)->_highest_dsn_sent << "[rtx: " << (*src_i)->_subs[0]. << "] nacks: " << (*src_i)->_nacks_received << " pulls: " << (*src_i)->_pulls_received << " paths: " << (*src_i)->_paths.size() << endl;
    // }
    flowlog << "Flow ID,Completion Time,ReceivedBytes,PacketsSent" << endl;
    for (src_i = srcs.begin(); src_i != srcs.end(); src_i++) {
        ConstantErasureCcaSink* sink = (*src_i)->_sink;
        simtime_picosec time = (*src_i)->_completion_time > 0 ? (*src_i)->_completion_time - (*src_i)->_start_time: 0;
        flowlog << (*src_i)->_addr << "->" << (*src_i)->_destination << "," << time << "," << sink->cumulative_ack() << "," << (*src_i)->_packets_sent <<  endl;
    }
    flowlog.close();
    list <ConstantErasureCcaSink*>::iterator sink_i;
    for (sink_i = sinks.begin(); sink_i != sinks.end(); sink_i++) {
        cout << (*sink_i)->nodename() << " received " << (*sink_i)->cumulative_ack() << " bytes" << endl;
        if ((*sink_i)->cumulative_ack() < 2004000) { // todo: change thsi
            cout << "Incomplete flow " << endl;
            ConstantErasureCcaSink* sink = (*sink_i);
            ConstantErasureCcaSrc* counterpart_src = sink->_src;
            cout << "Src, sent: " << counterpart_src->_packets_sent << "; ACKED " << counterpart_src->packets_acked() << endl;
        }
    }
}
