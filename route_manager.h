#ifndef ROUTE_MANAGER_H
#define ROUTE_MANAGER_H

#include "graph.h"
#include <string>
#include <vector>
#include <utility>

class RouteManager {
private:
    Graph mapGraph;

public:
    void addNewLocation(const std::string& location);
    void addNewRoute(const std::string& u, const std::string& v, double distance);
    void showNetwork() const;
    
    // Dijkstra's algorithm for shortest distance path
    std::pair<std::vector<std::string>, double> getShortestPath(const std::string& start, const std::string& end) const;
    
    // BFS algorithm for minimum connections / fewest hops path
    std::vector<std::string> getBFSPath(const std::string& start, const std::string& end) const;
};

#endif