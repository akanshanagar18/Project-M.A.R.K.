#include "graph.h"
#include <iostream>

using namespace std;

void Graph::addLocation(const string& location) {
    if (list.find(location) == list.end()) {
        list[location] = vector<dest>();
    }
}

void Graph::addRoute(const string& u, const string& v, double distance) {
    addLocation(u);
    addLocation(v);
    // Push the relatable RoadSegment structs into both locations' adjacency vectors
    list[u].push_back({v, distance});
    list[v].push_back({u, distance}); // Bidirectional commute road
}

void Graph::displayNetwork() const {
    cout << "\n=== TRANSPORT ROUTE NETWORK (GRAPH ADJACENCY LIST) ===\n";
    for (const auto& pair : list) {
        cout << "[" << pair.first << "] connected via roads: ";
        for (const auto& road : pair.second) {
            cout << "-> " << road.target_dest << " (" << road.distance << " km) ";
        }
        cout << "\n";
    }
}

bool Graph::locationExists(const string& location) const {
    return list.find(location) != list.end();
}

const unordered_map<string, vector<dest>>& Graph::getlist() const {
    return list;
}
