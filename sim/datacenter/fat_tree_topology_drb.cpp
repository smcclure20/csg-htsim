// -*- c-basic-offset: 4; indent-tabs-mode: nil -*-
#include "fat_tree_topology_drb.h"
#include <vector>
#include "string.h"
#include <sstream>

#include <iostream>
#include "main.h"
#include "../queue.h"
#include "fat_tree_switch_drb.h"
#include "../compositequeue.h"
#include "../aeolusqueue.h"
#include "../prioqueue.h"
#include "../ecnprioqueue.h"
#include "../queue_lossless.h"
#include "../queue_lossless_input.h"
#include "../queue_lossless_output.h"
#include "../swift_scheduler.h"
#include "../constant_cca_scheduler.h"
#include "../ecnqueue.h"

// use tokenize from connection matrix
extern void tokenize(string const &str, const char delim, vector<string> &out);

// Helper function to parse a link description string (implementation)
bool parse_link_description_string(const std::string& desc, ParsedLinkInfo& info) {
    // Expected formats:
    // "pod<pod_idx>_spine<spine_idx_in_pod>_leaf<leaf_idx_in_pod>"
    // "pod<pod_idx>_spine<spine_idx_in_pod>_core<core_idx_connected_to_spine>"
    // "tor<tor_id>_host<host_id>"

    size_t pos;

    // Try parsing "tor<tor_id>_host<host_id>"
    pos = desc.find("tor");
    if (pos != std::string::npos) {
        size_t end_tor = desc.find("_host", pos);
        if (end_tor != std::string::npos) {
            std::string tor_str = desc.substr(pos + 3, end_tor - (pos + 3));
            info.tor_id = std::stoi(tor_str);
            std::string host_str = desc.substr(end_tor + 5);
            info.host_id = std::stoi(host_str);
            info.type = ParsedLinkInfo::TOR_HOST_LINK;
            return true;
        }
    }

    // Try parsing "pod<pod_idx>_spine<spine_idx_in_pod>_leaf<leaf_idx_in_pod>" or "pod<pod_idx>_spine<spine_idx_in_pod>_core<core_idx_connected_to_spine>"
    pos = desc.find("pod");
    if (pos != std::string::npos) {
        size_t end_pod = desc.find("_spine", pos);
        if (end_pod != std::string::npos) {
            std::string pod_str = desc.substr(pos + 3, end_pod - (pos + 3));
            info.pod_id = std::stoi(pod_str);

            pos = end_pod + 6; // After "_spine"
            size_t end_spine = desc.find("_", pos); // Find next underscore
            if (end_spine != std::string::npos) {
                std::string spine_str = desc.substr(pos, end_spine - pos);
                info.spine_idx_in_pod = std::stoi(spine_str);

                pos = end_spine + 1; // After "_"
                if (desc.substr(pos, 4) == "leaf") {
                    info.type = ParsedLinkInfo::TOR_AGG_LINK; // Between ToR and Aggregate
                    std::string leaf_str = desc.substr(pos + 4);
                    info.leaf_idx_in_pod = std::stoi(leaf_str);
                    return true;
                } else if (desc.substr(pos, 4) == "core") {
                    info.type = ParsedLinkInfo::AGG_CORE_LINK; // Between Aggregate and Core
                    std::string core_str = desc.substr(pos + 4);
                    info.core_id_connected_to_spine = std::stoi(core_str);
                    return true;
                }
            }
        }
    }

    return false; // Could not parse
}

// default to 3-tier topology.  Change this with set_tiers() before calling the constructor.
uint32_t FatTreeTopologyDRB::_tiers = 3;
simtime_picosec FatTreeTopologyDRB::_link_latencies[] = {0,0,0};
simtime_picosec FatTreeTopologyDRB::_switch_latencies[] = {0,0,0};
uint32_t FatTreeTopologyDRB::_hosts_per_pod = 0;
uint32_t FatTreeTopologyDRB::_radix_up[] = {0,0};
uint32_t FatTreeTopologyDRB::_radix_down[] = {0,0,0};
mem_b FatTreeTopologyDRB::_queue_up[] = {0,0};
mem_b FatTreeTopologyDRB::_queue_down[] = {0,0,0};
uint32_t FatTreeTopologyDRB::_bundlesize[] = {1,1,1};
uint32_t FatTreeTopologyDRB::_oversub[] = {1,1,1};
linkspeed_bps FatTreeTopologyDRB::_downlink_speeds[] = {0,0,0};

void
FatTreeTopologyDRB::set_tier_parameters(int tier, int radix_up, int radix_down, mem_b queue_up, mem_b queue_down, int bundlesize, linkspeed_bps linkspeed, int oversub) {
    // tier is 0 for ToR, 1 for agg switch, 2 for core switch
    if (tier < CORE_TIER) {
        // no uplinks from core switches
        _radix_up[tier] = radix_up;
        _queue_up[tier] = queue_up;
    }
    _radix_down[tier] = radix_down;
    _queue_down[tier] = queue_down;
    _bundlesize[tier] = bundlesize;
    _downlink_speeds[tier] = linkspeed; // this is the link going downwards from this tier.  up/down linkspeeds are symmetric.
    _oversub[tier] = oversub;
    // xxx what to do about queue sizes
}

// load a config file and use it to create a FatTreeTopologyDRB
FatTreeTopologyDRB* FatTreeTopologyDRB::load(const char * filename, QueueLoggerFactory* logger_factory, EventList& eventlist, mem_b queuesize, queue_type q_type, queue_type sender_q_type){
    std::ifstream file(filename);
    if (file.is_open()) {
        FatTreeTopologyDRB* ft = load(file, logger_factory, eventlist, queuesize, q_type, sender_q_type);
        file.close();
	return ft;
    } else {
        cerr << "Failed to open FatTree config file " << filename << endl;
        exit(1);
    }
}

// in-place conversion to lower case
void to_lower(string& s) {
    string::iterator i;
    for (i = s.begin(); i != s.end(); i++) {
        *i = std::tolower(*i);
    }
        //std::transform(s.begin(), s.end(), s.begin(),
        //[](unsigned char c){ return std::tolower(c); });
}

FatTreeTopologyDRB* FatTreeTopologyDRB::load(istream& file, QueueLoggerFactory* logger_factory, EventList& eventlist, mem_b queuesize, queue_type q_type, queue_type sender_q_type){
    //cout << "topo load start\n";
    std::string line;
    int linecount = 0;
    int no_of_nodes = 0;
    _tiers = 0;
    _hosts_per_pod = 0;
    for (int tier = 0; tier < 3; tier++) {
        _queue_down[tier] = queuesize;
        if (tier != 2)
            _queue_up[tier] = queuesize;
    }
    while (std::getline(file, line)) {
        linecount++;
        vector<string> tokens;
        tokenize(line, ' ', tokens);
        if (tokens.size() == 0)
            continue;
        if (tokens[0][0] == '#') {
            continue;
        }
        to_lower(tokens[0]);
        if (tokens[0] == "nodes") {
            no_of_nodes = stoi(tokens[1]);
        } else if (tokens[0] == "tiers") {
            _tiers = stoi(tokens[1]);
        } else if (tokens[0] == "podsize") {
            _hosts_per_pod = stoi(tokens[1]);
        } else if (tokens[0] == "tier") {
            // we're done with the header
            break;
        }
    }
    if (no_of_nodes == 0) {
        cerr << "Missing number of nodes in header" << endl;
        exit(1);
    }
    if (_tiers == 0) {
        cerr << "Missing number of tiers in header" << endl;
        exit(1);
    }
    if (_tiers < 2 || _tiers > 3) {
        cerr << "Invalid number of tiers: " << _tiers << endl;
        exit(1);
    }
    if (_hosts_per_pod == 0) {
        cerr << "Missing pod size in header" << endl;
        exit(1);
    }
    linecount--;
    bool tiers_done[3] = {false, false, false};
    int current_tier = -1;
    do {
        linecount++;
        vector<string> tokens;
	tokenize(line, ' ', tokens);
        if (tokens.size() < 1) {
            continue;
    	}
        to_lower(tokens[0]);
        if (tokens.size() == 0 || tokens[0][0] == '#') {
            continue;
        } else if (tokens[0] == "tier") {
            current_tier = stoi(tokens[1]);
            if (current_tier < 0 || current_tier > 2) {
                cerr << "Invalid tier " << current_tier << " at line " << linecount << endl;
                exit(1);
            }
            tiers_done[current_tier] = true;
        } else if (tokens[0] == "downlink_speed_gbps") {
            if (_downlink_speeds[current_tier] != 0) {
                cerr << "Duplicate linkspeed setting for tier " << current_tier << " at line " << linecount << endl;
                exit(1);
            }
            _downlink_speeds[current_tier] = ((linkspeed_bps)stoi(tokens[1])) * 1000000000;
        } else if (tokens[0] == "radix_up") {
            if (_radix_up[current_tier] != 0) {
                cerr << "Duplicate radix_up setting for tier " << current_tier << " at line " << linecount << endl;
                exit(1);
            }
            if (current_tier == 2) {
                cerr << "Can't specific radix_up for tier " << current_tier << " at line " << linecount << " (no uplinks from top tier!)" << endl;
                exit(1);
            }
            _radix_up[current_tier] = stoi(tokens[1]);
        } else if (tokens[0] == "radix_down") {
            if (_radix_down[current_tier] != 0) {
                cerr << "Duplicate radix_down setting for tier " << current_tier << " at line " << linecount << endl;
                exit(1);
            }
            _radix_down[current_tier] = stoi(tokens[1]);
        } else if (tokens[0] == "queue_up") {
            if (_queue_up[current_tier] != 0) {
                cerr << "Duplicate queue_up setting for tier " << current_tier << " at line " << linecount << endl;
                exit(1);
            }
            if (current_tier == 2) {
                cerr << "Can't specific queue_up for tier " << current_tier << " at line " << linecount << " (no uplinks from top tier!)" << endl;
                exit(1);
            }
            _queue_up[current_tier] = stoi(tokens[1]);
        } else if (tokens[0] == "queue_down") {
            if (_queue_down[current_tier] != 0) {
                cerr << "Duplicate queue_down setting for tier " << current_tier << " at line " << linecount << endl;
                exit(1);
            }
            _queue_down[current_tier] = stoi(tokens[1]);
        } else if (tokens[0] == "oversubscribed") {
            if (_oversub[current_tier] != 1) {
                cerr << "Duplicate oversubscribed setting for tier " << current_tier << " at line " << linecount << endl;
                exit(1);
            }
            _oversub[current_tier] = stoi(tokens[1]); 
        } else if (tokens[0] == "bundle") {
            if (_bundlesize[current_tier] != 1) {
                cerr << "Duplicate bundle size setting for tier " << current_tier << " at line " << linecount << endl;
                exit(1);
            }
            _bundlesize[current_tier] = stoi(tokens[1]); 
        } else if (tokens[0] == "switch_latency_ns") {
            if (_switch_latencies[current_tier] != 0) {
                cerr << "Duplicate switch_latency setting for tier " << current_tier << " at line " << linecount << endl;
                exit(1);
            }
            _switch_latencies[current_tier] = timeFromNs(stoi(tokens[1])); 
        } else if (tokens[0] == "downlink_latency_ns") {
            if (_link_latencies[current_tier] != 0) {
                cerr << "Duplicate link latency setting for tier " << current_tier << " at line " << linecount << endl;
                exit(1);
            }
            _link_latencies[current_tier] = timeFromNs(stoi(tokens[1])); 
        }
    } while (std::getline(file, line));
    for (uint32_t tier = 0; tier < _tiers; tier++) {
        if (tiers_done[tier] == false) {
            cerr << "No configuration found for tier " << tier << endl;
            exit(1);
        }
        if (_downlink_speeds[tier] == 0) {
            cerr << "Missing downlink_speed_gbps for tier " << tier << endl;
            exit(1);
        }
        if (_link_latencies[tier] == 0) {
            cerr << "Missing downlink_latency_ns for tier " << tier << endl;
            exit(1);
        }
        if (tier < (_tiers - 1) && _radix_up[tier] == 0) {
            cerr << "Missing radix_up for tier " << tier << endl;
            exit(1);
        }
        if (_radix_down[tier] == 0) {
            cerr << "Missing radix_down for tier " << tier << endl;
            exit(1);
        }
        if (tier < (_tiers - 1) && _queue_up[tier] == 0) {
            cerr << "Missing queue_up for tier " << tier << endl;
            exit(1);
        }
        if (_queue_down[tier] == 0) {
            cerr << "Missing queue_down for tier " << tier << endl;
            exit(1);
        }
    }

    cout << "Topology load done\n";
    FatTreeTopologyDRB* ft = new FatTreeTopologyDRB(no_of_nodes, 0, 0, logger_factory, &eventlist, NULL, q_type, 0, 0, sender_q_type);
    cout << "FatTree constructor done, " << ft->no_of_nodes() << " nodes created\n";
    return ft;
}

FatTreeTopologyDRB::FatTreeTopologyDRB(uint32_t no_of_nodes, linkspeed_bps linkspeed, mem_b queuesize,
                                 QueueLoggerFactory* logger_factory,
                                 EventList* ev,FirstFit * fit,queue_type q, simtime_picosec latency, simtime_picosec switch_latency, queue_type snd){
    
    set_linkspeeds(linkspeed);
    set_queue_sizes(queuesize);
    _logger_factory = logger_factory;
    _eventlist = ev;
    ff = fit;
    _qt = q;
    _sender_qt = snd;
    failed_links = 0;
    if ((latency != 0 || switch_latency != 0) && _link_latencies[TOR_TIER] != 0) {
        cerr << "Don't set latencies using both the constructor and set_latencies - use only one of the two\n";
        exit(1);
    }
    _hop_latency = latency;
    _switch_latency = switch_latency;

    if (_link_latencies[TOR_TIER] == 0) {
        cout << "Fat Tree topology with " << timeAsUs(_hop_latency) << "us links and " << timeAsUs(_switch_latency) <<"us switching latency." <<endl;
    } else {
        cout << "Fat Tree topology with "
             << timeAsUs(_link_latencies[TOR_TIER]) << "us Src-ToR links, "
             << timeAsUs(_link_latencies[AGG_TIER]) << "us ToR-Agg links, ";
        if (_tiers == 3) {
            cout << timeAsUs(_link_latencies[CORE_TIER]) << "us Agg-Core links, ";
        }
        cout << timeAsUs(_switch_latencies[TOR_TIER]) << "us ToR switch latency, "
             << timeAsUs(_switch_latencies[AGG_TIER]) << "us Agg switch latency";
        if (_tiers == 3) {
            cout << ", " << timeAsUs(_switch_latencies[CORE_TIER]) << "us Core switch latency." << endl;
        } else {
            cout << "." << endl;
        }
    }
    set_params(no_of_nodes);

    init_network();
}

