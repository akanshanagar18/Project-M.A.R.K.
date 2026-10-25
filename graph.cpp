#include "graph.h"
#include <iostream>

void Graph::addLocation(const std::string& location) {
    if (list.find(location) == list.end()) {
        list[location] = std::vector<dest>();
    }
}

void Graph::addRoute(const std::string& u, const std::string& v, double distance) {
    addLocation(u);
    addLocation(v);
    // Push the relatable RoadSegment structs into both locations' adjacency vectors
    list[u].push_back({v, distance});
    list[v].push_back({u, distance}); // Bidirectional commute road
}

void Graph::displayNetwork() const {
    std::cout << "\n=== TRANSPORT ROUTE NETWORK (GRAPH ADJACENCY LIST) ===\n";
    for (const auto& pair : list) {
        std::cout << "[" << pair.first << "] connected via roads: ";
        for (const auto& road : pair.second) {
            std::cout << "-> " << road.target_dest << " (" << road.distance << " km) ";
        }
        std::cout << "\n";
    }
}

bool Graph::locationExists(const std::string& location) const {
    return list.find(location) != list.end();
}

const std::unordered_map<std::string, std::vector<dest>>& Graph::getlist() const {
    return list;
}