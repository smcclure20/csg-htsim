// -*- c-basic-offset: 4; indent-tabs-mode: nil -*- 

#ifndef FAILMANAGER_H
#define FAILMANAGER_H

#include "../trigger.h"
#include "../clock.h"
#include "../eventlist.h"
#include "fat_tree_topology.h"
#include "fat_tree_switch.h"

#define timeInf 0

class FailureManager: public EventSource {
    public:
        FailureManager(FatTreeTopology* ft, EventList& eventlist, simtime_picosec fail_time, simtime_picosec route_time, simtime_picosec weight_time, FailType failure_type);
        void setFailedLink(uint32_t switch_type, uint32_t id, uint32_t link_id);
        void doNextEvent();
    
    private:
        FatTreeTopology* _ft;
        FailType _failure_type;
        uint32_t _switch_type;
        uint32_t _switch_id;
        uint32_t _link_number;
        simtime_picosec _fail_time;
        bool _link_failed = false;
        simtime_picosec _route_update_time;
        bool _routes_updated= false;
        simtime_picosec _weight_update_time;
        bool _weights_updated = false;

};


#endif