FatTreeTopologyDRB::FatTreeTopologyDRB(uint32_t no_of_nodes, linkspeed_bps linkspeed, mem_b queuesize,
                                 QueueLoggerFactory* logger_factory,
                                 EventList* ev,FirstFit * fit,queue_type q){
    set_linkspeeds(linkspeed);
    set_queue_sizes(queuesize);
    _logger_factory = logger_factory;
    _eventlist = ev;
    ff = fit;
    _qt = q;
    _sender_qt = FAIR_PRIO;
    failed_links = 0;
    _rts = false;
    flaky_links = 0;
    if (_link_latencies[TOR_TIER] == 0) {
        _hop_latency = timeFromUs((uint32_t)1);
    } else {
        _hop_latency = timeFromUs((uint32_t)0); 
    }
    _switch_latency = timeFromUs((uint32_t)0); 
 
    cout << "Fat tree topology (1) with " << no_of_nodes << " nodes" << endl;
    set_params(no_of_nodes);

    init_network();
}

FatTreeTopologyDRB::FatTreeTopologyDRB(uint32_t no_of_nodes, linkspeed_bps linkspeed, mem_b queuesize,
                                 QueueLoggerFactory* logger_factory,
                                 EventList* ev,FirstFit * fit, queue_type q, uint32_t num_failed, double fail_pct){
    set_linkspeeds(linkspeed);
    set_queue_sizes(queuesize);
    if (_link_latencies[TOR_TIER] == 0) {
        _hop_latency = timeFromUs((uint32_t)1);
    } else {
        _hop_latency = timeFromUs((uint32_t)0); 
    }
    _switch_latency = timeFromUs((uint32_t)0); 
    _logger_factory = logger_factory;
    _qt = q;
    _sender_qt = FAIR_PRIO;

    _eventlist = ev;
    ff = fit;

    failed_links = num_failed;
    fail_bw_pct = fail_pct;
    _rts = false;
    flaky_links = 0;
  
    cout << "Fat tree topology (2) with " << no_of_nodes << " nodes" << endl;
    set_params(no_of_nodes);

    init_network();
}

FatTreeTopologyDRB::FatTreeTopologyDRB(uint32_t no_of_nodes, linkspeed_bps linkspeed, mem_b queuesize,
                                 QueueLoggerFactory* logger_factory,
                                 EventList* ev,FirstFit * fit, queue_type qtype,
                                 queue_type sender_qtype, uint32_t num_failed, double fail_pct, bool rts, simtime_picosec hoplatency,
                                 int flakylinks, simtime_picosec interarrival, simtime_picosec duration, bool weighted, const vector<string>& failed_link_descs){
    set_linkspeeds(linkspeed);
    set_queue_sizes(queuesize);
    if (hoplatency != 0) {
        _hop_latency = hoplatency;
    } else if (_link_latencies[TOR_TIER] == 0) {
        _hop_latency = timeFromUs((uint32_t)1);
    } else {
        _hop_latency = timeFromUs((uint32_t)0); 
    }
    _switch_latency = timeFromUs((uint32_t)0); 
    _logger_factory = logger_factory;
    _qt = qtype;
    _sender_qt = sender_qtype;

    _eventlist = ev;
    ff = fit;

    failed_links = num_failed;
    fail_bw_pct = fail_pct;
    _rts = rts;
    flaky_links = flakylinks;
    if (flaky_links > 0) {
        _link_loss_burst_interarrival_time = interarrival;
        _link_loss_burst_duration = duration;
    }
    
    set_weighted(weighted);

    _failed_link_descriptions = failed_link_descs; // Store the passed failed link descriptions

    // Parse all failed link descriptions once
    for (const string& desc : _failed_link_descriptions) {
        ParsedLinkInfo info;
        if (parse_link_description_string(desc, info)) {
            _parsed_failed_links.push_back(info);
            cout << "Parsed failed link: " << desc << " (Type: " << info.type
                 << ", Pod: " << info.pod_id << ", Spine: " << info.spine_idx_in_pod
                 << ", Leaf: " << info.leaf_idx_in_pod << ", Core: " << info.core_id_connected_to_spine << ")" << endl;
        } else {
            cerr << "Warning: Could not parse failed link description: " << desc << endl;
        }
    }

    cout << "Fat tree topology (3) with " << no_of_nodes << " nodes" << endl;
    set_params(no_of_nodes);

    init_network();
}

void FatTreeTopologyDRB::set_linkspeeds(linkspeed_bps linkspeed) {
    if (linkspeed != 0 && _downlink_speeds[TOR_TIER] != 0) {
        cerr << "Don't set linkspeeds using both the constructor and set_tier_parameters - use only one of the two\n";
        exit(1);
    }
    if (linkspeed == 0 && _downlink_speeds[TOR_TIER] == 0) {
        cerr << "Linkspeed is not set, either as a default or by constructor\n";
        exit(1);
    }
    // set tier linkspeeds if no defaults are specified
    if (_downlink_speeds[TOR_TIER] == 0) {_downlink_speeds[TOR_TIER] = linkspeed;}
    if (_downlink_speeds[AGG_TIER] == 0) {_downlink_speeds[AGG_TIER] = linkspeed;}
    if (_downlink_speeds[CORE_TIER] == 0) {_downlink_speeds[CORE_TIER] = linkspeed;}
}

void FatTreeTopologyDRB::set_queue_sizes(mem_b queuesize) {
    if (queuesize != 0) {
        // all tiers use the same queuesize
        for (int tier = TOR_TIER; tier <= CORE_TIER; tier++) {
            _queue_down[tier] = queuesize;
            if (tier != CORE_TIER)
                _queue_up[tier] = queuesize;
        }
    } else {
        // the tier queue sizes must have already been set
        assert(_queue_down[TOR_TIER] != 0);
    }
}

void FatTreeTopologyDRB::set_custom_params(uint32_t no_of_nodes) {
    //cout << "set_custom_params" << endl;
    // do some sanity checking before we proceed
    assert(_hosts_per_pod > 0);

    // check bundlesizes are feasible with switch radix
    for (uint32_t tier = TOR_TIER; tier < _tiers; tier++) {
        if (_radix_down[tier] == 0) {
            cerr << "Custom topology, but radix_down not set for tier " << tier << endl;
            exit(1);
        }
        if (_radix_down[tier] % _bundlesize[tier] != 0) {
            cerr << "Mismatch between tier " << tier << " down radix of " << _radix_down[tier] << " and bundlesize " << _bundlesize[tier] << "\n";
            cerr << "Radix must be a multiple of bundlesize\n";
            exit(1);
        }
        if (tier < (_tiers - 1) && _radix_up[tier] == 0) {
            cerr << "Custom topology, but radix_up not set for tier " << tier << endl;
            exit(1);
        }
        if (tier < (_tiers - 1) && _radix_up[tier] % _bundlesize[tier+1] != 0) {
            cerr << "Mismatch between tier " << tier << " up radix of " << _radix_up[tier] << " and tier " << tier+1 << " down bundlesize " << _bundlesize[tier+1] << "\n";
            cerr << "Radix must be a multiple of bundlesize\n";
            exit(1);
        }
    }

    int no_of_pods = 0;
    _no_of_nodes = no_of_nodes;
    _tor_switches_per_pod = 0;
    _agg_switches_per_pod = 0;
    int no_of_tor_uplinks = 0;
    int no_of_agg_uplinks = 0;
    int no_of_core_switches = 0;
    if (no_of_nodes % _hosts_per_pod != 0) {
        cerr << "No_of_nodes is not a multiple of hosts_per_pod\n";
        exit(1);
    }

    no_of_pods = no_of_nodes / _hosts_per_pod; // we don't allow multi-port hosts yet
    assert(_bundlesize[TOR_TIER] == 1);
    if (_hosts_per_pod % _radix_down[TOR_TIER] != 0) {
        cerr << "Mismatch between TOR radix " << _radix_down[TOR_TIER] << " and podsize " << _hosts_per_pod << endl;
        exit(1);
    }
    _tor_switches_per_pod = _hosts_per_pod / _radix_down[TOR_TIER];

    assert((no_of_nodes * _downlink_speeds[TOR_TIER]) % (_downlink_speeds[AGG_TIER] * _oversub[TOR_TIER]) == 0);
    no_of_tor_uplinks = (no_of_nodes * _downlink_speeds[TOR_TIER]) / (_downlink_speeds[AGG_TIER] *  _oversub[TOR_TIER]);
    cout << "no_of_tor_uplinks: " << no_of_tor_uplinks << endl;

    if (_radix_down[TOR_TIER]/_radix_up[TOR_TIER] != _oversub[TOR_TIER]) {
        cerr << "Mismatch between TOR linkspeeds (" << speedAsGbps(_downlink_speeds[TOR_TIER]) << "Gbps down, "
             << speedAsGbps(_downlink_speeds[AGG_TIER]) << "Gbps up) and TOR radix (" << _radix_down[TOR_TIER] << " down, "
             << _radix_up[TOR_TIER] << " up) and oversubscription ratio of " << _oversub[TOR_TIER] << endl;
        exit(1);
    }

    assert(no_of_tor_uplinks % (no_of_pods * _radix_down[AGG_TIER]) == 0);
    _agg_switches_per_pod = no_of_tor_uplinks / (no_of_pods * _radix_down[AGG_TIER]);
    if (_agg_switches_per_pod * _bundlesize[AGG_TIER] != _radix_up[TOR_TIER]) {
        cerr << "Mismatch between TOR up radix " << _radix_up[TOR_TIER] << " and " << _agg_switches_per_pod
             << " aggregation switches per pod required by " << no_of_tor_uplinks << " TOR uplinks in "
             << no_of_pods << " pods " << " with an aggregation switch down radix of " << _radix_down[AGG_TIER] << endl;
        if (_bundlesize[AGG_TIER] == 1 && _radix_up[TOR_TIER] % _agg_switches_per_pod  == 0 && _radix_up[TOR_TIER]/_agg_switches_per_pod > 1) {
            cerr << "Did you miss specifying a Tier 1 bundle size of " << _radix_up[TOR_TIER]/_agg_switches_per_pod << "?" << endl;
        } else if (_radix_up[TOR_TIER] % _agg_switches_per_pod  == 0
                   && _radix_up[TOR_TIER]/_agg_switches_per_pod != _bundlesize[AGG_TIER]) {
            cerr << "Tier 1 bundle size is " << _bundlesize[AGG_TIER] << ". Did you mean it to be "
                 << _radix_up[TOR_TIER]/_agg_switches_per_pod << "?" << endl;
        }
        exit(1);
    }

    if (_tiers == 3) {
        assert((no_of_tor_uplinks * _downlink_speeds[AGG_TIER]) % (_downlink_speeds[CORE_TIER] * _oversub[AGG_TIER]) == 0);
        no_of_agg_uplinks = (no_of_tor_uplinks * _downlink_speeds[AGG_TIER]) / (_downlink_speeds[CORE_TIER] * _oversub[AGG_TIER]);
        cout << "no_of_agg_uplinks: " << no_of_agg_uplinks << endl;

        assert(no_of_agg_uplinks % _radix_down[CORE_TIER] == 0);
        no_of_core_switches = no_of_agg_uplinks / _radix_down[CORE_TIER];

        if (no_of_core_switches % _agg_switches_per_pod != 0) {
            cerr << "Topology results in " << no_of_core_switches << " core switches, which isn't an integer multiple of "
                 << _agg_switches_per_pod << " aggregation switches per pod, computed from Tier 0 and 1 values\n";
            exit(1);
        }

        if ((no_of_core_switches * _bundlesize[CORE_TIER])/ _agg_switches_per_pod  != _radix_up[AGG_TIER]) {
            cerr << "Mismatch between the AGG switch up-radix of " << _radix_up[AGG_TIER] << " and calculated "
                 << _agg_switches_per_pod << " aggregation switched per pod with " << no_of_core_switches << " core switches" << endl;
            if (_bundlesize[CORE_TIER] == 1
                && _radix_up[AGG_TIER] % (no_of_core_switches/_agg_switches_per_pod) == 0
                && _radix_up[AGG_TIER] / (no_of_core_switches/_agg_switches_per_pod) > 1) {
                cerr << "Did you miss specifying a Tier 2 bundle size of "
                     << _radix_up[AGG_TIER] / (no_of_core_switches/_agg_switches_per_pod) << "?" << endl;
            } else if (_radix_up[AGG_TIER] % (no_of_core_switches/_agg_switches_per_pod) == 0
                       && _radix_up[AGG_TIER] / (no_of_core_switches/_agg_switches_per_pod) != _bundlesize[CORE_TIER]) {
                cerr << "Tier 2 bundle size is " << _bundlesize[CORE_TIER] << ". Did you mean it to be "
                     << _radix_up[AGG_TIER] /	(no_of_core_switches/_agg_switches_per_pod) << "?" << endl;
            }
            exit(1);
        }
    }

    cout << "No of nodes: " << no_of_nodes << endl;
    cout << "No of pods: " << no_of_pods << endl;
    cout << "Hosts per pod: " << _hosts_per_pod << endl;
    cout << "Hosts per pod: " << _hosts_per_pod << endl;
    cout << "ToR switches per pod: " << _tor_switches_per_pod << endl;
    cout << "Agg switches per pod: " << _agg_switches_per_pod << endl;
    cout << "No of core switches: " << no_of_core_switches << endl;
    for (uint32_t tier = TOR_TIER; tier < _tiers; tier++) {
        cout << "Tier " << tier << " QueueSize Down " << _queue_down[tier] << " bytes" << endl;
        if (tier < CORE_TIER)
            cout << "Tier " << tier << " QueueSize Up " << _queue_up[tier] << " bytes" << endl;
    }

    // looks like we're OK, lets build it
    NSRV = no_of_nodes;
    NTOR = _tor_switches_per_pod * no_of_pods;
    NAGG = _agg_switches_per_pod * no_of_pods;
    NPOD = no_of_pods;
    NCORE = no_of_core_switches;
    alloc_vectors();
}


