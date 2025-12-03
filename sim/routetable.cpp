// -*- c-basic-offset: 4; indent-tabs-mode: nil -*-  
#include <climits>
#include "routetable.h"
#include "network.h"
#include "queue.h"
#include "pipe.h"

void RouteTable::addRoute(int destination, Route* port, int cost, packet_direction direction){  
    if (_fib.find(destination) == _fib.end())
        _fib[destination] = new vector<FibEntry*>(); 
    
    assert(port!=NULL);

    _fib[destination]->push_back(new FibEntry(port,cost,direction));
}

void RouteTable::addRoute(int destination, Route* port, int cost, packet_direction direction, int weight){  
    addRoute(destination, port, cost, direction);
    addWeightedRoute(destination, port, cost, direction, weight);
}

void RouteTable::addWeightedRoute(int destination, Route* port, int cost, packet_direction direction, int weight){  
    if (!_weighted) {
        return;
    }
    if (_weighted_fib.find(destination) == _weighted_fib.end())
        _weighted_fib[destination] = new vector<FibEntry*>(); 
    
    assert(port!=NULL);

    for (int i = 0; i < weight; i++) {
        _weighted_fib[destination]->push_back(new FibEntry(port,cost,direction));
    }

}

void RouteTable::addHostRoute(int destination, Route* port, int flowid){  
    if (_hostfib.find(destination) == _hostfib.end())
        _hostfib[destination] = new unordered_map<int, HostFibEntry*>(); 
    
    assert(port!=NULL);

    (*_hostfib[destination])[flowid] = new HostFibEntry(port,flowid);
}


vector<FibEntry*>* RouteTable::getRoutes(int destination){
    if (_fib.find(destination) == _fib.end())
        return NULL;
    else        
        return _fib[destination];
}

vector<FibEntry*>* RouteTable::getWeightedRoutes(int destination){
    if (_weighted_fib.find(destination) == _weighted_fib.end())
        return NULL;
    else        
        return _weighted_fib[destination];
}

HostFibEntry* RouteTable::getHostRoute(int destination,int flowid){
    if (_hostfib.find(destination) == _hostfib.end() ||
        _hostfib[destination]->find(flowid) == _hostfib[destination]->end()) {
            std::cout << "No route found for destination " << destination << " and flowid " << flowid << std::endl;
        return NULL;
    }
    else {
        return (*_hostfib[destination])[flowid];
    }
}

void RouteTable::setRoutes(int destination, vector<FibEntry*>* routes){
    _fib[destination] = routes;
}

void RouteTable::clearRoutes() {
    _fib.clear();
}

vector<int> RouteTable::getDestinations() {
    vector<int> dsts = vector<int> {};
    for (unordered_map<int,vector<FibEntry*>* >::const_iterator it = _fib.begin(); it != _fib.end(); ++it) {
        dsts.push_back(it->first);
    }
    return dsts;
}