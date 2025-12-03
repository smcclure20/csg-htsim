// -*- c-basic-offset: 4; indent-tabs-mode: nil -*-        
#ifndef ROUTETABLE_H
#define ROUTETABLE_H

/*
 * A Route Table resolves switch IDs to output ports. 
 */

#include "queue.h"

#include <list>
#include <vector>
#include <unordered_map>

class FibEntry{
public:
    FibEntry(Route* outport, uint32_t cost, packet_direction direction){ _out = outport; _cost = cost;_direction = direction;}

    Route* getEgressPort(){return _out;}
    uint32_t getCost(){return _cost;}
    packet_direction getDirection(){return _direction;}
    
protected:
    Route* _out;
    uint32_t _cost;
    packet_direction _direction;
};

class HostFibEntry{
public:
    HostFibEntry(Route* outport, int flowid){ _flowid = flowid; _out = outport;}

    Route* getEgressPort(){return _out;}
    int getFlowID(){return _flowid;}

protected:
    Route* _out;
    uint32_t _flowid;

};

// TODO: some easy way to re-weight routes 
class RouteTable {
public:
    RouteTable() {};
    void addRoute(int destination, Route* port, int cost, packet_direction direction);  
    void addRoute(int destination, Route* port, int cost, packet_direction direction, int weight);  
    void addHostRoute(int destination, Route* port, int flowid);  
    void setRoutes(int destination, vector<FibEntry*>* routes);  
    vector <FibEntry*>* getRoutes(int destination);
    vector <FibEntry*>* getWeightedRoutes(int destination);
    void setWeighted(bool weighted) {_weighted=weighted;}
    bool getWeighted() {return _weighted;}
    void initialize_weighted_table() {_weighted_fib = _fib;}
    void clear_weighted_table() { _weighted_fib = unordered_map<int,vector<FibEntry*>* >{};}
    HostFibEntry* getHostRoute(int destination, int flowid);
    bool isEmpty() {return _fib.empty();}
    RouteTable* copy() {RouteTable* rt = new RouteTable(); rt->_weighted=_weighted; rt->_fib = _fib; return rt;}
    void clearRoutes();
    void addWeightedRoute(int destination, Route* port, int cost, packet_direction direction, int weight);  
    vector<int> getDestinations();
    
private:
    unordered_map<int,vector<FibEntry*>* > _fib;
    bool _weighted = false;
    unordered_map<int,vector<FibEntry*>* > _weighted_fib;
    unordered_map<int,unordered_map<int,HostFibEntry*>*> _hostfib;
};

#endif