void FatTreeTopologyDRB::set_params(uint32_t no_of_nodes) {
    if (_hosts_per_pod > 0) {
        // if we've set all the detailed parameters, we'll use them, otherwise fall through to defaults
        set_custom_params(no_of_nodes);
        return;
    }
    
    cout << "Set params " << no_of_nodes << endl;
    for (int tier = TOR_TIER; tier <= CORE_TIER; tier++) {
        cout << "Tier " << tier << " QueueSize Down " << _queue_down[tier] << " bytes" << endl;
        if (tier < CORE_TIER)
            cout << "Tier " << tier << " QueueSize Up " << _queue_up[tier] << " bytes" << endl;
    }
    _no_of_nodes = 0;
    int K = 0;
    if (_tiers == 3) {
        while (_no_of_nodes < no_of_nodes) {
            K++;
            _no_of_nodes = K * K * K /4;
        }
        if (_no_of_nodes > no_of_nodes) {
            cerr << "Topology Error: can't have a 3-Tier FatTree with " << no_of_nodes
                 << " nodes\n";
            exit(1);
        }
        int NK = (K*K/2);
        NSRV = (K*K*K/4);
        NTOR = NK;
        NAGG = NK;
        NPOD = K;
        NCORE = (K*K/4);
    } else if (_tiers == 2) {
        // We want a leaf-spine topology
        while (_no_of_nodes < no_of_nodes) {
            K++;
            _no_of_nodes = K * K /2;
        }
        if (_no_of_nodes > no_of_nodes) {
            cerr << "Topology Error: can't have a 2-Tier FatTree with " << no_of_nodes
                 << " nodes\n";
            exit(1);
        }
        int NK = K;
        NSRV = K * K /2;
        NTOR = NK;
        NAGG = NK/2;
        NPOD = 1;
        NCORE = 0;
    } else {
        cerr << "Topology Error: " << _tiers << " tier FatTree not supported\n";
        exit(1);
    }
    
    
    cout << "_no_of_nodes " << _no_of_nodes << endl;
    cout << "K " << K << endl;
    cout << "Queue type " << _qt << endl;

    // if these are set, we should be in the custom code, not here
    assert(_radix_down[TOR_TIER] == 0); 
    assert(_radix_up[TOR_TIER] == 0);
    
    _radix_down[TOR_TIER] = K/2;
    _radix_up[TOR_TIER] = K/2;
    _radix_down[AGG_TIER] = K/2;
    _radix_up[AGG_TIER] = K/2;
    _radix_down[CORE_TIER] = K;
    assert(_hosts_per_pod == 0);
    _tor_switches_per_pod = K/2;
    _agg_switches_per_pod = K/2;
    _hosts_per_pod = _no_of_nodes / NPOD;

    alloc_vectors();
}

void FatTreeTopologyDRB::alloc_vectors() {

    switches_lp.resize(NTOR,NULL);
    switches_up.resize(NAGG,NULL);
    switches_c.resize(NCORE,NULL);


    // These vectors are sparse - we won't use all the entries
    if (_tiers == 3) {
        // resizing 3d vectors is scary magic
        pipes_nc_nup.resize(NCORE, vector< vector<Pipe*> >(NAGG, vector<Pipe*>(_bundlesize[CORE_TIER])));
        queues_nc_nup.resize(NCORE, vector< vector<BaseQueue*> >(NAGG, vector<BaseQueue*>(_bundlesize[CORE_TIER])));
    }

    pipes_nup_nlp.resize(NAGG, vector< vector<Pipe*> >(NTOR, vector<Pipe*>(_bundlesize[AGG_TIER])));
    queues_nup_nlp.resize(NAGG, vector< vector<BaseQueue*> >(NTOR, vector<BaseQueue*>(_bundlesize[AGG_TIER])));

    pipes_nlp_ns.resize(NTOR, vector< vector<Pipe*> >(NSRV, vector<Pipe*>(_bundlesize[TOR_TIER])));
    queues_nlp_ns.resize(NTOR, vector< vector<BaseQueue*> >(NSRV, vector<BaseQueue*>(_bundlesize[TOR_TIER])));


    if (_tiers == 3) {
        pipes_nup_nc.resize(NAGG, vector< vector<Pipe*> >(NCORE, vector<Pipe*>(_bundlesize[CORE_TIER])));
        queues_nup_nc.resize(NAGG, vector< vector<BaseQueue*> >(NCORE, vector<BaseQueue*>(_bundlesize[CORE_TIER])));
    }
    
    pipes_nlp_nup.resize(NTOR, vector< vector<Pipe*> >(NAGG, vector<Pipe*>(_bundlesize[AGG_TIER])));
    pipes_ns_nlp.resize(NSRV, vector< vector<Pipe*> >(NTOR, vector<Pipe*>(_bundlesize[TOR_TIER])));
    queues_nlp_nup.resize(NTOR, vector< vector<BaseQueue*> >(NAGG, vector<BaseQueue*>(_bundlesize[AGG_TIER])));
    queues_ns_nlp.resize(NSRV, vector< vector<BaseQueue*> >(NTOR, vector<BaseQueue*>(_bundlesize[TOR_TIER])));
}

BaseQueue* FatTreeTopologyDRB::alloc_src_queue(QueueLogger* queueLogger){
    linkspeed_bps linkspeed = _downlink_speeds[TOR_TIER]; // linkspeeds are symmetric
    switch (_sender_qt) {
    case SWIFT_SCHEDULER:
        return new FairScheduler(linkspeed, *_eventlist, queueLogger);
    case CONST_SCHEDULER:
        return new ConstFairScheduler(linkspeed, *_eventlist, queueLogger);
    case PRIORITY:
        return new PriorityQueue(linkspeed,
                                 memFromPkt(FEEDER_BUFFER), *_eventlist, queueLogger);
    case FAIR_PRIO:
        return new FairPriorityQueue(linkspeed,
                                     memFromPkt(FEEDER_BUFFER), *_eventlist, queueLogger);
    default:
        abort();
    }
}

BaseQueue* FatTreeTopologyDRB::alloc_queue(QueueLogger* queueLogger, mem_b queuesize,
                                        link_direction dir, int switch_tier, bool tor = false){
    if (dir == UPLINK) {
        switch_tier++; // _downlink_speeds is set for the downlinks, so uplinks need to use the tier above's linkspeed
    }
    return alloc_queue(queueLogger, _downlink_speeds[switch_tier], queuesize, dir, switch_tier, tor);
}

BaseQueue*
FatTreeTopologyDRB::alloc_queue(QueueLogger* queueLogger, linkspeed_bps speed, mem_b queuesize,
                             link_direction dir, int switch_tier, bool tor){
    switch (_qt) {
    case RANDOM:
        return new RandomQueue(speed, queuesize, *_eventlist, queueLogger, memFromPkt(RANDOM_BUFFER));
    case COMPOSITE:
    {
        CompositeQueue* q = new CompositeQueue(speed, queuesize, *_eventlist, queueLogger);
        q->setRTS(_rts);
        return q;
    }
    case CTRL_PRIO:
        return new CtrlPrioQueue(speed, queuesize, *_eventlist, queueLogger);
    case AEOLUS:
        return new AeolusQueue(speed, queuesize, FatTreeSwitchDRB::_speculative_threshold_fraction * queuesize,  *_eventlist, queueLogger);
    case AEOLUS_ECN:
        {
            AeolusQueue* q = new AeolusQueue(speed, queuesize, FatTreeSwitchDRB::_speculative_threshold_fraction * queuesize ,  *_eventlist, queueLogger);
            if (!tor || dir == UPLINK) {
                // don't use ECN on ToR downlinks
                q->set_ecn_threshold(FatTreeSwitchDRB::_ecn_threshold_fraction * queuesize);
            }
            return q;
        }
    case ECN:
        return new ECNQueue(speed, queuesize, *_eventlist, queueLogger, FatTreeSwitchDRB::_ecn_threshold_fraction * queuesize);
    case ECN_PRIO:
        return new ECNPrioQueue(speed, queuesize, queuesize,
                                FatTreeSwitchDRB::_ecn_threshold_fraction * queuesize,
                                FatTreeSwitchDRB::_ecn_threshold_fraction * queuesize,
                                *_eventlist, queueLogger);
    case LOSSLESS:
        return new LosslessQueue(speed, queuesize, *_eventlist, queueLogger, NULL);
    case LOSSLESS_INPUT:
        return new LosslessOutputQueue(speed, queuesize, *_eventlist, queueLogger);
    case LOSSLESS_INPUT_ECN: 
        return new LosslessOutputQueue(speed, queuesize*10, *_eventlist, queueLogger,1,FatTreeSwitchDRB::_ecn_threshold_fraction * queuesize);
    case COMPOSITE_ECN:
        if (tor && dir == DOWNLINK) {
            CompositeQueue* q = new CompositeQueue(speed, queuesize, *_eventlist, queueLogger);
            q->setRTS(_rts);
            return q;
        } else {
            return new ECNQueue(speed, queuesize, *_eventlist, queueLogger, FatTreeSwitchDRB::_ecn_threshold_fraction * queuesize);
            // return new ECNQueue(speed, memFromPkt(2*SWITCH_BUFFER), *_eventlist, queueLogger, memFromPkt(15));
        }
    case COMPOSITE_ECN_DEF:
        if (tor && dir == DOWNLINK) {
            CompositeQueue* q = new CompositeQueue(speed, queuesize, *_eventlist, queueLogger);
            q->setRTS(_rts);
            return q;
        } else {
            // return new ECNQueue(speed, queuesize, *_eventlist, queueLogger, FatTreeSwitchDRB::_ecn_threshold_fraction * queuesize);
            return new ECNQueue(speed, memFromPkt(2*SWITCH_BUFFER), *_eventlist, queueLogger, memFromPkt(15));
        }
    case ECN_BIG:
        if (tor && dir == DOWNLINK) {
            return new ECNQueue(speed, queuesize, *_eventlist, queueLogger, FatTreeSwitchDRB::_ecn_threshold_fraction * queuesize);
        } else {
            return new ECNQueue(speed, memFromPkt(2*SWITCH_BUFFER), *_eventlist, queueLogger, memFromPkt(15));
        }
    case COMPOSITE_ECN_LB:
        {
            CompositeQueue* q = new CompositeQueue(speed, queuesize, *_eventlist, queueLogger);
            if (!tor || dir == UPLINK) {
                // don't use ECN on ToR downlinks
                q->set_ecn_threshold(FatTreeSwitchDRB::_ecn_threshold_fraction * queuesize);
            }
            q->setRTS(_rts);
            return q;
        }
    default:
        abort();
    }
}

