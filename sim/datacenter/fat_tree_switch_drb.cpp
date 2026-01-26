// -*- c-basic-offset: 4; indent-tabs-mode: nil -*-
#include "fat_tree_switch_drb.h"
#include "../routetable.h"
#include "fat_tree_topology_drb.h"
#include "../callback_pipe.h"
#include "../queue_lossless.h"
#include "../queue_lossless_output.h"
#include <algorithm>

unordered_map<BaseQueue*,uint32_t> FatTreeSwitchDRB::_port_flow_counts;

// Global flag to enable BGP-like behavior (checking downstream connectivity)
bool g_enable_bgp = false;
void set_enable_bgp(bool b) {
    g_enable_bgp = b;
}

FatTreeSwitchDRB::FatTreeSwitchDRB(EventList& eventlist, string s, switch_type t, uint32_t id,simtime_picosec delay, FatTreeTopologyDRB* ft): Switch(eventlist, s) {
    _id = id;
    _type = t;
    _pipe = new CallbackPipe(delay,eventlist, this);
    _uproutes = NULL;
    _ft = ft;
    _crt_route = 0;
    _hash_salt = random();
    _last_choice = eventlist.now();
    _fib = new RouteTable(); 
    // _fib->setWeighted(ft->weighted());
    // if (ft->weighted()) {
    //     _fib->initialize_weighted_table();
    // }
    _switch_weights = map<string,int> {};
        
    // LLSS Stuff
    _llss_packet_counts.clear();
    _rng.seed(_hash_salt); // Use the salt derived from global seed

    // Initialize LLSS vectors
    uint32_t num_pods = _ft->no_of_pods();
    uint32_t num_tors = num_pods * _ft->tor_switches_per_pod();
    _llss_schedules.resize(num_pods);
    if (_type == TOR) {
        _llss_pointers.resize(num_tors * 2, UINT32_MAX);
        _llss_pointer_wraparounds.resize(num_tors * 2, UINT32_MAX);
    } else {
        _llss_pointers.resize(num_pods * 2, UINT32_MAX);
        _llss_pointer_wraparounds.resize(num_pods * 2, UINT32_MAX);
    }
    _cached_weights.resize(num_pods);
    // Validity cache depends on destination ToR for both types
    _path_validity_cache.resize(num_tors);
}

void FatTreeSwitchDRB::receivePacket(Packet& pkt){
    if (pkt.type()==ETH_PAUSE){
        EthPausePacket* p = (EthPausePacket*)&pkt;
        //I must be in lossless mode!
        //find the egress queue that should process this, and pass it over for processing. 
        for (size_t i = 0;i < _ports.size();i++){
            LosslessQueue* q = (LosslessQueue*)_ports.at(i);
            if (q->getRemoteEndpoint() && ((Switch*)q->getRemoteEndpoint())->getID() == p->senderID()){
                q->receivePacket(pkt);
                break;
            }
        }
        
        return;
    }
    if (_packets.find(&pkt)==_packets.end()){
        //ingress pipeline processing.


        _packets[&pkt] = true;

        // Check if the packet has a full source route that we should follow (e.g. from DRB).
        // We identify this by the route length. A hop-by-hop route has size 3.
        if (pkt.route() && pkt.route()->size() > 4 && pkt.nexthop() < pkt.route()->size() && _strategy != ADAPTIVE_ROUTING) {
            if (pkt.nexthop() == NULL) {
                cout << "Dropping DRB packet with src:dst" << pkt.src() << "->" << pkt.dst() << endl;
                _packets.erase(&pkt);
                pkt.free(); // this is a black hole; drop the packet
                return;
            }
            _pipe->receivePacket(pkt);
            return;
        }

        const Route * nh = getNextHop(pkt,NULL);
        if (nh == NULL) {
            _packets.erase(&pkt);
            pkt.free(); // this is a black hole; drop the packet
            return;
        }
        //set next hop which is peer switch.
        pkt.set_route(*nh);

        //emulate the switching latency between ingress and packet arriving at the egress queue.
        _pipe->receivePacket(pkt); 
    }
    else {
        _packets.erase(&pkt);

        //egress queue processing.
        //cout << "Switch type " << _type <<  " id " << _id << " pkt dst " << pkt.dst() << " dir " << pkt.get_direction() << endl;
        pkt.sendOn();
    }
};

void FatTreeSwitchDRB::addHostPort(int addr, int flowid, PacketSink* transport){
    Route* rt = new Route();
    rt->push_back(_ft->queues_nlp_ns[_ft->HOST_POD_SWITCH(addr)][addr][0]);
    rt->push_back(_ft->pipes_nlp_ns[_ft->HOST_POD_SWITCH(addr)][addr][0]);
    rt->push_back(transport);
    _fib->addHostRoute(addr,rt,flowid);
}

