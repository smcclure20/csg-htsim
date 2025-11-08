// -*- c-basic-offset: 4; indent-tabs-mode: nil -*-  

#include "failure_manager.h"
#include "fat_tree_switch.h"
#include <assert.h>

FailureManager::FailureManager(FatTreeTopology* ft, EventList& eventlist, simtime_picosec fail_time, simtime_picosec route_time, simtime_picosec weight_time, FailType failure_type) 
: EventSource(eventlist, "failure_manager") {
    _ft = ft;
    _eventlist.sourceIsPending(*this, fail_time);
    _failure_type = failure_type;
    _fail_time = fail_time;
    _route_update_time = route_time;
    _weight_update_time = weight_time;
};

void FailureManager::setFailedLink(uint32_t switch_type, uint32_t id, uint32_t link_id) {
    assert(switch_type == FatTreeSwitch::AGG);
    _switch_type = switch_type;
    _switch_id = id;
    _link_number = link_id;
}

void FailureManager::doNextEvent() { 
    if (!_link_failed && _eventlist.now() >= _fail_time) {
        std::cout << "Failing link " << _switch_type << " " << _switch_id << ": link " << _link_number <<  " at time " << _eventlist.now() << std::endl;
        _ft->add_failed_link(_switch_type, 0, 0, _failure_type);
        _link_failed = true;
        _eventlist.sourceIsPending(*this, _route_update_time);
        _eventlist.sourceIsPending(*this, _weight_update_time);
    } else {
        if (!_routes_updated && _eventlist.now() >= _route_update_time) {
            std::cout << "Updating routes " << _eventlist.now() << std::endl;
            _ft->update_routes(_switch_id);
            _routes_updated = true;
            uint32_t podpos = _switch_id % _ft->agg_switches_per_pod();
            for (uint32_t p = 0; p < _ft->no_of_pods(); p++) {
                uint32_t agg =  (p * _ft->agg_switches_per_pod())  + podpos; 
                _ft->update_routes(agg);
            }
        }
        if (!_weights_updated && _eventlist.now() >= _weight_update_time) {
            std::cout << "Updating weights " << _eventlist.now() << std::endl;
            
            _ft->update_weights_tor_otherpod(_switch_id % _ft->agg_switches_per_pod());
            _ft->update_weights_tor_podfailed(_switch_id % _ft->agg_switches_per_pod());
            _weights_updated = true;
        }
    }
}