void FatTreeTopologyDRB::init_network(){
    QueueLogger* queueLogger;
    if (_tiers == 3) {
        for (uint32_t j=0;j<NCORE;j++) {
            for (uint32_t k=0;k<NAGG;k++) {
                for (uint32_t b = 0; b < _bundlesize[CORE_TIER]; b++) {
                    queues_nc_nup[j][k][b] = NULL;
                    pipes_nc_nup[j][k][b] = NULL;
                    queues_nup_nc[k][j][b] = NULL;
                    pipes_nup_nc[k][j][b] = NULL;
                }
            }
        }
    }
    
    for (uint32_t j=0;j<NAGG;j++) {
        for (uint32_t k=0;k<NTOR;k++) {
            for (uint32_t b = 0; b < _bundlesize[AGG_TIER]; b++) {
                queues_nup_nlp[j][k][b] = NULL;
                pipes_nup_nlp[j][k][b] = NULL;
                queues_nlp_nup[k][j][b] = NULL;
                pipes_nlp_nup[k][j][b] = NULL;
            }
        }
    }
    
    for (uint32_t j=0;j<NTOR;j++) {
        for (uint32_t k=0;k<NSRV;k++) {
            for (uint32_t b = 0; b < _bundlesize[TOR_TIER]; b++) { 
                queues_nlp_ns[j][k][b] = NULL;
                pipes_nlp_ns[j][k][b] = NULL;
                queues_ns_nlp[k][j][b] = NULL;
                pipes_ns_nlp[k][j][b] = NULL;
            }
        }
    }

    //create switches if we have lossless operation
    //if (_qt==LOSSLESS)
    // changed to always create switches
    for (uint32_t j=0;j<NTOR;j++){
        simtime_picosec switch_latency = (_switch_latencies[TOR_TIER] > 0) ? _switch_latencies[TOR_TIER] : _switch_latency;
        switches_lp[j] = new FatTreeSwitchDRB(*_eventlist, "Switch_LowerPod_"+ntoa(j),FatTreeSwitchDRB::TOR,j,switch_latency,this);
    }
    for (uint32_t j=0;j<NAGG;j++){
        simtime_picosec switch_latency = (_switch_latencies[AGG_TIER] > 0) ? _switch_latencies[AGG_TIER] : _switch_latency;
        switches_up[j] = new FatTreeSwitchDRB(*_eventlist, "Switch_UpperPod_"+ntoa(j), FatTreeSwitchDRB::AGG,j,switch_latency,this);
    }
    for (uint32_t j=0;j<NCORE;j++){
        simtime_picosec switch_latency = (_switch_latencies[CORE_TIER] > 0) ? _switch_latencies[CORE_TIER] : _switch_latency;
        switches_c[j] = new FatTreeSwitchDRB(*_eventlist, "Switch_Core_"+ntoa(j), FatTreeSwitchDRB::CORE,j,switch_latency,this);
    }
      
    // links from lower layer pod switch to server
    for (uint32_t tor = 0; tor < NTOR; tor++) {
        uint32_t link_bundles = _radix_down[TOR_TIER]/_bundlesize[TOR_TIER];
        for (uint32_t l = 0; l < link_bundles; l++) {
            uint32_t srv = tor * link_bundles + l;
            for (uint32_t b = 0; b < _bundlesize[TOR_TIER]; b++) {
                // Downlink
                if (_logger_factory) {
                    queueLogger = _logger_factory->createQueueLogger();
                } else {
                    queueLogger = NULL;
                }
            
                queues_nlp_ns[tor][srv][b] = alloc_queue(queueLogger, _queue_down[TOR_TIER], DOWNLINK, TOR_TIER, true);
                queues_nlp_ns[tor][srv][b]->setName("LS" + ntoa(tor) + "->DST" +ntoa(srv) + "(" + ntoa(b) + ")");
                //if (logfile) logfile->writeName(*(queues_nlp_ns[tor][srv]));
                simtime_picosec hop_latency = (_hop_latency == 0) ? _link_latencies[TOR_TIER] : _hop_latency;
                pipes_nlp_ns[tor][srv][b] = new Pipe(hop_latency, *_eventlist);
                pipes_nlp_ns[tor][srv][b]->setName("Pipe-LS" + ntoa(tor)  + "->DST" + ntoa(srv) + "(" + ntoa(b) + ")");
                //if (logfile) logfile->writeName(*(pipes_nlp_ns[tor][srv]));
            
                // Uplink
                if (_logger_factory) {
                    queueLogger = _logger_factory->createQueueLogger();
                } else {
                    queueLogger = NULL;
                }
                queues_ns_nlp[srv][tor][b] = alloc_src_queue(queueLogger);   
                queues_ns_nlp[srv][tor][b]->setName("SRC" + ntoa(srv) + "->LS" +ntoa(tor) + "(" + ntoa(b) + ")");
                //cout << queues_ns_nlp[srv][tor][b]->str() << endl;
                //if (logfile) logfile->writeName(*(queues_ns_nlp[srv][tor]));

                queues_ns_nlp[srv][tor][b]->setRemoteEndpoint(switches_lp[tor]);

                assert(switches_lp[tor]->addPort(queues_nlp_ns[tor][srv][b]) < 96);

                if (_qt==LOSSLESS_INPUT || _qt == LOSSLESS_INPUT_ECN){
                    //no virtual queue needed at server
                    new LosslessInputQueue(*_eventlist, queues_ns_nlp[srv][tor][b], switches_lp[tor], _hop_latency);
                }
        
                pipes_ns_nlp[srv][tor][b] = new Pipe(hop_latency, *_eventlist);
                pipes_ns_nlp[srv][tor][b]->setName("Pipe-SRC" + ntoa(srv) + "->LS" + ntoa(tor) + "(" + ntoa(b) + ")");
                //if (logfile) logfile->writeName(*(pipes_ns_nlp[srv][tor]));
            
                if (ff){
                    ff->add_queue(queues_nlp_ns[tor][srv][b]);
                    ff->add_queue(queues_ns_nlp[srv][tor][b]);
                }
            }
        }
    }

    //Lower layer in pod to upper layer in pod!
    for (uint32_t tor = 0; tor < NTOR; tor++) {
        uint32_t podid = tor/_tor_switches_per_pod;
        uint32_t agg_min, agg_max;
        if (_tiers == 3) {
            //Connect the lower layer switch to the upper layer switches in the same pod
            agg_min = MIN_POD_AGG_SWITCH(podid);
            agg_max = MAX_POD_AGG_SWITCH(podid);
        } else {
            //Connect the lower layer switch to all upper layer switches
            assert(_tiers == 2);
            agg_min = 0;
            agg_max = NAGG-1;
        }
        for (uint32_t agg=agg_min; agg<=agg_max; agg++){
            for (uint32_t b = 0; b < _bundlesize[AGG_TIER]; b++) {
                // Downlink
                if (_logger_factory) {
                    queueLogger = _logger_factory->createQueueLogger();
                } else {
                    queueLogger = NULL;
                }
                queues_nup_nlp[agg][tor][b] = alloc_queue(queueLogger, _queue_down[AGG_TIER], DOWNLINK, AGG_TIER);
                queues_nup_nlp[agg][tor][b]->setName("US" + ntoa(agg) + "->LS_" + ntoa(tor) + "(" + ntoa(b) + ")");
                //if (logfile) logfile->writeName(*(queues_nup_nlp[agg][tor]));
            
                simtime_picosec hop_latency = (_hop_latency == 0) ? _link_latencies[AGG_TIER] : _hop_latency;
                pipes_nup_nlp[agg][tor][b] = new Pipe(hop_latency, *_eventlist);
                pipes_nup_nlp[agg][tor][b]->setName("Pipe-US" + ntoa(agg) + "->LS" + ntoa(tor) + "(" + ntoa(b) + ")");
                //if (logfile) logfile->writeName(*(pipes_nup_nlp[agg][tor]));
            
                // Uplink
                if (_logger_factory) {
                    queueLogger = _logger_factory->createQueueLogger();
                } else {
                    queueLogger = NULL;
                }
                queues_nlp_nup[tor][agg][b] = alloc_queue(queueLogger, _queue_up[TOR_TIER], UPLINK, TOR_TIER, true);
                queues_nlp_nup[tor][agg][b]->setName("LS" + ntoa(tor) + "->US" + ntoa(agg) + "(" + ntoa(b) + ")");
                //cout << queues_nlp_nup[tor][agg][b]->str() << endl;
                //if (logfile) logfile->writeName(*(queues_nlp_nup[tor][agg]));

                assert(switches_lp[tor]->addPort(queues_nlp_nup[tor][agg][b]) < 96);
                assert(switches_up[agg]->addPort(queues_nup_nlp[agg][tor][b]) < 64);
                queues_nlp_nup[tor][agg][b]->setRemoteEndpoint(switches_up[agg]);
                queues_nup_nlp[agg][tor][b]->setRemoteEndpoint(switches_lp[tor]);

                /*if (_qt==LOSSLESS){
                  ((LosslessQueue*)queues_nlp_nup[tor][agg])->setRemoteEndpoint(queues_nup_nlp[agg][tor]);
                  ((LosslessQueue*)queues_nup_nlp[agg][tor])->setRemoteEndpoint(queues_nlp_nup[tor][agg]);
                  }else */
                if (_qt==LOSSLESS_INPUT || _qt == LOSSLESS_INPUT_ECN){            
                    new LosslessInputQueue(*_eventlist, queues_nlp_nup[tor][agg][b],switches_up[agg],_hop_latency);
                    new LosslessInputQueue(*_eventlist, queues_nup_nlp[agg][tor][b],switches_lp[tor],_hop_latency);
                }
        
                pipes_nlp_nup[tor][agg][b] = new Pipe(hop_latency, *_eventlist);
                pipes_nlp_nup[tor][agg][b]->setName("Pipe-LS" + ntoa(tor) + "->US" + ntoa(agg) + "(" + ntoa(b) + ")");
                //if (logfile) logfile->writeName(*(pipes_nlp_nup[tor][agg]));
        
                if (ff){
                    ff->add_queue(queues_nlp_nup[tor][agg][b]);
                    ff->add_queue(queues_nup_nlp[agg][tor][b]);
                }
            }
        }
    }

    /*for (int32_t i = 0;i<NK;i++){
      for (uint32_t j = 0;j<NK;j++){
      printf("%p/%p ",queues_nlp_nup[i][j], queues_nup_nlp[j][i]);
      }
      printf("\n");
      }*/
    
    // Upper layer in pod to core
    if (_tiers == 3) {
        for (uint32_t agg = 0; agg < NAGG; agg++) {
            uint32_t podpos = agg%(_agg_switches_per_pod);
            for (uint32_t l = 0; l < _radix_up[AGG_TIER]/_bundlesize[CORE_TIER]; l++) {
                uint32_t core = podpos +  _agg_switches_per_pod * l;
                assert(core < NCORE);
                for (uint32_t b = 0; b < _bundlesize[CORE_TIER]; b++) {
                
                    // Downlink
                    if (_logger_factory) {
                        queueLogger = _logger_factory->createQueueLogger();
                    } else {
                        queueLogger = NULL;
                    }
                    assert(queues_nup_nc[agg][core][b] == NULL);
                    queues_nup_nc[agg][core][b] = alloc_queue(queueLogger, _queue_up[AGG_TIER], UPLINK, AGG_TIER);
                    queues_nup_nc[agg][core][b]->setName("US" + ntoa(agg) + "->CS" + ntoa(core) + "(" + ntoa(b) + ")");
                    //cout << queues_nup_nc[agg][core][b]->str() << endl;
                    //if (logfile) logfile->writeName(*(queues_nup_nc[agg][core]));
        
                    simtime_picosec hop_latency = (_hop_latency == 0) ? _link_latencies[CORE_TIER] : _hop_latency;
                    pipes_nup_nc[agg][core][b] = new Pipe(hop_latency, *_eventlist);
                    pipes_nup_nc[agg][core][b]->setName("Pipe-US" + ntoa(agg) + "->CS" + ntoa(core) + "(" + ntoa(b) + ")");
                    //if (logfile) logfile->writeName(*(pipes_nup_nc[agg][core]));
        
                    // Uplink
                    if (_logger_factory) {
                        queueLogger = _logger_factory->createQueueLogger();
                    } else {
                        queueLogger = NULL;
                    }
        
                    if ((l+agg*_agg_switches_per_pod)<failed_links){ // -> l < failed_links does this per-pod TODO: FIX - THESE FAILURES ARE NOT SYMMETRICAL - SEE ABOVE
                        double bw_pct = fail_bw_pct;
                        if (fail_bw_pct == 0.0) { // TODO: clean this up
                            bw_pct = 1.0;
                        }
                        BaseQueue* q = alloc_queue(queueLogger, _downlink_speeds[CORE_TIER]*bw_pct, _queue_down[CORE_TIER],
                                                               DOWNLINK, CORE_TIER, false);
                        if (fail_bw_pct == 0.0) {
                            q->setDropAll(true);
                        }
                        queues_nc_nup[core][agg][b] = q;
                        cout << "Adding link failure for agg_sw " << ntoa(agg) << " l " << ntoa(l) << " b " << ntoa(b) << " bw " << _downlink_speeds[CORE_TIER]*fail_bw_pct << " drop all " << q->getDropAll() <<  endl;
                    } else if ((l+agg*_agg_switches_per_pod)<flaky_links) {
                        BaseQueue* q = alloc_queue(queueLogger, _downlink_speeds[CORE_TIER], _queue_down[CORE_TIER],
                                                               DOWNLINK, CORE_TIER, false);
                        queues_nc_nup[core][agg][b] = q;
                        q->setBurstyLossParameters(_link_loss_burst_interarrival_time, _link_loss_burst_duration);
                    } else {
                        queues_nc_nup[core][agg][b] = alloc_queue(queueLogger, _queue_down[CORE_TIER], DOWNLINK, CORE_TIER);
                    }
        
                    queues_nc_nup[core][agg][b]->setName("CS" + ntoa(core) + "->US" + ntoa(agg) + "(" + ntoa(b) + ")");

                    assert(switches_up[agg]->addPort(queues_nup_nc[agg][core][b]) < 64);
                    assert(switches_c[core]->addPort(queues_nc_nup[core][agg][b]) < 64);
                    queues_nup_nc[agg][core][b]->setRemoteEndpoint(switches_c[core]);
                    queues_nc_nup[core][agg][b]->setRemoteEndpoint(switches_up[agg]);

                    /*if (_qt==LOSSLESS){
                      ((LosslessQueue*)queues_nup_nc[agg][core])->setRemoteEndpoint(queues_nc_nup[core][agg]);
                      ((LosslessQueue*)queues_nc_nup[core][agg])->setRemoteEndpoint(queues_nup_nc[agg][core]);
                      }
                      else*/
                    if (_qt == LOSSLESS_INPUT || _qt == LOSSLESS_INPUT_ECN){
                        new LosslessInputQueue(*_eventlist, queues_nup_nc[agg][core][b], switches_c[core], _hop_latency);
                        new LosslessInputQueue(*_eventlist, queues_nc_nup[core][agg][b], switches_up[agg], _hop_latency);
                    }
                    //if (logfile) logfile->writeName(*(queues_nc_nup[core][agg]));
            
                    pipes_nc_nup[core][agg][b] = new Pipe(hop_latency, *_eventlist);
                    pipes_nc_nup[core][agg][b]->setName("Pipe-CS" + ntoa(core) + "->US" + ntoa(agg) + "(" + ntoa(b) + ")");
                    //if (logfile) logfile->writeName(*(pipes_nc_nup[core][agg]));
            
                    if (ff){
                        ff->add_queue(queues_nup_nc[agg][core][b]);
                        ff->add_queue(queues_nc_nup[core][agg][b]);
                    }
                }
            }
        }
    }

    for (const ParsedLinkInfo& info : _parsed_failed_links) {
        if (info.type == ParsedLinkInfo::TOR_HOST_LINK) {
            // ToR-Host links
            uint32_t tor = info.tor_id;
            uint32_t host = info.host_id;
            if (tor < NTOR && host < NSRV) {
                for (uint32_t b = 0; b < _bundlesize[TOR_TIER]; b++) {
                    if (queues_nlp_ns[tor][host][b]) {
                        queues_nlp_ns[tor][host][b]->setFailed(true);
                        queues_nlp_ns[tor][host][b]->setDropAll(true);
                        cout << "Failing ToR-Host Downlink: tor" << tor << "_host" << host << "_bundle" << b << endl;
                    }
                    if (queues_ns_nlp[host][tor][b]) {
                        queues_ns_nlp[host][tor][b]->setFailed(true);
                        queues_ns_nlp[host][tor][b]->setDropAll(true);
                        cout << "Failing Host-ToR Uplink: host" << host << "_tor" << tor << "_bundle" << b << endl;
                    }
                }
            } else {
                cerr << "Warning: Failed to apply TOR_HOST_LINK failure - invalid IDs (tor=" << tor << ", host=" << host << ")" << endl;
            }
        } else if (info.type == ParsedLinkInfo::TOR_AGG_LINK) {
            // ToR-Agg links (user's "podX_spineY_leafZ" refers to agg Y in pod X, tor Z in pod X)
            uint32_t pod_id = info.pod_id;
            uint32_t spine_idx_in_pod = info.spine_idx_in_pod; // This is agg_id within pod
            uint32_t leaf_idx_in_pod = info.leaf_idx_in_pod;   // This is tor_id within pod

            if (pod_id < NPOD && spine_idx_in_pod < _agg_switches_per_pod && leaf_idx_in_pod < _tor_switches_per_pod) {
                uint32_t tor_global_id = MIN_POD_TOR_SWITCH(pod_id) + leaf_idx_in_pod;
                uint32_t agg_global_id = MIN_POD_AGG_SWITCH(pod_id) + spine_idx_in_pod;

                for (uint32_t b = 0; b < _bundlesize[AGG_TIER]; b++) {
                    if (queues_nlp_nup[tor_global_id][agg_global_id][b]) {
                        queues_nlp_nup[tor_global_id][agg_global_id][b]->setFailed(true);
                        queues_nlp_nup[tor_global_id][agg_global_id][b]->setDropAll(true);
                        cout << "Failing ToR-Agg Uplink: pod" << pod_id << "_spine" << spine_idx_in_pod << "_leaf" << leaf_idx_in_pod << "_bundle" << b << endl;
                    }
                    if (queues_nup_nlp[agg_global_id][tor_global_id][b]) {
                        queues_nup_nlp[agg_global_id][tor_global_id][b]->setFailed(true);
                        queues_nup_nlp[agg_global_id][tor_global_id][b]->setDropAll(true);
                        cout << "Failing Agg-ToR Downlink: pod" << pod_id << "_spine" << spine_idx_in_pod << "_leaf" << leaf_idx_in_pod << "_bundle" << b << endl;
                    }
                }
            } else {
                cerr << "Warning: Failed to apply TOR_AGG_LINK failure - invalid IDs (pod=" << pod_id << ", spine_in_pod=" << spine_idx_in_pod << ", leaf_in_pod=" << leaf_idx_in_pod << ")" << endl;
            }
        } else if (info.type == ParsedLinkInfo::AGG_CORE_LINK) {
            // Agg-Core links (user's "podX_spineY_coreZ" refers to agg Y in pod X, core Z)
            uint32_t pod_id = info.pod_id;
            uint32_t spine_idx_in_pod = info.spine_idx_in_pod; // This is agg_id within pod
            uint32_t core_id_connected_to_spine = info.core_id_connected_to_spine; // This is global core_id

            if (pod_id < NPOD && spine_idx_in_pod < _agg_switches_per_pod && core_id_connected_to_spine < NCORE) {
                uint32_t agg_global_id = MIN_POD_AGG_SWITCH(pod_id) + spine_idx_in_pod;
                uint32_t core_global_id = core_id_connected_to_spine; // Core ID is already global

                for (uint32_t b = 0; b < _bundlesize[CORE_TIER]; b++) {
                    if (queues_nup_nc[agg_global_id][core_global_id][b]) {
                        queues_nup_nc[agg_global_id][core_global_id][b]->setFailed(true);
                        queues_nup_nc[agg_global_id][core_global_id][b]->setDropAll(true);
                        cout << "Failing Agg-Core Uplink: pod" << pod_id << "_spine" << spine_idx_in_pod << "_core" << core_id_connected_to_spine << "_bundle" << b << endl;
                    }
                    if (queues_nc_nup[core_global_id][agg_global_id][b]) {
                        queues_nc_nup[core_global_id][agg_global_id][b]->setFailed(true);
                        queues_nc_nup[core_global_id][agg_global_id][b]->setDropAll(true);
                        cout << "Failing Core-Agg Downlink: pod" << pod_id << "_spine" << spine_idx_in_pod << "_core" << core_id_connected_to_spine << "_bundle" << b << endl;
                    }
                }
            } else {
                cerr << "Warning: Failed to apply AGG_CORE_LINK failure - invalid IDs (pod=" << pod_id << ", spine_in_pod=" << spine_idx_in_pod << ", core=" << core_id_connected_to_spine << ")" << endl;
            }
        } else {
            cerr << "Warning: Encountered unknown or unhandled ParsedLinkInfo type during failure application." << endl;
        }
    }

    /*    for (uint32_t i = 0;i<NK;i++){
          for (uint32_t j = 0;j<NC;j++){
          printf("%p/%p ",queues_nup_nc[i][agg], queues_nc_nup[agg][i]);
          }
          printf("\n");
          }*/

    precompute_drb_paths();
    
    //init thresholds for lossless operation
    if (_qt==LOSSLESS) {
        for (uint32_t j=0;j<NTOR;j++){
            switches_lp[j]->configureLossless();
        }
        for (uint32_t j=0;j<NAGG;j++){
            switches_up[j]->configureLossless();
        }
        for (uint32_t j=0;j<NCORE;j++){
            switches_c[j]->configureLossless();
        }
    }
}