uint32_t mhash(uint32_t x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

uint32_t FatTreeSwitchDRB::adaptive_route_p2c(vector<FibEntry*>* ecmp_set, int8_t (*cmp)(FibEntry*,FibEntry*)){
    uint32_t choice = 0, min = UINT32_MAX;
    uint32_t start, i = 0;
    static const uint16_t nr_choices = 2;
    
    do {
        start = random()%ecmp_set->size();

        Route * r= (*ecmp_set)[start]->getEgressPort();
        assert(r && r->size()>1);
        BaseQueue* q = (BaseQueue*)(r->at(0));
        assert(q);
        if (q->queuesize()<min){
            choice = start;
            min = q->queuesize();
        }
        i++;
    } while (i<nr_choices);
    return choice;
}

uint32_t FatTreeSwitchDRB::adaptive_route(vector<FibEntry*>* ecmp_set, int8_t (*cmp)(FibEntry*,FibEntry*)){
    //cout << "adaptive_route" << endl;
    uint32_t choice = 0;

    uint32_t best_choices[256];
    uint32_t best_choices_count = 0;
  
    FibEntry* min = (*ecmp_set)[choice];
    best_choices[best_choices_count++] = choice;

    for (uint32_t i = 1; i< ecmp_set->size(); i++){
        int8_t c = cmp(min,(*ecmp_set)[i]);

        if (c < 0){
            choice = i;
            min = (*ecmp_set)[choice];
            best_choices_count = 0;
            best_choices[best_choices_count++] = choice;
        }
        else if (c==0){
            assert(best_choices_count<255);
            best_choices[best_choices_count++] = i;
        }        
    }

    assert (best_choices_count>=1);
    uint32_t choiceindex = random()%best_choices_count;
    choice = best_choices[choiceindex];
    //cout << "ECMP set choices " << ecmp_set->size() << " Choice count " << best_choices_count << " chosen entry " << choiceindex << " chosen path " << choice << " ";

    if (cmp==compare_flow_count){
        //for (uint32_t i = 0; i<best_choices_count;i++)
          //  cout << "pathcnt " << best_choices[i] << "="<< _port_flow_counts[(BaseQueue*)( (*ecmp_set)[best_choices[i]]->getEgressPort()->at(0))]<< " ";
        
        _port_flow_counts[(BaseQueue*)((*ecmp_set)[choice]->getEgressPort()->at(0))]++;
    }

    return choice;
}

uint32_t FatTreeSwitchDRB::replace_worst_choice(vector<FibEntry*>* ecmp_set, int8_t (*cmp)(FibEntry*,FibEntry*),uint32_t my_choice){
    uint32_t best_choice = 0;
    uint32_t worst_choice = 0;

    uint32_t best_choices[256];
    uint32_t best_choices_count = 0;

    FibEntry* min = (*ecmp_set)[best_choice];
    FibEntry* max = (*ecmp_set)[worst_choice];
    best_choices[best_choices_count++] = best_choice;

    for (uint32_t i = 1; i< ecmp_set->size(); i++){
        int8_t c = cmp(min,(*ecmp_set)[i]);

        if (c < 0){
            best_choice = i;
            min = (*ecmp_set)[best_choice];
            best_choices_count = 0;
            best_choices[best_choices_count++] = best_choice;
        }
        else if (c==0){
            assert(best_choices_count<256);
            best_choices[best_choices_count++] = i;
        }        

        if (cmp(max,(*ecmp_set)[i])>0){
            worst_choice = i;
            max = (*ecmp_set)[worst_choice];
        }
    }

    //might need to play with different alternatives here, compare to worst rather than just to worst index.
    int8_t r = cmp((*ecmp_set)[my_choice],(*ecmp_set)[worst_choice]);
    assert(r>=0);

    if (r==0){
        assert (best_choices_count>=1);
        return best_choices[random()%best_choices_count];
    }
    else return my_choice;
}


int8_t FatTreeSwitchDRB::compare_pause(FibEntry* left, FibEntry* right){
    Route * r1= left->getEgressPort();
    assert(r1 && r1->size()>1);
    LosslessOutputQueue* q1 = dynamic_cast<LosslessOutputQueue*>(r1->at(0));
    Route * r2= right->getEgressPort();
    assert(r2 && r2->size()>1);
    LosslessOutputQueue* q2 = dynamic_cast<LosslessOutputQueue*>(r2->at(0));

    if (!q1->is_paused()&&q2->is_paused())
        return 1;
    else if (q1->is_paused()&&!q2->is_paused())
        return -1;
    else 
        return 0;
}

int8_t FatTreeSwitchDRB::compare_flow_count(FibEntry* left, FibEntry* right){
    Route * r1= left->getEgressPort();
    assert(r1 && r1->size()>1);
    BaseQueue* q1 = (BaseQueue*)(r1->at(0));
    Route * r2= right->getEgressPort();
    assert(r2 && r2->size()>1);
    BaseQueue* q2 = (BaseQueue*)(r2->at(0));

    if (_port_flow_counts.find(q1)==_port_flow_counts.end())
        _port_flow_counts[q1] = 0;

    if (_port_flow_counts.find(q2)==_port_flow_counts.end())
        _port_flow_counts[q2] = 0;

    //cout << "CMP q1 " << q1 << "=" << _port_flow_counts[q1] << " q2 " << q2 << "=" << _port_flow_counts[q2] << endl; 

    if (_port_flow_counts[q1] < _port_flow_counts[q2])
        return 1;
    else if (_port_flow_counts[q1] > _port_flow_counts[q2] )
        return -1;
    else 
        return 0;
}

int8_t FatTreeSwitchDRB::compare_queuesize(FibEntry* left, FibEntry* right){
    Route * r1= left->getEgressPort();
    assert(r1 && r1->size()>1);
    BaseQueue* q1 = dynamic_cast<BaseQueue*>(r1->at(0));
    Route * r2= right->getEgressPort();
    assert(r2 && r2->size()>1);
    BaseQueue* q2 = dynamic_cast<BaseQueue*>(r2->at(0));

    if (q1->quantized_queuesize() < q2->quantized_queuesize())
        return 1;
    else if (q1->quantized_queuesize() > q2->quantized_queuesize())
        return -1;
    else 
        return 0;
}

int8_t FatTreeSwitchDRB::compare_bandwidth(FibEntry* left, FibEntry* right){
    Route * r1= left->getEgressPort();
    assert(r1 && r1->size()>1);
    BaseQueue* q1 = dynamic_cast<BaseQueue*>(r1->at(0));
    Route * r2= right->getEgressPort();
    assert(r2 && r2->size()>1);
    BaseQueue* q2 = dynamic_cast<BaseQueue*>(r2->at(0));

    if (q1->quantized_utilization() < q2->quantized_utilization())
        return 1;
    else if (q1->quantized_utilization() > q2->quantized_utilization())
        return -1;
    else 
        return 0;

    /*if (q1->average_utilization() < q2->average_utilization())
        return 1;
    else if (q1->average_utilization() > q2->average_utilization())
        return -1;
    else 
        return 0;        */
}

int8_t FatTreeSwitchDRB::compare_pqb(FibEntry* left, FibEntry* right){
    //compare pause, queuesize, bandwidth.
    int8_t p = compare_pause(left, right);

    if (p!=0)
        return p;
    
    p = compare_queuesize(left,right);

    if (p!=0)
        return p;

    return compare_bandwidth(left,right);
}

int8_t FatTreeSwitchDRB::compare_pq(FibEntry* left, FibEntry* right){
    //compare pause, queuesize, bandwidth.
    int8_t p = compare_pause(left, right);

    if (p!=0)
        return p;
    
    return compare_queuesize(left,right);
}

int8_t FatTreeSwitchDRB::compare_qb(FibEntry* left, FibEntry* right){
    //compare pause, queuesize, bandwidth.
    int8_t p = compare_queuesize(left, right);

    if (p!=0)
        return p;
    
    return compare_bandwidth(left,right);
}

int8_t FatTreeSwitchDRB::compare_pb(FibEntry* left, FibEntry* right){
    //compare pause, queuesize, bandwidth.
    int8_t p = compare_pause(left, right);

    if (p!=0)
        return p;
    
    return compare_bandwidth(left,right);
}

void FatTreeSwitchDRB::permute_paths(vector<FibEntry *>* uproutes) {
    int len = uproutes->size();
    for (int i = 0; i < len; i++) {
        int ix = random() % (len - i);
        FibEntry* tmppath = (*uproutes)[ix];
        (*uproutes)[ix] = (*uproutes)[len-1-i];
        (*uproutes)[len-1-i] = tmppath;
    }
}

map<uint32_t, BaseQueue*> FatTreeSwitchDRB::getUpPorts() {
    map<uint32_t, BaseQueue*> up_ports = {};
    if (_type == TOR) {
        uint32_t podid,agg_min,agg_max;

        if (_ft->get_tiers()==3) {
            podid = _id / _ft->tor_switches_per_pod();
            agg_min = _ft->MIN_POD_AGG_SWITCH(podid);
            agg_max = _ft->MAX_POD_AGG_SWITCH(podid);
        }
        else {
            agg_min = 0;
            agg_max = _ft->getNAGG()-1;
        }

        for (uint32_t k=agg_min; k<=agg_max;k++){
            for (uint32_t b = 0; b < _ft->bundlesize(AGG_TIER); b++) {
                up_ports[k] = _ft->queues_nlp_nup[_id][k][b];
                
            }
        }
    } else if (_type == AGG) {
        uint32_t podpos = _id % _ft->agg_switches_per_pod();
        uint32_t uplink_bundles = _ft->radix_up(AGG_TIER) / _ft->bundlesize(CORE_TIER);
        for (uint32_t l = 0; l <  uplink_bundles ; l++) {
            uint32_t core = l * _ft->agg_switches_per_pod() + podpos;

            for (uint32_t b = 0; b < _ft->bundlesize(CORE_TIER); b++) {
                up_ports[core] =_ft->queues_nup_nc[_id][core][b];
            }
        }
    }
    return up_ports;
}

int FatTreeSwitchDRB::countActiveUpPorts() {
    map<uint32_t, BaseQueue*> up_ports = getUpPorts();
    int count = 0;
    for (map<uint32_t, BaseQueue*>::iterator it = up_ports.begin(); it != up_ports.end(); ++it) {
        if (it->second == NULL || it->second->getFailed()) {
            continue;
        }
        count++;
    } 
    return count;
}

int FatTreeSwitchDRB::countActiveRoutesToPod(int pod) {
    assert(_type == AGG);
    int count = 0;
    uint32_t podpos = _id % _ft->agg_switches_per_pod();
    uint32_t uplink_bundles = _ft->radix_up(AGG_TIER) / _ft->bundlesize(CORE_TIER);
    for (uint32_t l = 0; l <  uplink_bundles ; l++) {
        uint32_t core = l * _ft->agg_switches_per_pod() + podpos;
        uint32_t dst_agg =  _ft->MIN_POD_AGG_SWITCH(pod)   + podpos;
        for (uint32_t b = 0; b < _ft->bundlesize(CORE_TIER); b++) {
            // Check if the link between the core switch and the destination AGG switch is up
            if (_ft->queues_nc_nup[core][dst_agg][b] == NULL || _ft->queues_nc_nup[core][dst_agg][b]->getFailed()) {
                continue;
            }
            count++;
        }
    }
    return count;
}

// vector<FibEntry*>* FatTreeSwitchDRB::getUproutes() {
//     if (_uproutes != NULL) {
//         return _uproutes;
//     }
//     uint32_t example_dst = (_id + 1) * _ft->radix_down(TOR_TIER);

//     addRoute(example_dst);
//     assert(_uproutes);
//     return _uproutes;
// }

void FatTreeSwitchDRB::forcePopulateRoutes() {
    uint32_t hosts = _ft->no_of_servers();
    for (uint32_t i=0; i < hosts; i++) {
        addRoute(i);
    }
}

void FatTreeSwitchDRB::forcePopulateLlssSchedules() {
    uint32_t pods = _ft->no_of_pods();
    for (uint32_t i=0; i < pods; i++) {
        _llss_schedules[i] = _generate_weighted_llss_schedule(i);
    }
}

void FatTreeSwitchDRB::clearLlssState() {
    uint32_t num_pods = _ft->no_of_pods();
    uint32_t num_tors = num_pods * _ft->tor_switches_per_pod();
    _llss_schedules.clear();
    _llss_schedules.resize(num_pods);
    _path_validity_cache.clear();
    _path_validity_cache.resize(num_tors);
    _llss_pointers.clear();
    _llss_pointer_wraparounds.clear();
    if (_type == TOR) {
        _llss_pointers.resize(num_tors * 2, UINT32_MAX);
        _llss_pointer_wraparounds.resize(num_tors * 2, UINT32_MAX);
    } else {
        _llss_pointers.resize(num_pods * 2, UINT32_MAX);
         _llss_pointer_wraparounds.resize(num_pods * 2, UINT32_MAX);
    }
    // _cached_weights = ; // Unused it seems??

}

void FatTreeSwitchDRB::applyWeights() {
    // For all existing routes, add according to weight. For future ones, the _switch_weights and _weighted should indicate that you have to add to the weighted fib too
    _fib->setWeighted(true);
    vector<int> dsts = _fib->getDestinations();
    for (int dst: dsts) {
        vector<FibEntry*>* routes = _fib->getRoutes(dst);
        for (FibEntry* route : *routes) {
            string neighborname = route->getEgressPort()->at(0)->getRemoteEndpoint()->nodename();
            int weight = _switch_weights.find(neighborname) == _switch_weights.end() ? 1 : _switch_weights[neighborname];
            _fib->addWeightedRoute(dst, route->getEgressPort(), route->getCost(), route->getDirection(), weight);
        }
    }
}

bool FatTreeSwitchDRB::_is_path_valid(uint32_t dst, BaseQueue* port) {
    // if (port == NULL || port->getFailed()) return false;
    if (!g_enable_bgp) return true; // If BGP not enabled, only check link status
    
    // Check cache first
    uint32_t dest_tor = _ft->HOST_POD_SWITCH(dst);
    int port_idx = -1;
    for (size_t i = 0; i < _ports.size(); ++i) {
        if (_ports[i] == port) {
            port_idx = i;
            break;
        }
    }
    
    if (port_idx != -1 && dest_tor < _path_validity_cache.size()) {
        if (_path_validity_cache[dest_tor].empty()) {
            _path_validity_cache[dest_tor].resize(_ports.size(), 0);
        }
        int8_t status = _path_validity_cache[dest_tor][port_idx];
        if (status != 0) {
            return (status == 1);
        }
    }

    // BGP-like downstream check
    if (_type == TOR) {
        // ToR -> Agg -> ...
        // Find the Agg switch connected to this port
        Switch* next_hop = (Switch*)port->getRemoteEndpoint();
        uint32_t agg_id = next_hop->getID();
        
        uint32_t dest_pod = _ft->HOST_POD(dst);
        uint32_t dest_tor = _ft->HOST_POD_SWITCH(dst);
        uint32_t my_pod = _id / _ft->tor_switches_per_pod(); // _id is ToR ID

        if (dest_pod == my_pod) {
            // Intra-pod: Check Agg -> Dest ToR
            for (uint32_t b = 0; b < _ft->bundlesize(AGG_TIER); b++) {
                if (_ft->queues_nup_nlp[agg_id][dest_tor][b] && !_ft->queues_nup_nlp[agg_id][dest_tor][b]->getFailed()) return true;
                if (_ft->queues_nup_nlp[agg_id][dest_tor][b] && !_ft->queues_nup_nlp[agg_id][dest_tor][b]->getFailed()) {
                    if (port_idx != -1) _path_validity_cache[dest_tor][port_idx] = 1;
                    return true;
                }
            }
        } else {
            // Inter-pod: Check Remote Agg -> Dest ToR
            // Local Agg connects to Core which connects to Remote Agg at same position
            uint32_t podpos = agg_id % _ft->agg_switches_per_pod();
            uint32_t dest_agg = _ft->MIN_POD_AGG_SWITCH(dest_pod) + podpos;
             for (uint32_t b = 0; b < _ft->bundlesize(AGG_TIER); b++) {
                if (_ft->queues_nup_nlp[dest_agg][dest_tor][b] && !_ft->queues_nup_nlp[dest_agg][dest_tor][b]->getFailed()) {
                    if (port_idx != -1) _path_validity_cache[dest_tor][port_idx] = 1;
                    return true;
                }
            }
        }
        if (port_idx != -1) _path_validity_cache[dest_tor][port_idx] = 2;
        return false;
    } else if (_type == AGG) {
        // Agg -> Core -> ...
        Switch* next_hop = (Switch*)port->getRemoteEndpoint();
        uint32_t core_id = next_hop->getID();
        
        uint32_t dest_pod = _ft->HOST_POD(dst);
        uint32_t dest_agg = _ft->MIN_POD_AGG_SWITCH(dest_pod) + (core_id % _ft->agg_switches_per_pod());
        uint32_t dest_tor = _ft->HOST_POD_SWITCH(dst);

        // Check Core -> Dest Agg
        bool core_to_agg = false;
        for (uint32_t b = 0; b < _ft->bundlesize(CORE_TIER); b++) {
            if (_ft->queues_nc_nup[core_id][dest_agg][b] && !_ft->queues_nc_nup[core_id][dest_agg][b]->getFailed()) {
                core_to_agg = true; break;
            }
        }
        if (!core_to_agg) {
            if (port_idx != -1) _path_validity_cache[dest_tor][port_idx] = 2;
            return false;
        }

        // Check Dest Agg -> Dest ToR
        for (uint32_t b = 0; b < _ft->bundlesize(AGG_TIER); b++) {
            if (_ft->queues_nup_nlp[dest_agg][dest_tor][b] && !_ft->queues_nup_nlp[dest_agg][dest_tor][b]->getFailed()) {
                if (port_idx != -1) _path_validity_cache[dest_tor][port_idx] = 1;
                return true;
            }
        }
        if (port_idx != -1) _path_validity_cache[dest_tor][port_idx] = 2;
        return false;
    }
    return true;
}

static int gcd(int a, int b) {
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

void FatTreeSwitchDRB::permute_schedule(vector<BaseQueue *>* schedule) {
    int len = schedule->size();
    for (int i = 0; i < len; i++) {
        int ix = random() % (len - i);
        BaseQueue* tmpsched = (*schedule)[ix];
        (*schedule)[ix] = (*schedule)[len-1-i];
        (*schedule)[len-1-i] = tmpsched;
    }
}

std::vector<BaseQueue*> FatTreeSwitchDRB::_generate_llss_schedule(const std::map<BaseQueue*, int>& weights, bool exp_mode) {
    std::vector<BaseQueue*> schedule;

    int common_divisor = 0;
    for (auto const& entry : weights) {
        // BaseQueue* q = entry.first;
        int w = entry.second;
        if (w > 0) {
            if (common_divisor == 0) common_divisor = w;
            else common_divisor = gcd(common_divisor, w);
        }
    }
    if (common_divisor == 0) common_divisor = 1;

    int total_weight_sum = 0;
    std::map<BaseQueue*, int> w_prime;
    std::vector<BaseQueue*> ports;

    // Build ports list in deterministic order (by port index)
    for (size_t i = 0; i < _ports.size(); i++) {
        BaseQueue* q = (BaseQueue*)_ports[i];
        if (weights.count(q)) {
            int w = weights.at(q);
            if (w > 0) {
                int count = w / common_divisor;
                w_prime[q] = count;
                total_weight_sum += count;
                ports.push_back(q);
            }
        }
    }

    if (!exp_mode) {
        // LLSS / LLSS-A-Rand: Shuffle unique ports
        std::shuffle(ports.begin(), ports.end(), _rng);
    }
    // LLSS-RR / exp_mode: Keep fixed order

    // Round-robin decrement (Interleaved)
    bool active = true;
    while (active) {
        active = false;
        for (BaseQueue* q : ports) {
            if (w_prime[q] > 0) {
                schedule.push_back(q);
                w_prime[q]--;
                active = true;
            }
        }
    }

    if (schedule.size() != (size_t)total_weight_sum) {
        cout << "ALERT: Schedule length mismatch! Expected " << total_weight_sum << ", got " << schedule.size() << endl;
        cout << "Weights (GCD=" << common_divisor << "): ";
        for (auto const& entry : weights) {
            cout << entry.first->nodename() << ":" << entry.second << " ";
        }
        cout << endl;
    }
    return schedule;
}

std::map<BaseQueue*, int> FatTreeSwitchDRB::_get_llss_leaf_weights(uint32_t dest_pod_id) {
    std::map<BaseQueue*, int> weights;
    map<uint32_t, BaseQueue*> up_ports = getUpPorts(); // Key is Agg ID
    
    uint32_t my_pod_id = _id / _ft->tor_switches_per_pod();

    for (auto const& entry : up_ports) {
        uint32_t agg_id = entry.first;
        BaseQueue* port = entry.second;
        if (port == NULL || port->getFailed()) {
            weights[port] = 0;
            continue;
        }
        
        if (dest_pod_id == my_pod_id) {
            weights[port] = 1;
        } else {
            // Symmetric Spine Proxy Logic
            uint32_t src_spine_idx = agg_id % _ft->agg_switches_per_pod();
            uint32_t dest_spine_id = _ft->MIN_POD_AGG_SWITCH(dest_pod_id) + src_spine_idx;

            std::vector<uint32_t> cores_src = _ft->get_reachable_cores(agg_id, AGG);
            std::vector<uint32_t> cores_dest = _ft->get_reachable_cores(dest_spine_id, AGG);

            std::set<uint32_t> s_src(cores_src.begin(), cores_src.end());
            std::set<uint32_t> s_dest(cores_dest.begin(), cores_dest.end());
            
            std::vector<uint32_t> intersection;
            std::set_intersection(s_src.begin(), s_src.end(), s_dest.begin(), s_dest.end(), std::back_inserter(intersection));
            weights[port] = intersection.size();
        }
    }
    return weights;
}

std::map<BaseQueue*, int> FatTreeSwitchDRB::_get_llss_spine_weights(uint32_t dest_pod_id) {
    std::map<BaseQueue*, int> weights;
    map<uint32_t, BaseQueue*> up_ports = getUpPorts(); // Key is Core ID

    uint32_t src_spine_idx = _id % _ft->agg_switches_per_pod();
    uint32_t dest_spine_id = _ft->MIN_POD_AGG_SWITCH(dest_pod_id) + src_spine_idx;
    std::vector<uint32_t> cores_dest = _ft->get_reachable_cores(dest_spine_id, AGG);
    std::set<uint32_t> s_dest(cores_dest.begin(), cores_dest.end());

    for (auto const& entry : up_ports) {
        uint32_t core_id = entry.first;
        BaseQueue* port = entry.second;
        if (port == NULL || port->getFailed()) {
            weights[port] = 0;
            continue;
        }
        // Weight is 1 if this core connects to the symmetric spine in dest pod
        weights[port] = (s_dest.find(core_id) != s_dest.end()) ? 1 : 0;
    }
    return weights;
}

std::vector<BaseQueue*> FatTreeSwitchDRB::_generate_weighted_llss_schedule(uint32_t dest_pod_id) {
    std::map<BaseQueue*, int> weights;
    if (_type == TOR) {
        weights = _get_llss_leaf_weights(dest_pod_id);
    } else {
        weights = _get_llss_spine_weights(dest_pod_id);
    }
    bool exp_mode = (_strategy == PER_POD_IWRR_EXP || _strategy == PER_POD_IWRR_DET || _strategy == PER_POD_IWRR_A_RR);
    return _generate_llss_schedule(weights, exp_mode);
}

FatTreeSwitchDRB::routing_strategy FatTreeSwitchDRB::_strategy = FatTreeSwitchDRB::NIX;
uint16_t FatTreeSwitchDRB::_ar_fraction = 0;
uint16_t FatTreeSwitchDRB::_ar_sticky = FatTreeSwitchDRB::PER_PACKET;
simtime_picosec FatTreeSwitchDRB::_sticky_delta = timeFromUs((uint32_t)10);
double FatTreeSwitchDRB::_ecn_threshold_fraction = 0.5;
double FatTreeSwitchDRB::_speculative_threshold_fraction = 0.2;
int8_t (*FatTreeSwitchDRB::fn)(FibEntry*,FibEntry*)= &FatTreeSwitchDRB::compare_queuesize;

Route* FatTreeSwitchDRB::getNextHop(Packet& pkt, BaseQueue* ingress_port){
    vector<FibEntry*> * available_hops = _fib->getWeighted() ? _fib->getWeightedRoutes(pkt.dst()) : _fib->getRoutes(pkt.dst());
    
    if (available_hops){
        //implement a form of ECMP hashing; might need to revisit based on measured performance.
        uint32_t ecmp_choice = 0;
        // int num_options = (int) available_hops->size();
        if (available_hops->size()>1)
            switch(_strategy){
            case NIX:
                abort();
            case ECMP:
                ecmp_choice = freeBSDHashDRB(pkt.flow_id(),pkt.pathid(),_hash_salt) % available_hops->size();
                break;
            case ADAPTIVE_ROUTING:
                if (_ar_sticky==FatTreeSwitchDRB::PER_PACKET){
                    ecmp_choice = adaptive_route(available_hops,fn); 
                } 
                else if (_ar_sticky==FatTreeSwitchDRB::PER_FLOWLET){     
                    if (_flowlet_maps.find(pkt.flow_id())!=_flowlet_maps.end()){
                        FlowletInfoDRB* f = _flowlet_maps[pkt.flow_id()];
                        
                        // only reroute an existing flow if its inter packet time is larger than _sticky_delta and
                        // and
                        // 50% chance happens. 
                        // and (commented out) if the switch has not taken any other placement decision that we've not seen the effects of.
                        if (eventlist().now() - f->_last > _sticky_delta && /*eventlist().now() - _last_choice > _pipe->delay() + BaseQueue::_update_period  &&*/ random()%2==0){ 
                            //cout << "AR 1 " << timeAsUs(eventlist().now()) << endl;
                            uint32_t new_route = adaptive_route(available_hops,fn); 
                            if (fn(available_hops->at(f->_egress),available_hops->at(new_route)) < 0){
                                f->_egress = new_route;
                                _last_choice = eventlist().now();
                                //cout << "Switch " << _type << ":" << _id << " choosing new path "<<  f->_egress << " for " << pkt.flow_id() << " at " << timeAsUs(eventlist().now()) << " last is " << timeAsUs(f->_last) << endl;
                            }
                        }
                        ecmp_choice = f->_egress;

                        f->_last = eventlist().now();
                    }
                    else {
                        //cout << "AR 2 " << timeAsUs(eventlist().now()) << endl;
                        ecmp_choice = adaptive_route(available_hops,fn); 
                        _last_choice = eventlist().now();

                        _flowlet_maps[pkt.flow_id()] = new FlowletInfoDRB(ecmp_choice,eventlist().now());
                    }
                }

                break;
            case ECMP_ADAPTIVE:
                ecmp_choice = freeBSDHashDRB(pkt.flow_id(),pkt.pathid(),_hash_salt) % available_hops->size();
                if (random()%100 < 50)
                    ecmp_choice = replace_worst_choice(available_hops,fn, ecmp_choice);
                break;
            case RR:
                if (_crt_route>=5 * available_hops->size()){
                    _crt_route = 0;
                    permute_paths(available_hops);
                }
                ecmp_choice = _crt_route % available_hops->size();
                _crt_route ++;
                break;
            case RR_ECMP:
                if (_type == TOR){
                    if (_crt_route>=5 * available_hops->size()){
                        _crt_route = 0;
                        permute_paths(available_hops);
                    }
                    ecmp_choice = _crt_route % available_hops->size();
                    _crt_route ++;
                }
                else ecmp_choice = freeBSDHashDRB(pkt.flow_id(),pkt.pathid(),_hash_salt) % available_hops->size();
                
                break;
            case FIB_LLSS_PERMUTE:
            case FIB_LLSS:
                if (_ft->has_failures()) {
                    // Use LLSS-style Round Robin on valid ports (available_hops)
                    uint32_t dest_id;
                    if (_type == TOR) {
                        dest_id = _ft->HOST_POD_SWITCH(pkt.dst());
                    } else {
                        dest_id = _ft->HOST_POD(pkt.dst());
                    }
                    bool is_ack = (pkt.type() == CONSTCCAACK || pkt.type() == SWIFTACK);
                    uint32_t ptr_idx = dest_id * 2 + (is_ack ? 1 : 0);
                    if (_llss_pointers[ptr_idx] == UINT32_MAX || 
                        (_strategy == FIB_LLSS_PERMUTE && (_llss_pointers[ptr_idx] - _llss_pointer_wraparounds[ptr_idx] > 5 * available_hops->size()))) {
                        uint32_t val = _rng();
                        _llss_pointers[ptr_idx] = val;
                        _llss_pointer_wraparounds[ptr_idx] = val;
                    }
                    ecmp_choice = _llss_pointers[ptr_idx] % available_hops->size();
                    _llss_pointers[ptr_idx]++;
                    break;
                }
                // Fall through
            case PER_POD_IWRR:
            case PER_POD_IWRR_PERMUTE:
                {
                    // Check for downlink first (host directly connected or in same pod for Agg)
                    // If available_hops has a DOWN route, take it.
                    // In FatTree, if dst is in subtree, there is only 1 path down.
                    if (available_hops->at(0)->getDirection() == DOWN) {
                        ecmp_choice = freeBSDHashDRB(pkt.flow_id(),pkt.pathid(),_hash_salt) % available_hops->size();
                    } else {
                        // Uplink logic
                        uint32_t dest_pod = _ft->HOST_POD(pkt.dst());
                        uint32_t dest_leaf = _ft->HOST_POD_SWITCH(pkt.dst());
                        bool is_ack = (pkt.type() == CONSTCCAACK || pkt.type() == SWIFTACK);
                        
                        std::vector<BaseQueue*>* schedule = &_llss_schedules[dest_pod];
                        
                        if (schedule->empty()) {
                            *schedule = _generate_weighted_llss_schedule(dest_pod);
                        }

                        if (schedule->empty()) {
                            // No valid path found
                            return NULL;
                        }

                        // Pointer Logic
                        uint32_t ptr_key_id = (_type == TOR) ? dest_leaf : dest_pod;
                        uint32_t ptr_idx = ptr_key_id * 2 + (is_ack ? 1 : 0);

                        if (_llss_pointers[ptr_idx] == UINT32_MAX) {
                            uint32_t m = schedule->size();
                            uint32_t i;
                            
                            i = _rng() % m;
                            
                            _llss_pointers[ptr_idx] = i;
                            _llss_pointer_wraparounds[ptr_idx] = 0;

                            // For non-adaptive algorithms, initialize the pair pointer simultaneously with an offset
                            if (_strategy != PER_POD_IWRR_A_RR && _strategy != PER_POD_IWRR_A_RAND) {
                                uint32_t pair_idx = ptr_key_id * 2 + (is_ack ? 0 : 1);
                                if (is_ack) {
                                    _llss_pointers[pair_idx] = (i + m/2) % m;
                                    _llss_pointer_wraparounds[pair_idx] = 0;
                                } else {
                                    _llss_pointers[pair_idx] = (i - (m/2)) % m;
                                    _llss_pointer_wraparounds[pair_idx] = 0;
                                }
                            }
                        }
                        
                        uint32_t ptr = _llss_pointers[ptr_idx];

                        if (_strategy == PER_POD_IWRR_PERMUTE &&  _llss_pointer_wraparounds[ptr_idx] >= 5) {
                            permute_schedule(schedule);
                            uint32_t val = _rng() % schedule->size();
                            _llss_pointers[ptr_idx] = val;
                            _llss_pointer_wraparounds[ptr_idx] = 0;
                        }

                        BaseQueue* chosen_q = nullptr;
                        
                        // Iterate to find valid hop
                        for (size_t i = 0; i < schedule->size(); i++) {
                            BaseQueue* candidate = schedule->at((ptr + i) % schedule->size());
                            if (_is_path_valid(pkt.dst(), candidate)) {
                                chosen_q = candidate;
                                uint32_t new_ptr = (ptr + i + 1);
                                if (new_ptr % schedule->size() != new_ptr) {
                                    new_ptr = new_ptr % schedule->size();
                                    _llss_pointer_wraparounds[ptr_idx]++;
                                }
                                _llss_pointers[ptr_idx] = new_ptr;
                                break;
                            }
                        }

                        if (chosen_q == nullptr) {
                            return NULL;
                        }
                        
                        // We need to return a Route* containing this queue. 
                        // Since getNextHop returns Route*, and we don't have the Route object from FIB easily mapped back from Queue...
                        // Actually, available_hops contains FibEntries which contain Routes.
                        // We need to find the FibEntry that corresponds to chosen_q.
                        bool found = false;
                        for (size_t i = 0; i < available_hops->size(); i++) {
                            if (available_hops->at(i)->getEgressPort()->at(0) == chosen_q) {
                                ecmp_choice = i;
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            ecmp_choice = 0;
                        }
                    }
                }
                break;
            }
        // std::cout << "Destination " << pkt.dst() << " choice " << ecmp_choice << " options " << available_hops->size() << std::endl;
        FibEntry* e = (*available_hops)[ecmp_choice];
        pkt.set_direction(e->getDirection());
        return e->getEgressPort();
    } else if (_type == TOR &&  _ft->HOST_POD_SWITCH(pkt.dst()) == _id) {
        //this host is directly connected!
        HostFibEntry* fe = _fib->getHostRoute(pkt.dst(),pkt.flow_id());
        assert(fe);
        pkt.set_direction(DOWN);
        return fe->getEgressPort();
    } else { // We have not seen a packet with this destination yet, figure out its route
        bool added = addRoute(pkt.dst());
        if (added) {
            return getNextHop(pkt, ingress_port);
        }
        else { // This is a black hole (probably due to failure that hasn't been resolved)
            return NULL;
        }
    }
    
}


bool FatTreeSwitchDRB::addRoute(uint32_t dst) {
    //no route table entries for this destination. Add them to FIB or fail. 
    if (_type == TOR){
        // std::cout << "Adding route to TOR switch" << std::endl;s
        if ( _ft->HOST_POD_SWITCH(dst) == _id) {
            //this host is directly connected! should be directly set elsewhere
            return false;
        }
        //route packet up!
        _up_destinations.push_back(dst);
        if (_uproutes && _strategy == RR) // this avoids calculating the uproutes for all destinations (can keep for now since no failures on ToR uplinks) <- wrong, might be weighted based on destination
            // WARNING: including this for now because it is important to RR performance. We do need it to be per-dst if there are failures (turn this off then or write around this)
            _fib->setRoutes(dst,_uproutes);
        else {
            uint32_t podid,agg_min,agg_max;

            if (_ft->get_tiers()==3) {
                podid = _id / _ft->tor_switches_per_pod();
                agg_min = _ft->MIN_POD_AGG_SWITCH(podid);
                agg_max = _ft->MAX_POD_AGG_SWITCH(podid);
            }
            else {
                agg_min = 0;
                agg_max = _ft->getNAGG()-1;
            }

            for (uint32_t k=agg_min; k<=agg_max;k++){
                for (uint32_t b = 0; b < _ft->bundlesize(AGG_TIER); b++) {
                    Route * r = new Route();
                    r->push_back(_ft->queues_nlp_nup[_id][k][b]);
                    assert(((BaseQueue*)r->at(0))->getSwitch() == this);

                    r->push_back(_ft->pipes_nlp_nup[_id][k][b]);
                    r->push_back(_ft->queues_nlp_nup[_id][k][b]->getRemoteEndpoint());
                    int weight = 1;
                    if (_fib->getWeighted()) {
                        string neighborname = _ft->queues_nlp_nup[_id][k][b]->getRemoteEndpoint()->nodename();
                        weight = _switch_weights.find(neighborname) == _switch_weights.end() ? 1 : _switch_weights[neighborname];
                    }

                    _fib->addRoute(dst,r,1,UP,weight);
                }
            }
            _uproutes = _fib->getRoutes(dst);
            permute_paths(_uproutes);
        }

    } else if (_type == AGG) {
        // std::cout << "Adding route to AGG switch " << _id << std::endl;
        if ( _ft->get_tiers()==2 || _ft->HOST_POD(dst) == _ft->AGG_SWITCH_POD_ID(_id)) {
            //must go down!
            //target NLP id is 2 * pkt.dst()/K
            // std::cout << "Down route" << std::endl;
            uint32_t target_tor = _ft->HOST_POD_SWITCH(dst);
            for (uint32_t b = 0; b < _ft->bundlesize(AGG_TIER); b++) {
                Route * r = new Route();
                r->push_back(_ft->queues_nup_nlp[_id][target_tor][b]);
                assert(((BaseQueue*)r->at(0))->getSwitch() == this);

                r->push_back(_ft->pipes_nup_nlp[_id][target_tor][b]);          
                r->push_back(_ft->queues_nup_nlp[_id][target_tor][b]->getRemoteEndpoint());

                int weight = 1;
                if (_fib->getWeighted()) {
                    string neighborname = _ft->queues_nup_nlp[_id][target_tor][b]->getRemoteEndpoint()->nodename();
                    weight = _switch_weights.find(neighborname) == _switch_weights.end() ? 1 : _switch_weights[neighborname];
                }

                _fib->addRoute(dst,r,1, DOWN, weight);
            }
        } else {
            //go up!
            // std::cout << "Up route" << std::endl;
            _up_destinations.push_back(dst);
            
            uint32_t podpos = _id % _ft->agg_switches_per_pod();
            uint32_t uplink_bundles = _ft->radix_up(AGG_TIER) / _ft->bundlesize(CORE_TIER);
            for (uint32_t l = 0; l <  uplink_bundles ; l++) {
                uint32_t core = l * _ft->agg_switches_per_pod() + podpos;
                uint32_t dst_agg =  _ft->MIN_POD_AGG_SWITCH(_ft->HOST_POD(dst))   + podpos;  // find first switch of destination pod and move to pod position
                for (uint32_t b = 0; b < _ft->bundlesize(CORE_TIER); b++) {
                    // std::cout << "Checking core router " << core  << " and dst agg " << dst_agg << std::endl;
                    // Check if the link between the core switch and the destination AGG switch is up TODO: make this general. Add a method to fattreetop to check if a down path is up
                    if (_ft->queues_nc_nup[core][dst_agg][b] == NULL || _ft->queues_nc_nup[core][dst_agg][b]->getFailed()) {
                        // std::cout << " no down link from core " << std::endl;
                        continue;
                    }
                    // Check if the link between this switch and the core is up
                    if (_ft->queues_nup_nc[_id][core][b] == NULL || _ft->queues_nup_nc[_id][core][b]->getFailed()) { 
                        // std::cout << " no up link to core " << std::endl;
                        continue;
                    }

                    Route *r = new Route();
                    r->push_back(_ft->queues_nup_nc[_id][core][b]);
                    assert(((BaseQueue*)r->at(0))->getSwitch() == this);

                    r->push_back(_ft->pipes_nup_nc[_id][core][b]);
                    r->push_back(_ft->queues_nup_nc[_id][core][b]->getRemoteEndpoint());

                    int weight = 1;
                    if (_fib->getWeighted()) {
                        string neighborname = _ft->queues_nup_nc[_id][core][b]->getRemoteEndpoint()->nodename();
                        weight = _switch_weights.find(neighborname) == _switch_weights.end() ? 1 : _switch_weights[neighborname];
                    }
                
                    _fib->addRoute(dst,r,1,UP,weight);

                    // cout << "AGG switch " << _id << " adding route to " << pkt.dst() << " via CORE " << core << " bundle_id " << b << endl;
                }
            }
        }
    } else if (_type == CORE) { 
        // std::cout << "Adding route to CORE switch" << std::endl;
        uint32_t nup = _ft->MIN_POD_AGG_SWITCH(_ft->HOST_POD(dst)) + (_id % _ft->agg_switches_per_pod());
        for (uint32_t b = 0; b < _ft->bundlesize(CORE_TIER); b++) {
            // Do not add the route if the link is down
            if (_ft->queues_nc_nup[_id][nup][b] == NULL || _ft->queues_nc_nup[_id][nup][b]->getFailed()) {
                continue;
            }

            Route *r = new Route();
            // cout << "CORE switch " << _id << " adding route to " << pkt.dst() << " via AGG " << nup << endl;

            assert (_ft->queues_nc_nup[_id][nup][b]);
            r->push_back(_ft->queues_nc_nup[_id][nup][b]);
            assert(((BaseQueue*)r->at(0))->getSwitch() == this);

            assert (_ft->pipes_nc_nup[_id][nup][b]);
            r->push_back(_ft->pipes_nc_nup[_id][nup][b]);

            r->push_back(_ft->queues_nc_nup[_id][nup][b]->getRemoteEndpoint());

            int weight = 1;
            if (_fib->getWeighted()) {
                string neighborname = _ft->queues_nc_nup[_id][nup][b]->getRemoteEndpoint()->nodename();
                weight = _switch_weights.find(neighborname) == _switch_weights.end() ? 1 : _switch_weights[neighborname];
            }
            _fib->addRoute(dst,r,1,DOWN,weight);
        }
    }
    else {
        cerr << "Route lookup on switch with no proper type: " << _type << endl;
        abort();
    }
    // assert(_fib->getRoutes(dst)); This may happen for packets at a router when its downlink goes down
    return (_fib->getRoutes(dst) != NULL);
};
