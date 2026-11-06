#include "route_manager.h"
#include <iostream>
#include <queue>
#include <unordered_map>
#include <algorithm>
#include <limits>

using namespace std;

const double INF = numeric_limits<double>::infinity();

void RouteManager::addNewLocation(const string& location) {
    mapGraph.addLocation(location);
}

void RouteManager::addNewRoute(const string& u, const string& v, double distance) {
    mapGraph.addRoute(u, v, distance);
}

void RouteManager::showNetwork() const {
    mapGraph.displayNetwork();
}

// Dijkstra's Algorithm using Priority Queue (Min-Heap) for Shortest Path
pair<vector<string>, double> RouteManager::getShortestPath(const string& start, const string& end) const {
    const auto& adjList = mapGraph.getlist();
    
    if (!mapGraph.locationExists(start) || !mapGraph.locationExists(end)) {
        return {{}, -1.0};
    }

    unordered_map<string, double> distances;
    unordered_map<string, string> previous;
    
    priority_queue<pair<double, string>, 
                   vector<pair<double, string>>, 
                   greater<pair<double, string>>> pq;

    for (const auto& pair : adjList) {
        distances[pair.first] = INF;
    }

    distances[start] = 0.0;
    pq.push({0.0, start});

    while (!pq.empty()) {
        double currentDist = pq.top().first;
        string u = pq.top().second;
        pq.pop();

        if (u == end) break;
        if (currentDist > distances[u]) continue;

        for (const auto& d : adjList.at(u)) {
            string v = d.target_dest;
            double weight = d.distance;

            if (distances[u] + weight < distances[v]) {
                distances[v] = distances[u] + weight;
                previous[v] = u;
                pq.push({distances[v], v});
            }
        }
    }

    vector<string> path;
    if (distances[end] == INF) return {path, -1.0};

    string curr = end;
    while (curr != start) {
        path.push_back(curr);
        curr = previous[curr];
    }
    path.push_back(start);
    reverse(path.begin(), path.end());

    return {path, distances[end]};
}

// Breadth-First Search (BFS) using Queue for Minimum Connections
vector<string> RouteManager::getBFSPath(const string& start, const string& end) const {
    const auto& adjList = mapGraph.getlist();
    vector<string> path;

    if (!mapGraph.locationExists(start) || !mapGraph.locationExists(end)) {
        return path;
    }

    unordered_map<string, bool> visited;
    unordered_map<string, string> parent;
    queue<string> q;

    q.push(start);
    visited[start] = true;
    bool found = false;

    while (!q.empty()) {
        string u = q.front();
        q.pop();

        if (u == end) {
            found = true;
            break;
        }

        for (const auto& d : adjList.at(u)) {
            string v = d.target_dest;
            if (!visited[v]) {
                visited[v] = true;
                parent[v] = u;
                q.push(v);
            }
        }
    }

    if (!found) return path;

    string curr = end;
    while (curr != start) {
        path.push_back(curr);
        curr = parent[curr];
    }
    path.push_back(start);
    reverse(path.begin(), path.end());

    return path;
}