void FatTreeTopologyDRB::precompute_drb_paths() {
    // --- DRB: Pre-compute valid spine/core switches based on static failures ---
    // This happens after all links are created and initial failures are applied.

    // Populate _valid_spine_switches_cache (Case 3: Same Pod, Different ToR)
    for (uint32_t pod_id = 0; pod_id < NPOD; ++pod_id) {
        for (uint32_t src_tor_in_pod = 0; src_tor_in_pod < _tor_switches_per_pod; ++src_tor_in_pod) {
            uint32_t src_tor = MIN_POD_TOR_SWITCH(pod_id) + src_tor_in_pod;
            for (uint32_t dest_tor_in_pod = 0; dest_tor_in_pod < _tor_switches_per_pod; ++dest_tor_in_pod) {
                uint32_t dest_tor = MIN_POD_TOR_SWITCH(pod_id) + dest_tor_in_pod;

                if (src_tor == dest_tor) {
                    continue; // Handled by Case 2, no spine needed
                }

                std::vector<uint32_t> reachable_spines;
                for (uint32_t spine_idx_in_pod = 0; spine_idx_in_pod < _agg_switches_per_pod; ++spine_idx_in_pod) {
                    uint32_t selected_agg_switch = MIN_POD_AGG_SWITCH(pod_id) + spine_idx_in_pod;

                    // Check reachability for the path: src_tor -> selected_agg_switch -> dest_tor
                    // Using bundle index 0 for this check, assuming failures apply per bundle element.
                    // If any element is NULL or failed, the path is not reachable.
                    if (queues_nlp_nup[src_tor][selected_agg_switch][0] && !queues_nlp_nup[src_tor][selected_agg_switch][0]->getFailed() &&
                        queues_nup_nlp[selected_agg_switch][dest_tor][0] && !queues_nup_nlp[selected_agg_switch][dest_tor][0]->getFailed()) {
                        reachable_spines.push_back(selected_agg_switch);
                    }
                }
                _valid_spine_switches_cache[{src_tor, dest_tor}] = reachable_spines;
                // Optional: Print if no path found, for debugging
                // if (reachable_spines.empty()) {
                //     cout << "DRB Warning: No valid spine path for src_tor " << src_tor << " to dest_tor " << dest_tor << " in pod " << pod_id << endl;
                // }
            }
        }
    }

    // Populate _valid_core_switches_cache (Case 4: Different Pods)
    if (_tiers == 3) { // Only applicable for 3-tier fat trees
        for (uint32_t src_pod = 0; src_pod < NPOD; ++src_pod) {
            for (uint32_t dest_pod = 0; dest_pod < NPOD; ++dest_pod) {
                if (src_pod == dest_pod) {
                    continue; // Handled by Case 3
                }

                for (uint32_t src_tor_in_pod = 0; src_tor_in_pod < _tor_switches_per_pod; ++src_tor_in_pod) {
                    uint32_t src_tor = MIN_POD_TOR_SWITCH(src_pod) + src_tor_in_pod;
                    for (uint32_t dest_tor_in_pod = 0; dest_tor_in_pod < _tor_switches_per_pod; ++dest_tor_in_pod) {
                        uint32_t dest_tor = MIN_POD_TOR_SWITCH(dest_pod) + dest_tor_in_pod;

                        std::vector<uint32_t> reachable_cores;
                        for (uint32_t selected_core = 0; selected_core < NCORE; ++selected_core) {
                            uint32_t src_agg_switch = MIN_POD_AGG_SWITCH(src_pod) + (selected_core % _agg_switches_per_pod);
                            uint32_t dest_agg_switch = MIN_POD_AGG_SWITCH(dest_pod) + (selected_core % _agg_switches_per_pod);
                            
                            // Ensure the determined agg switches are within their respective pods
                            if (src_agg_switch < MIN_POD_AGG_SWITCH(src_pod) || src_agg_switch > MAX_POD_AGG_SWITCH(src_pod) ||
                                dest_agg_switch < MIN_POD_AGG_SWITCH(dest_pod) || dest_agg_switch > MAX_POD_AGG_SWITCH(dest_pod)) {
                                // This core cannot be reached from/to these specific pods via a valid agg switch
                                continue;
                            }

                            // Check reachability for the path: src_tor -> src_agg_switch -> selected_core -> dest_agg_switch -> dest_tor
                            // Using bundle index 0 for this check.
                            if (queues_nlp_nup[src_tor][src_agg_switch][0] && !queues_nlp_nup[src_tor][src_agg_switch][0]->getFailed() &&
                                queues_nup_nc[src_agg_switch][selected_core][0] && !queues_nup_nc[src_agg_switch][selected_core][0]->getFailed() &&
                                queues_nc_nup[selected_core][dest_agg_switch][0] && !queues_nc_nup[selected_core][dest_agg_switch][0]->getFailed() &&
                                queues_nup_nlp[dest_agg_switch][dest_tor][0] && !queues_nup_nlp[dest_agg_switch][dest_tor][0]->getFailed()) {
                                reachable_cores.push_back(selected_core);
                            }
                        }
                        _valid_core_switches_cache[{src_tor, dest_tor}] = reachable_cores;
                        // Optional: Print if no path found, for debugging
                        // if (reachable_cores.empty()) {
                        //     cout << "DRB Warning: No valid core path for src_tor " << src_tor << " to dest_tor " << dest_tor << " (src_pod " << src_pod << ", dest_pod " << dest_pod << ")" << endl;
                        // }
                    }
                }
            }
        }
    }
    // --- End DRB Pre-computation ---
}

void FatTreeTopologyDRB::add_failed_link(uint32_t type, uint32_t switch_id, uint32_t link_id){
    assert(type == FatTreeSwitchDRB::AGG);
    assert(link_id < _radix_up[AGG_TIER]);
    assert(switch_id < NAGG);
    
    uint32_t podpos = switch_id%(_agg_switches_per_pod);
    uint32_t k = podpos * _agg_switches_per_pod + link_id;

    // note: if bundlesize > 1, we only fail the first link in a bundle.
    std::cout << "Failing the link between agg switch " << switch_id << " and core switch " <<  k << std::endl;
    
    assert(queues_nup_nc[switch_id][k][0]!=NULL && queues_nc_nup[k][switch_id][0]!=NULL );
    queues_nup_nc[switch_id][k][0] = NULL;
    queues_nc_nup[k][switch_id][0] = NULL;

    assert(pipes_nup_nc[switch_id][k][0]!=NULL && pipes_nc_nup[k][switch_id][0]);
    pipes_nup_nc[switch_id][k][0] = NULL;
    pipes_nc_nup[k][switch_id][0] = NULL;
}

void FatTreeTopologyDRB::add_failed_link(uint32_t type, uint32_t switch_id, uint32_t link_id, FailType failure_type){
    assert(type == FatTreeSwitchDRB::AGG);
    assert(link_id < _radix_up[AGG_TIER]);
    assert(switch_id < NAGG);
    
    uint32_t podpos = switch_id%(_agg_switches_per_pod);
    uint32_t k = podpos + (_agg_switches_per_pod * link_id); // l * _ft->agg_switches_per_pod() + podpos;

    // note: if bundlesize > 1, we only fail the first link in a bundle.
    std::cout << "Failing the link between agg switch " << switch_id << " and core switch " <<  k << std::endl;
    
    assert(queues_nup_nc[switch_id][k][0]!=NULL && queues_nc_nup[k][switch_id][0]!=NULL );
    if (failure_type == BLACK_HOLE_DROP) {
        queues_nup_nc[switch_id][k][0]->setDropAll(true);
        queues_nc_nup[k][switch_id][0]->setDropAll(true);
    } else if (failure_type == BLACK_HOLE_QUEUE) {
        queues_nup_nc[switch_id][k][0]->changeRate(speedFromMbps(0.00001));
        queues_nc_nup[k][switch_id][0]->changeRate(speedFromMbps(0.00001));
    }
    queues_nup_nc[switch_id][k][0]->setFailed(true);
    queues_nc_nup[k][switch_id][0]->setFailed(true);

}

void FatTreeTopologyDRB::restore_failed_link(uint32_t type, uint32_t switch_id, uint32_t link_id){
    assert(type == FatTreeSwitchDRB::AGG);
    assert(link_id < _radix_up[AGG_TIER]);
    assert(switch_id < NAGG);
    
    uint32_t podpos = switch_id%(_agg_switches_per_pod);
    uint32_t k = podpos + (_agg_switches_per_pod * link_id);

    BaseQueue* agg_queue = queues_nup_nc[switch_id][k][0];
    BaseQueue* core_queue = queues_nc_nup[k][switch_id][0];
    agg_queue->setFailed(false);
    core_queue->setFailed(false);
    agg_queue->setDropAll(false);
    core_queue->setDropAll(false);
    agg_queue->changeRate(_downlink_speeds[CORE_TIER]);
    core_queue->changeRate(_downlink_speeds[CORE_TIER]);
}

void FatTreeTopologyDRB::reset_routes(uint32_t switch_id) {
    // Get the appropriate switch and clear its FIB
    Switch* switch_up = switches_up[switch_id];
    switch_up->resetFib(); //clears all current routes to be repopulated
    // Let the swtich re-populate its FIB as traffic arrives (will check if links are down)  
}

void FatTreeTopologyDRB::reset_weights() {
    // For all tor switches, turn off weighting
    for (int i = 0; i < tor_switches_per_pod() * no_of_pods(); i++) {
        FatTreeSwitchDRB* tor = (FatTreeSwitchDRB*) switches_lp[i];
        tor->setFibWeighted(false); // todo: maybe clear the weighted fib too in case it is used again later
    }
}

void FatTreeTopologyDRB::update_routes(uint32_t switch_id) {
    // Get the appropriate switch and clear its FIB
    Switch* switch_up = switches_up[switch_id];
    switch_up->resetFib(); //clears all current routes to be repopulated
    // Let the swtich re-populate its FIB as traffic arrives (will check if links are down)  
}

void FatTreeTopologyDRB::update_weights(uint32_t switch_id, map<string,int> link_weights) {
    // Get the appropriate switch
    FatTreeSwitchDRB* switch_lp = (FatTreeSwitchDRB*)switches_lp[switch_id];
    // Set the link weights on the switch
    switch_lp->setWeights(link_weights);
    // Apply weights to existing routes (if any)
    switch_lp->applyWeights();
}

void FatTreeTopologyDRB::update_weights_tor_podfailed(int failed_pod) { 
    uint32_t first_tor = MIN_POD_TOR_SWITCH(failed_pod);
    uint32_t first_agg = MIN_POD_AGG_SWITCH(failed_pod);
    // Get the weight for each agg switch in the pod
    int total_weight = 0;
    map<string, int> switch_weights = {};
    for (int j = 0; j < agg_switches_per_pod(); j++) {
        uint32_t agg = first_agg + j;
        int switch_weight = ((FatTreeSwitchDRB*)switches_up[agg])->countActiveUpPorts();
        switch_weights[switches_up[agg]->nodename()] = switch_weight;
        total_weight += switch_weight;
    }
    // // Assign weights to links on ToR based on agg switch weights
    for (int i = 0; i < tor_switches_per_pod(); i++) {
        uint32_t tor = first_tor + i;
        update_weights(tor, switch_weights);
    }

}

void FatTreeTopologyDRB::update_weights_tor_otherpod(int failed_pod) { // this should be for just the destinations that are in the other pod
    for (int p = 0; p < no_of_pods(); p++) {
        if (p == failed_pod) {
            continue;
        }
        uint32_t first_tor = MIN_POD_TOR_SWITCH(p);
        uint32_t first_agg = MIN_POD_AGG_SWITCH(p);
        // Get the weight for each agg switch in the pod
        int total_weight = 0;
        map<string, int> switch_weights = {};
        for (int j = 0; j < agg_switches_per_pod(); j++) {
            uint32_t agg = first_agg + j;
            int switch_weight = ((FatTreeSwitchDRB*)switches_up[agg])->countActiveRoutesToPod(failed_pod);
            switch_weights[switches_up[agg]->nodename()] = switch_weight;
            total_weight += switch_weight;
        }
        // Assign weights to links on ToR based on agg switch weights
        for (int i = 0; i < tor_switches_per_pod(); i++) {
            uint32_t tor = first_tor + i;
            update_weights(tor, switch_weights);
        }
    }
}

void FatTreeTopologyDRB::populate_all_routes() {
    for (Switch* s : switches_lp) {
        FatTreeSwitchDRB* ft = (FatTreeSwitchDRB*) s;
        ft->forcePopulateRoutes();
    } 
    for (Switch* s : switches_up) {
        FatTreeSwitchDRB* ft = (FatTreeSwitchDRB*) s;
        ft->forcePopulateRoutes();
    } for (Switch* s : switches_c) {
        FatTreeSwitchDRB* ft = (FatTreeSwitchDRB*) s;
        ft->forcePopulateRoutes();
    }
}

void FatTreeTopologyDRB::add_symmetric_failures(int failures) {
    for (uint32_t p = 0; p < NPOD; p++) {
        for (uint32_t i = 0; i < (uint32_t)failures; i++) {
            uint32_t switch_id = _agg_switches_per_pod * p + (int)(i / _radix_up[AGG_TIER]);
            add_failed_link(FatTreeSwitchDRB::AGG, switch_id, i % _radix_up[AGG_TIER]);
        }
    }
    
}

uint32_t FatTreeTopologyDRB::get_drb_path_count(uint32_t src, uint32_t dest) {
    uint32_t src_tor = HOST_POD_SWITCH(src);
    uint32_t dest_tor = HOST_POD_SWITCH(dest);
    uint32_t src_pod = HOST_POD(src);
    uint32_t dest_pod = HOST_POD(dest);

    if (src_tor == dest_tor) {
        return 1;
    }
    
    if (src_pod == dest_pod) {
        // Same pod, different ToR: check valid spines
        auto it = _valid_spine_switches_cache.find({src_tor, dest_tor});
        if (it != _valid_spine_switches_cache.end()) {
            return it->second.size();
        } else {
            // Should not happen if init_network ran, but fallback
            return _agg_switches_per_pod; 
        }
    } else {
        // Different pods: check valid cores
        if (_tiers == 3) {
            auto it = _valid_core_switches_cache.find({src_tor, dest_tor});
            if (it != _valid_core_switches_cache.end()) {
                return it->second.size();
            } else {
                return NCORE;
            }
        } else {
            // 2-tier logic if needed, though DRB usually implies 3-tier here
            return 1; 
        }
    }
}

std::vector<uint32_t> FatTreeTopologyDRB::get_reachable_cores(uint32_t switch_id, int switch_type) const {
    std::vector<uint32_t> reachable_cores;
    if (switch_type == FatTreeSwitchDRB::AGG) {
        // Iterate uplinks to cores
        uint32_t podpos = switch_id % _agg_switches_per_pod;
        uint32_t uplink_bundles = _radix_up[AGG_TIER] / _bundlesize[CORE_TIER];
        for (uint32_t l = 0; l < uplink_bundles; l++) {
            uint32_t core = l * _agg_switches_per_pod + podpos;
            for (uint32_t b = 0; b < _bundlesize[CORE_TIER]; b++) {
                if (queues_nup_nc[switch_id][core][b] && !queues_nup_nc[switch_id][core][b]->getFailed()) {
                    reachable_cores.push_back(core);
                    break; // Count core once if any link in bundle is up
                }
            }
        }
    } else if (switch_type == FatTreeSwitchDRB::TOR) {
        // Iterate uplinks to spines, recurse
        uint32_t podid = switch_id / _tor_switches_per_pod;
        uint32_t agg_min = podid * _agg_switches_per_pod; // MIN_POD_AGG_SWITCH
        uint32_t agg_max = (podid + 1) * _agg_switches_per_pod - 1; // MAX_POD_AGG_SWITCH

        for (uint32_t k = agg_min; k <= agg_max; k++) {
            if (queues_nlp_nup[switch_id][k][0] && !queues_nlp_nup[switch_id][k][0]->getFailed()) {
                std::vector<uint32_t> cores_from_spine = get_reachable_cores(k, FatTreeSwitchDRB::AGG);
                reachable_cores.insert(reachable_cores.end(), cores_from_spine.begin(), cores_from_spine.end());
            }
        }
        // Note: duplicates possible if multiple spines reach same core (unlikely in standard FT but possible with failures/multipath)
        // For intersection logic, duplicates don't hurt, or we can unique them. Let's keep it simple.
    }
    return reachable_cores;
}

Route* FatTreeTopologyDRB::get_drb_selected_path(uint32_t src, uint32_t dest, uint32_t rr_index, bool reverse_path) {
    // Input validation
    if (src >= NSRV || dest >= NSRV) {
        cerr << "DRB Error: Invalid src=" << src << " or dest=" << dest << " (max=" << NSRV << ")" << endl;
        exit(1);
    }

    // Check cache for existing route with this pointer value
    // We only cache forward paths (when reverse_path is false) for simplicity
    if (!reverse_path) {
        auto cache_key = std::make_tuple(src, dest, rr_index);
        auto cache_iter = _drb_route_cache.find(cache_key);
        if (cache_iter != _drb_route_cache.end()) {
            return cache_iter->second;  // Return cached route
        }
    }

    route_t *routeout = new Route();
    route_t *routeback = nullptr;

    if (reverse_path) {
        routeback = new Route();
    }

    uint32_t src_tor = HOST_POD_SWITCH(src);
    uint32_t dest_tor = HOST_POD_SWITCH(dest);
    uint32_t src_pod = HOST_POD(src);
    uint32_t dest_pod = HOST_POD(dest);

    // Validate TOR and POD calculations
    if (src_tor >= NTOR || dest_tor >= NTOR) {
        cerr << "DRB Error: Invalid src_tor=" << src_tor << " or dest_tor=" << dest_tor << " (max=" << NTOR << ")" << endl;
        exit(1);
    }
    if (src_pod >= NPOD || dest_pod >= NPOD) {
        cerr << "DRB Error: Invalid src_pod=" << src_pod << " or dest_pod=" << dest_pod << " (max=" << NPOD << ")" << endl;
        exit(1);
    }

    // Bundle indices - for simplicity, using the first link in each bundle for now
    uint32_t b0 = 0; // Host-ToR bundle index
    uint32_t b1_up = 0; // ToR-Agg uplink bundle index
    uint32_t b1_down = 0; // Agg-ToR downlink bundle index
    uint32_t b2_up = 0; // Agg-Core uplink bundle index
    uint32_t b2_down = 0; // Core-Agg downlink bundle index

    // Scenario 1 & 2: Destination host is under the same ToR switch t as the source host (includes src==dest)
    if (src_tor == dest_tor) {
        // Unique shortest path: (source host, ToR switch t, destination host)
        routeout->push_back(queues_ns_nlp[src][src_tor][b0]);
        routeout->push_back(pipes_ns_nlp[src][src_tor][b0]);
        if (queues_ns_nlp[src][src_tor][b0]->getRemoteEndpoint())
            routeout->push_back(queues_ns_nlp[src][src_tor][b0]->getRemoteEndpoint());

        routeout->push_back(queues_nlp_ns[dest_tor][dest][b0]);
        routeout->push_back(pipes_nlp_ns[dest_tor][dest][b0]);
        if (queues_nlp_ns[dest_tor][dest][b0]->getRemoteEndpoint())
            routeout->push_back(queues_nlp_ns[dest_tor][dest][b0]->getRemoteEndpoint());

        if (reverse_path) {
            routeback->push_back(queues_ns_nlp[dest][dest_tor][b0]);
            routeback->push_back(pipes_ns_nlp[dest][dest_tor][b0]);
            if (queues_ns_nlp[dest][dest_tor][b0]->getRemoteEndpoint())
                routeback->push_back(queues_ns_nlp[dest][dest_tor][b0]->getRemoteEndpoint());

            routeback->push_back(queues_nlp_ns[src_tor][src][b0]);
            routeback->push_back(pipes_nlp_ns[src_tor][src][b0]);
            if (queues_nlp_ns[src_tor][src][b0]->getRemoteEndpoint())
                routeback->push_back(queues_nlp_ns[src_tor][src][b0]->getRemoteEndpoint());
            routeout->set_reverse(routeback);
            routeback->set_reverse(routeout);
        }
    }
    // Scenario 3: Destination host is in the same pod p as the source host, but not under the same ToR
    else if (src_pod == dest_pod) {
        // Path: (source host, ToR switch t_s, spine s in pod p, ToR switch t_d, destination host)
        
        // Retrieve valid spines from cache
        auto it = _valid_spine_switches_cache.find({src_tor, dest_tor});
        if (it == _valid_spine_switches_cache.end() || it->second.empty()) {
            cerr << "DRB Error: No valid spine path found for src_tor " << src_tor << " to dest_tor " << dest_tor << " in pod " << src_pod << endl;
            exit(1);
        }
        const std::vector<uint32_t>& valid_spines = it->second;

        uint32_t selected_agg_idx = rr_index % valid_spines.size();
        uint32_t selected_agg_switch = valid_spines[selected_agg_idx];

        // Original validation (now mostly redundant due to pre-computation and filtering, but good for sanity check)
        if (selected_agg_switch >= NAGG || selected_agg_switch < MIN_POD_AGG_SWITCH(src_pod) || selected_agg_switch > MAX_POD_AGG_SWITCH(src_pod)) {
             cerr << "DRB Error: Selected_agg_switch " << selected_agg_switch << " is out of bounds for pod " << src_pod << endl;
             exit(1);
        }

        routeout->push_back(queues_ns_nlp[src][src_tor][b0]);
        routeout->push_back(pipes_ns_nlp[src][src_tor][b0]);
        if (queues_ns_nlp[src][src_tor][b0]->getRemoteEndpoint())
            routeout->push_back(queues_ns_nlp[src][src_tor][b0]->getRemoteEndpoint());

        routeout->push_back(queues_nlp_nup[src_tor][selected_agg_switch][b1_up]);
        routeout->push_back(pipes_nlp_nup[src_tor][selected_agg_switch][b1_up]);
        if (queues_nlp_nup[src_tor][selected_agg_switch][b1_up]->getRemoteEndpoint())
            routeout->push_back(queues_nlp_nup[src_tor][selected_agg_switch][b1_up]->getRemoteEndpoint());

        routeout->push_back(queues_nup_nlp[selected_agg_switch][dest_tor][b1_down]);
        routeout->push_back(pipes_nup_nlp[selected_agg_switch][dest_tor][b1_down]);
        if (queues_nup_nlp[selected_agg_switch][dest_tor][b1_down]->getRemoteEndpoint())
            routeout->push_back(queues_nup_nlp[selected_agg_switch][dest_tor][b1_down]->getRemoteEndpoint());

        routeout->push_back(queues_nlp_ns[dest_tor][dest][b0]);
        routeout->push_back(pipes_nlp_ns[dest_tor][dest][b0]);
        if (queues_nlp_ns[dest_tor][dest][b0]->getRemoteEndpoint())
            routeout->push_back(queues_nlp_ns[dest_tor][dest][b0]->getRemoteEndpoint());

        if (reverse_path) {
            routeback->push_back(queues_ns_nlp[dest][dest_tor][b0]);
            routeback->push_back(pipes_ns_nlp[dest][dest_tor][b0]);
            if (queues_ns_nlp[dest][dest_tor][b0]->getRemoteEndpoint())
                routeback->push_back(queues_ns_nlp[dest][dest_tor][b0]->getRemoteEndpoint());

            routeback->push_back(queues_nlp_nup[dest_tor][selected_agg_switch][b1_down]); // Note: b1_down in reverse is actually b1_up in forward
            routeback->push_back(pipes_nlp_nup[dest_tor][selected_agg_switch][b1_down]);
            if (queues_nlp_nup[dest_tor][selected_agg_switch][b1_down]->getRemoteEndpoint())
                routeback->push_back(queues_nlp_nup[dest_tor][selected_agg_switch][b1_down]->getRemoteEndpoint());

            routeback->push_back(queues_nup_nlp[selected_agg_switch][src_tor][b1_up]); // Note: b1_up in reverse is actually b1_down in forward
            routeback->push_back(pipes_nup_nlp[selected_agg_switch][src_tor][b1_up]);
            if (queues_nup_nlp[selected_agg_switch][src_tor][b1_up]->getRemoteEndpoint())
                routeback->push_back(queues_nup_nlp[selected_agg_switch][src_tor][b1_up]->getRemoteEndpoint());

            routeback->push_back(queues_nlp_ns[src_tor][src][b0]);
            routeback->push_back(pipes_nlp_ns[src_tor][src][b0]);
            if (queues_nlp_ns[src_tor][src][b0]->getRemoteEndpoint())
                routeback->push_back(queues_nlp_ns[src_tor][src][b0]->getRemoteEndpoint());
            routeout->set_reverse(routeback);
            routeback->set_reverse(routeout);
        }
    }
    // Scenario 4: Destination host is in a different pod p as the source host
    else {
        assert(_tiers == 3); // This scenario only applies to 3-tier fat trees

        // Retrieve valid cores from cache
        auto it = _valid_core_switches_cache.find({src_tor, dest_tor});
        if (it == _valid_core_switches_cache.end() || it->second.empty()) {
            cerr << "DRB Error: No valid core path found for src_tor " << src_tor << " to dest_tor " << dest_tor << " (src_pod " << src_pod << ", dest_pod " << dest_pod << ")" << endl;
            exit(1);
        }
        const std::vector<uint32_t>& valid_cores = it->second;

        uint32_t selected_core_idx = rr_index % valid_cores.size();
        uint32_t selected_core = valid_cores[selected_core_idx];

        // Determine source aggregation switch that connects to selected_core
        // Logic: core_id = podpos + _agg_switches_per_pod * l
        // So, podpos for the agg switch should be selected_core % _agg_switches_per_pod
        uint32_t src_agg_switch = MIN_POD_AGG_SWITCH(src_pod) + (selected_core % _agg_switches_per_pod);
        uint32_t dest_agg_switch = MIN_POD_AGG_SWITCH(dest_pod) + (selected_core % _agg_switches_per_pod);

        // Original validation (now mostly redundant due to pre-computation and filtering, but good for sanity check)
        if (src_agg_switch >= NAGG || src_agg_switch < MIN_POD_AGG_SWITCH(src_pod) || src_agg_switch > MAX_POD_AGG_SWITCH(src_pod)) {
            cerr << "DRB Error: Invalid src_agg_switch " << src_agg_switch << " for src_pod " << src_pod << " and core " << selected_core << endl;
            exit(1);
        }
        if (dest_agg_switch >= NAGG || dest_agg_switch < MIN_POD_AGG_SWITCH(dest_pod) || dest_agg_switch > MAX_POD_AGG_SWITCH(dest_pod)) {
            cerr << "DRB Error: Invalid dest_agg_switch " << dest_agg_switch << " for dest_pod " << dest_pod << " and core " << selected_core << endl;
            exit(1);
        }
        
        routeout->push_back(queues_ns_nlp[src][src_tor][b0]);
        routeout->push_back(pipes_ns_nlp[src][src_tor][b0]);
        if (queues_ns_nlp[src][src_tor][b0]->getRemoteEndpoint())
            routeout->push_back(queues_ns_nlp[src][src_tor][b0]->getRemoteEndpoint());

        routeout->push_back(queues_nlp_nup[src_tor][src_agg_switch][b1_up]);
        routeout->push_back(pipes_nlp_nup[src_tor][src_agg_switch][b1_up]);
        if (queues_nlp_nup[src_tor][src_agg_switch][b1_up]->getRemoteEndpoint())
            routeout->push_back(queues_nlp_nup[src_tor][src_agg_switch][b1_up]->getRemoteEndpoint());

        routeout->push_back(queues_nup_nc[src_agg_switch][selected_core][b2_up]);
        routeout->push_back(pipes_nup_nc[src_agg_switch][selected_core][b2_up]);
        if (queues_nup_nc[src_agg_switch][selected_core][b2_up]->getRemoteEndpoint())
            routeout->push_back(queues_nup_nc[src_agg_switch][selected_core][b2_up]->getRemoteEndpoint());

        routeout->push_back(queues_nc_nup[selected_core][dest_agg_switch][b2_down]);
        routeout->push_back(pipes_nc_nup[selected_core][dest_agg_switch][b2_down]);
        if (queues_nc_nup[selected_core][dest_agg_switch][b2_down]->getRemoteEndpoint())
            routeout->push_back(queues_nc_nup[selected_core][dest_agg_switch][b2_down]->getRemoteEndpoint());

        routeout->push_back(queues_nup_nlp[dest_agg_switch][dest_tor][b1_down]);
        routeout->push_back(pipes_nup_nlp[dest_agg_switch][dest_tor][b1_down]);
        if (queues_nup_nlp[dest_agg_switch][dest_tor][b1_down]->getRemoteEndpoint())
            routeout->push_back(queues_nup_nlp[dest_agg_switch][dest_tor][b1_down]->getRemoteEndpoint());

        routeout->push_back(queues_nlp_ns[dest_tor][dest][b0]);
        routeout->push_back(pipes_nlp_ns[dest_tor][dest][b0]);
        if (queues_nlp_ns[dest_tor][dest][b0]->getRemoteEndpoint())
            routeout->push_back(queues_nlp_ns[dest_tor][dest][b0]->getRemoteEndpoint());

        if (reverse_path) {
            routeback->push_back(queues_ns_nlp[dest][dest_tor][b0]);
            routeback->push_back(pipes_ns_nlp[dest][dest_tor][b0]);
            if (queues_ns_nlp[dest][dest_tor][b0]->getRemoteEndpoint())
                routeback->push_back(queues_ns_nlp[dest][dest_tor][b0]->getRemoteEndpoint());

            routeback->push_back(queues_nlp_nup[dest_tor][dest_agg_switch][b1_down]); // reverse of b1_down
            routeback->push_back(pipes_nlp_nup[dest_tor][dest_agg_switch][b1_down]);
            if (queues_nlp_nup[dest_tor][dest_agg_switch][b1_down]->getRemoteEndpoint())
                routeback->push_back(queues_nlp_nup[dest_tor][dest_agg_switch][b1_down]->getRemoteEndpoint());

            routeback->push_back(queues_nup_nc[dest_agg_switch][selected_core][b2_down]); // reverse of b2_down
            routeback->push_back(pipes_nup_nc[dest_agg_switch][selected_core][b2_down]);
            if (queues_nup_nc[dest_agg_switch][selected_core][b2_down]->getRemoteEndpoint())
                routeback->push_back(queues_nup_nc[dest_agg_switch][selected_core][b2_down]->getRemoteEndpoint());

            routeback->push_back(queues_nc_nup[selected_core][src_agg_switch][b2_up]); // reverse of b2_up
            routeback->push_back(pipes_nc_nup[selected_core][src_agg_switch][b2_up]);
            if (queues_nc_nup[selected_core][src_agg_switch][b2_up]->getRemoteEndpoint())
                routeback->push_back(queues_nc_nup[selected_core][src_agg_switch][b2_up]->getRemoteEndpoint());

            routeback->push_back(queues_nup_nlp[src_agg_switch][src_tor][b1_up]); // reverse of b1_up
            routeback->push_back(pipes_nup_nlp[src_agg_switch][src_tor][b1_up]);
            if (queues_nup_nlp[src_agg_switch][src_tor][b1_up]->getRemoteEndpoint())
                routeback->push_back(queues_nup_nlp[src_agg_switch][src_tor][b1_up]->getRemoteEndpoint());

            routeback->push_back(queues_nlp_ns[src_tor][src][b0]);
            routeback->push_back(pipes_nlp_ns[src_tor][src][b0]);
            if (queues_nlp_ns[src_tor][src][b0]->getRemoteEndpoint())
                routeback->push_back(queues_nlp_ns[src_tor][src][b0]->getRemoteEndpoint());
            routeout->set_reverse(routeback);
            routeback->set_reverse(routeout);
        }
    }

    check_non_null(routeout); // This is a general check, specific shortest path alerts are embedded above

    // Cache the route for future calls with the same pointer value
    // We only cache forward paths (when reverse_path is false)
    if (!reverse_path) {
        auto cache_key = std::make_tuple(src, dest, rr_index);
        _drb_route_cache[cache_key] = routeout;
    }

    return routeout;
}

Route* FatTreeTopologyDRB::get_tor_route(uint32_t hostnum) {
    Route* torroute = new Route();
    torroute->push_back(queues_ns_nlp[hostnum][HOST_POD_SWITCH(hostnum)][0]);
    torroute->push_back(pipes_ns_nlp[hostnum][HOST_POD_SWITCH(hostnum)][0]);
    torroute->push_back(queues_ns_nlp[hostnum][HOST_POD_SWITCH(hostnum)][0]->getRemoteEndpoint());
    return torroute;
}

void FatTreeTopologyDRB::add_host_port(uint32_t hostnum, flowid_t flow_id, PacketSink* host) {
    assert(switches_lp[HOST_POD_SWITCH(hostnum)]);
    switches_lp[HOST_POD_SWITCH(hostnum)]->addHostPort(hostnum,flow_id,host);
}

vector<const Route*>* FatTreeTopologyDRB::get_bidir_paths(uint32_t src, uint32_t dest, bool reverse){
    vector<const Route*>* paths = new vector<const Route*>();

    route_t *routeout, *routeback;
  
    //QueueLoggerSimple *simplequeuelogger = new QueueLoggerSimple();
    //QueueLoggerSimple *simplequeuelogger = 0;
    //logfile->addLogger(*simplequeuelogger);
    //Queue* pqueue = new Queue(_linkspeed, memFromPkt(FEEDER_BUFFER), *_eventlist, simplequeuelogger);
    //pqueue->setName("PQueue_" + ntoa(src) + "_" + ntoa(dest));
    //logfile->writeName(*pqueue);
    if (HOST_POD_SWITCH(src)==HOST_POD_SWITCH(dest)){
  
        // forward path
        routeout = new Route();
        //routeout->push_back(pqueue);
        routeout->push_back(queues_ns_nlp[src][HOST_POD_SWITCH(src)][0]);
        routeout->push_back(pipes_ns_nlp[src][HOST_POD_SWITCH(src)][0]);

        if (_qt==LOSSLESS_INPUT || _qt==LOSSLESS_INPUT_ECN)
            routeout->push_back(queues_ns_nlp[src][HOST_POD_SWITCH(src)][0]->getRemoteEndpoint());

        routeout->push_back(queues_nlp_ns[HOST_POD_SWITCH(dest)][dest][0]);
        routeout->push_back(pipes_nlp_ns[HOST_POD_SWITCH(dest)][dest][0]);

        if (reverse) {
            // reverse path for RTS packets
            routeback = new Route();
            routeback->push_back(queues_ns_nlp[dest][HOST_POD_SWITCH(dest)][0]);
            routeback->push_back(pipes_ns_nlp[dest][HOST_POD_SWITCH(dest)][0]);

            if (_qt==LOSSLESS_INPUT || _qt==LOSSLESS_INPUT_ECN)
                routeback->push_back(queues_ns_nlp[dest][HOST_POD_SWITCH(dest)][0]->getRemoteEndpoint());

            routeback->push_back(queues_nlp_ns[HOST_POD_SWITCH(src)][src][0]);
            routeback->push_back(pipes_nlp_ns[HOST_POD_SWITCH(src)][src][0]);

            routeout->set_reverse(routeback);
            routeback->set_reverse(routeout);
        }

        //print_route(*routeout);
        paths->push_back(routeout);

        check_non_null(routeout);
        //cout << "pathcount " << paths->size() << endl;
        return paths;
    }
    else if (HOST_POD(src)==HOST_POD(dest)){
        //don't go up the hierarchy, stay in the pod only.

        uint32_t pod = HOST_POD(src);
        //there are K/2 paths between the source and the destination  <- this is no longer true for bundles
        if (_tiers == 2) {
            // xxx sanity check for debugging, remove later.
            assert(MIN_POD_AGG_SWITCH(pod) == 0);
            assert(MAX_POD_AGG_SWITCH(pod) == NAGG - 1);
        }
        for (uint32_t upper = MIN_POD_AGG_SWITCH(pod);upper <= MAX_POD_AGG_SWITCH(pod); upper++){
            for (uint32_t b_up = 0; b_up < _bundlesize[AGG_TIER]; b_up++) {
                for (uint32_t b_down = 0; b_down < _bundlesize[AGG_TIER]; b_down++) {
                    // b_up is link number in upgoing bundle, b_down is link number in downgoing bundle
                    // note: no bundling supported between host and tor - just use link number 0
                
                    //upper is nup
      
                    routeout = new Route();
      
                    routeout->push_back(queues_ns_nlp[src][HOST_POD_SWITCH(src)][0]);
                    routeout->push_back(pipes_ns_nlp[src][HOST_POD_SWITCH(src)][0]);

                    if (_qt==LOSSLESS_INPUT || _qt==LOSSLESS_INPUT_ECN)
                        routeout->push_back(queues_ns_nlp[src][HOST_POD_SWITCH(src)][0]->getRemoteEndpoint());

                    routeout->push_back(queues_nlp_nup[HOST_POD_SWITCH(src)][upper][b_up]);
                    routeout->push_back(pipes_nlp_nup[HOST_POD_SWITCH(src)][upper][b_up]);

                    if (_qt==LOSSLESS_INPUT || _qt==LOSSLESS_INPUT_ECN)
                        routeout->push_back(queues_nlp_nup[HOST_POD_SWITCH(src)][upper][b_up]->getRemoteEndpoint());

                    routeout->push_back(queues_nup_nlp[upper][HOST_POD_SWITCH(dest)][b_down]);
                    routeout->push_back(pipes_nup_nlp[upper][HOST_POD_SWITCH(dest)][b_down]);

                    if (_qt==LOSSLESS_INPUT || _qt==LOSSLESS_INPUT_ECN)
                        routeout->push_back(queues_nup_nlp[upper][HOST_POD_SWITCH(dest)][b_down]->getRemoteEndpoint());

                    routeout->push_back(queues_nlp_ns[HOST_POD_SWITCH(dest)][dest][0]);
                    routeout->push_back(pipes_nlp_ns[HOST_POD_SWITCH(dest)][dest][0]);

                    if (reverse) {
                        // reverse path for RTS packets
                        routeback = new Route();
      
                        routeback->push_back(queues_ns_nlp[dest][HOST_POD_SWITCH(dest)][0]);
                        routeback->push_back(pipes_ns_nlp[dest][HOST_POD_SWITCH(dest)][0]);

                        if (_qt==LOSSLESS_INPUT || _qt==LOSSLESS_INPUT_ECN)
                            routeback->push_back(queues_ns_nlp[dest][HOST_POD_SWITCH(dest)][0]->getRemoteEndpoint());

                        routeback->push_back(queues_nlp_nup[HOST_POD_SWITCH(dest)][upper][b_down]);
                        routeback->push_back(pipes_nlp_nup[HOST_POD_SWITCH(dest)][upper][b_down]);

                        if (_qt==LOSSLESS_INPUT || _qt==LOSSLESS_INPUT_ECN)
                            routeback->push_back(queues_nlp_nup[HOST_POD_SWITCH(dest)][upper][b_down]->getRemoteEndpoint());

                        routeback->push_back(queues_nup_nlp[upper][HOST_POD_SWITCH(src)][b_up]);
                        routeback->push_back(pipes_nup_nlp[upper][HOST_POD_SWITCH(src)][b_up]);

                        if (_qt==LOSSLESS_INPUT || _qt==LOSSLESS_INPUT_ECN)
                            routeback->push_back(queues_nup_nlp[upper][HOST_POD_SWITCH(src)][b_up]->getRemoteEndpoint());
      
                        routeback->push_back(queues_nlp_ns[HOST_POD_SWITCH(src)][src][0]);
                        routeback->push_back(pipes_nlp_ns[HOST_POD_SWITCH(src)][src][0]);

                        routeout->set_reverse(routeback);
                        routeback->set_reverse(routeout);
                    }
      
                    //print_route(*routeout);
                    paths->push_back(routeout);
                    check_non_null(routeout);
                }
            }
        }
        // cout << "pathcount " << paths->size() << endl;
        return paths;
    } else {
        assert(_tiers == 3);
        uint32_t pod = HOST_POD(src);

        for (uint32_t upper = MIN_POD_AGG_SWITCH(pod); upper <= MAX_POD_AGG_SWITCH(pod); upper++) {
            uint32_t podpos = upper % _agg_switches_per_pod;

            for (uint32_t l = 0; l < _radix_up[AGG_TIER]/_bundlesize[CORE_TIER]; l++) {
                uint32_t core = podpos +  _agg_switches_per_pod * l;

                for (uint32_t b1_up = 0; b1_up < _bundlesize[AGG_TIER]; b1_up++) {
                    for (uint32_t b1_down = 0; b1_down < _bundlesize[AGG_TIER]; b1_down++) {
                        // b1_up is link number in upgoing bundle from tor to agg, b1_down is link number in downgoing bundle

                        for (uint32_t b2_up = 0; b2_up < _bundlesize[CORE_TIER]; b2_up++) {
                            for (uint32_t b2_down = 0; b2_down < _bundlesize[CORE_TIER]; b2_down++) {
                                // b2_up is link number in upgoing bundle from agg to core, b2_down is link number in downgoing bundle
                                // note: no bundling supported between host and tor - just use link number 0
                                //upper is nup
        
                                routeout = new Route();
                                //routeout->push_back(pqueue);
        
                                routeout->push_back(queues_ns_nlp[src][HOST_POD_SWITCH(src)][0]);
                                routeout->push_back(pipes_ns_nlp[src][HOST_POD_SWITCH(src)][0]);

                                if (_qt==LOSSLESS_INPUT || _qt==LOSSLESS_INPUT_ECN)
                                    routeout->push_back(queues_ns_nlp[src][HOST_POD_SWITCH(src)][0]->getRemoteEndpoint());
        
                                routeout->push_back(queues_nlp_nup[HOST_POD_SWITCH(src)][upper][b1_up]);
                                routeout->push_back(pipes_nlp_nup[HOST_POD_SWITCH(src)][upper][b1_up]);

                                if (_qt==LOSSLESS_INPUT || _qt==LOSSLESS_INPUT_ECN)
                                    routeout->push_back(queues_nlp_nup[HOST_POD_SWITCH(src)][upper][b1_up]->getRemoteEndpoint());
        
                                routeout->push_back(queues_nup_nc[upper][core][b2_up]);
                                routeout->push_back(pipes_nup_nc[upper][core][b2_up]);

                                if (_qt==LOSSLESS_INPUT || _qt==LOSSLESS_INPUT_ECN)
                                    routeout->push_back(queues_nup_nc[upper][core][b2_up]->getRemoteEndpoint());
        
                                //now take the only link down to the destination server!
        
                                uint32_t upper2 = MIN_POD_AGG_SWITCH(HOST_POD(dest)) + core % _agg_switches_per_pod;
                                //printf("K %d HOST_POD(%d) %d core %d upper2 %d\n",K,dest,HOST_POD(dest),core, upper2);
        
                                routeout->push_back(queues_nc_nup[core][upper2][b2_down]);
                                routeout->push_back(pipes_nc_nup[core][upper2][b2_down]);

                                if (_qt==LOSSLESS_INPUT || _qt==LOSSLESS_INPUT_ECN)
                                    routeout->push_back(queues_nc_nup[core][upper2][b2_down]->getRemoteEndpoint());        

                                routeout->push_back(queues_nup_nlp[upper2][HOST_POD_SWITCH(dest)][b1_down]);
                                routeout->push_back(pipes_nup_nlp[upper2][HOST_POD_SWITCH(dest)][b1_down]);

                                if (_qt==LOSSLESS_INPUT || _qt==LOSSLESS_INPUT_ECN)
                                    routeout->push_back(queues_nup_nlp[upper2][HOST_POD_SWITCH(dest)][b1_down]->getRemoteEndpoint());
        
                                routeout->push_back(queues_nlp_ns[HOST_POD_SWITCH(dest)][dest][0]);
                                routeout->push_back(pipes_nlp_ns[HOST_POD_SWITCH(dest)][dest][0]);

                                if (reverse) {
                                    // reverse path for RTS packets
                                    routeback = new Route();
        
                                    routeback->push_back(queues_ns_nlp[dest][HOST_POD_SWITCH(dest)][0]);
                                    routeback->push_back(pipes_ns_nlp[dest][HOST_POD_SWITCH(dest)][0]);

                                    if (_qt==LOSSLESS_INPUT || _qt==LOSSLESS_INPUT_ECN)
                                        routeback->push_back(queues_ns_nlp[dest][HOST_POD_SWITCH(dest)][0]->getRemoteEndpoint());
        
                                    routeback->push_back(queues_nlp_nup[HOST_POD_SWITCH(dest)][upper2][b1_down]);
                                    routeback->push_back(pipes_nlp_nup[HOST_POD_SWITCH(dest)][upper2][b1_down]);

                                    if (_qt==LOSSLESS_INPUT || _qt==LOSSLESS_INPUT_ECN)
                                        routeback->push_back(queues_nlp_nup[HOST_POD_SWITCH(dest)][upper2][b1_down]->getRemoteEndpoint());
        
                                    routeback->push_back(queues_nup_nc[upper2][core][b2_down]);
                                    routeback->push_back(pipes_nup_nc[upper2][core][b2_down]);

                                    if (_qt==LOSSLESS_INPUT || _qt==LOSSLESS_INPUT_ECN)
                                        routeback->push_back(queues_nup_nc[upper2][core][b2_down]->getRemoteEndpoint());
        
                                    //now take the only link back down to the src server!
        
                                    routeback->push_back(queues_nc_nup[core][upper][b2_up]);
                                    routeback->push_back(pipes_nc_nup[core][upper][b2_up]);

                                    if (_qt==LOSSLESS_INPUT || _qt==LOSSLESS_INPUT_ECN)
                                        routeback->push_back(queues_nc_nup[core][upper][b2_up]->getRemoteEndpoint());
        
                                    routeback->push_back(queues_nup_nlp[upper][HOST_POD_SWITCH(src)][b1_up]);
                                    routeback->push_back(pipes_nup_nlp[upper][HOST_POD_SWITCH(src)][b1_up]);

                                    if (_qt==LOSSLESS_INPUT || _qt==LOSSLESS_INPUT_ECN)
                                        routeback->push_back(queues_nup_nlp[upper][HOST_POD_SWITCH(src)][b1_up]->getRemoteEndpoint());
        
                                    routeback->push_back(queues_nlp_ns[HOST_POD_SWITCH(src)][src][0]);
                                    routeback->push_back(pipes_nlp_ns[HOST_POD_SWITCH(src)][src][0]);


                                    routeout->set_reverse(routeback);
                                    routeback->set_reverse(routeout);
                                }
        
                                //print_route(*routeout);
                                paths->push_back(routeout);
                                check_non_null(routeout);
                            }
                        }
                    }
                }
            }
        }
        // cout << "pathcount " << paths->size() << endl;
        return paths;
    }
}

void FatTreeTopologyDRB::count_queue(Queue* queue){
    if (_link_usage.find(queue)==_link_usage.end()){
        _link_usage[queue] = 0;
    }

    _link_usage[queue] = _link_usage[queue] + 1;
}

int64_t FatTreeTopologyDRB::find_lp_switch(Queue* queue){
    //first check ns_nlp
    for (uint32_t srv=0;srv<NSRV;srv++)
        for (uint32_t tor = 0; tor < NTOR; tor++)
            if (queues_ns_nlp[srv][tor][0] == queue)
                return tor;

    //only count nup to nlp
    count_queue(queue);

    for (uint32_t agg = 0; agg < NAGG; agg++)
        for (uint32_t tor = 0; tor < NTOR; tor++)
            for (uint32_t b = 0; b < _bundlesize[AGG_TIER]; b++) {
                if (queues_nup_nlp[agg][tor][b] == queue)
                    return tor;
            }

    return -1;
}

int64_t FatTreeTopologyDRB::find_up_switch(Queue* queue){
    count_queue(queue);
    //first check nc_nup
    for (uint32_t core=0; core < NCORE; core++)
        for (uint32_t agg = 0; agg < NAGG; agg++)
            for (uint32_t b = 0; b < _bundlesize[CORE_TIER]; b++) {
                if (queues_nc_nup[core][agg][b] == queue)
                    return agg;
            }

    //check nlp_nup
    for (uint32_t tor=0; tor < NTOR; tor++)
        for (uint32_t agg = 0; agg < NAGG; agg++)
            for (uint32_t b = 0; b < _bundlesize[AGG_TIER]; b++) {
                if (queues_nlp_nup[tor][agg][b] == queue)
                    return agg;
            }

    return -1;
}

int64_t FatTreeTopologyDRB::find_core_switch(Queue* queue){
    count_queue(queue);
    //first check nup_nc
    for (uint32_t agg=0;agg<NAGG;agg++)
        for (uint32_t core = 0;core<NCORE;core++)
            for (uint32_t b = 0; b < _bundlesize[CORE_TIER]; b++) {
                if (queues_nup_nc[agg][core][b] == queue)
                    return core;
            }

    return -1;
}

int64_t FatTreeTopologyDRB::find_destination(Queue* queue){
    //first check nlp_ns
    for (uint32_t tor=0; tor<NTOR; tor++)
        for (uint32_t srv = 0; srv<NSRV; srv++)
            if (queues_nlp_ns[tor][srv][0]==queue)
                return srv;

    return -1;
}

void FatTreeTopologyDRB::print_path(std::ofstream &paths,uint32_t src,const Route* route){
    paths << "SRC_" << src << " ";
  
    if (route->size()/2==2){
        paths << "LS_" << find_lp_switch((Queue*)route->at(1)) << " ";
        paths << "DST_" << find_destination((Queue*)route->at(3)) << " ";
    } else if (route->size()/2==4){
        paths << "LS_" << find_lp_switch((Queue*)route->at(1)) << " ";
        paths << "US_" << find_up_switch((Queue*)route->at(3)) << " ";
        paths << "LS_" << find_lp_switch((Queue*)route->at(5)) << " ";
        paths << "DST_" << find_destination((Queue*)route->at(7)) << " ";
    } else if (route->size()/2==6){
        paths << "LS_" << find_lp_switch((Queue*)route->at(1)) << " ";
        paths << "US_" << find_up_switch((Queue*)route->at(3)) << " ";
        paths << "CS_" << find_core_switch((Queue*)route->at(5)) << " ";
        paths << "US_" << find_up_switch((Queue*)route->at(7)) << " ";
        paths << "LS_" << find_lp_switch((Queue*)route->at(9)) << " ";
        paths << "DST_" << find_destination((Queue*)route->at(11)) << " ";
    } else {
        paths << "Wrong hop count " << ntoa(route->size()/2);
    }
  
    paths << endl;
}

void FatTreeTopologyDRB::add_switch_loggers(Logfile& log, simtime_picosec sample_period) {
    for (uint32_t i = 0; i < NTOR; i++) {
        switches_lp[i]->add_logger(log, sample_period);
    }
    for (uint32_t i = 0; i < NAGG; i++) {
        switches_up[i]->add_logger(log, sample_period);
    }
    for (uint32_t i = 0; i < NCORE
             ; i++) {
        switches_c[i]->add_logger(log, sample_period);
    }
}

bool FatTreeTopologyDRB::_is_path_reachable(const std::vector<PacketSink*>& path_elements) {
    for (PacketSink* e : path_elements) {
        // If it's a queue, check if it's failed
        if (BaseQueue* q = dynamic_cast<BaseQueue*>(e)) {
            if (q->getFailed()) { // Corrected from isFailed() to getFailed()
                return false;
            }
        }
        // PacketSink (e.g., a Switch) might also be a BaseQueue.
        // The previous check covers this if it is.
        // If it's a generic PacketSink (like a Switch object itself),
        // we're assuming its "failure" would be reflected in its connected queues.
        // So, only checking BaseQueue for now is sufficient for link failures.
    }
    return true;
}

bool FatTreeTopologyDRB::check_non_empty() {
    bool found_non_empty = false;
    
    auto check_queue = [&](BaseQueue* q, const string& name) {
        // Only alert if queue contains at least one full data packet (filters out stray ACKs)
        if (q && q->queuesize() >= (mem_b)Packet::data_packet_size()) {
            cout << "ALERT: Queue " << name << " (ID " << q->get_id() << ") is non-empty: " << q->queuesize() << " bytes" << endl;
            found_non_empty = true;
        }
    };

    // Check ToR -> Server
    for (uint32_t j=0; j<NTOR; j++) {
        for (uint32_t k=0; k<NSRV; k++) {
            for (uint32_t b=0; b<_bundlesize[TOR_TIER]; b++) {
                check_queue(queues_nlp_ns[j][k][b], "ToR" + ntoa(j) + "->Srv" + ntoa(k));
                check_queue(queues_ns_nlp[k][j][b], "Srv" + ntoa(k) + "->ToR" + ntoa(j));
            }
        }
    }

    // Check Agg -> ToR
    for (uint32_t j=0; j<NAGG; j++) {
        for (uint32_t k=0; k<NTOR; k++) {
            for (uint32_t b=0; b<_bundlesize[AGG_TIER]; b++) {
                check_queue(queues_nup_nlp[j][k][b], "Agg" + ntoa(j) + "->ToR" + ntoa(k));
                check_queue(queues_nlp_nup[k][j][b], "ToR" + ntoa(k) + "->Agg" + ntoa(j));
            }
        }
    }

    // Check Core -> Agg
    if (_tiers == 3) {
        for (uint32_t j=0; j<NCORE; j++) {
            for (uint32_t k=0; k<NAGG; k++) {
                for (uint32_t b=0; b<_bundlesize[CORE_TIER]; b++) {
                    check_queue(queues_nc_nup[j][k][b], "Core" + ntoa(j) + "->Agg" + ntoa(k));
                    check_queue(queues_nup_nc[k][j][b], "Agg" + ntoa(k) + "->Core" + ntoa(j));
                }
            }
        }
    }

    // Check Queues inside Switches (if any specific buffering exists there, usually it's the output queues we just checked)
    // But we should also check if any packets are stuck in the switches' internal logic if applicable.
    // For FatTreeSwitchDRB, packets are usually forwarded immediately to output queues.
    
    // Note: We are not checking Pipes (packets in flight) because Pipe interface is not exposed here easily.
    // Assuming if simulation ends gracefully, pipes should be empty or negligible compared to stuck queues.

    if (found_non_empty) {
        cout << "ALERT: System contains packets at simulation end." << endl;
    } else {
        cout << "System is empty." << endl;
    }
    return found_non_empty;
}
