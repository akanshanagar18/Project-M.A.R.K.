#ifndef ROUTE_MANAGER_H
#define ROUTE_MANAGER_H

#include "graph.h"
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std;

struct LocationRecord {
    string name;
    string type;
    string city;
    double lat;
    double lon;

    LocationRecord() : name(""), type(""), city(""), lat(0.0), lon(0.0) {}
    LocationRecord(const string &n, const string &t,
                   const string &c, double la, double lo)
        : name(n), type(t), city(c), lat(la), lon(lo) {}
};

struct RoadRouteResult {
    bool success = false;
    double distanceKm = 0.0;
    int hops = 0;
    vector<string> path;
    vector<pair<double, double>> coordinates;
};

class RouteManager {
public:
    struct RoadEdge {
        string target;
        double distanceKm;
        vector<pair<double, double>> geometry;
    };

private:
    Graph mapGraph;
    unordered_map<string, vector<RoadEdge>> roadAdjacency;
    unordered_map<string, pair<double, double>> roadNodeCoordinates;
    bool roadNetworkLoaded = false;
    vector<LocationRecord> locations;

    bool loadRoadNetworkCSV(const string &filename);
    pair<string, double> findNearestRoadNode(double lat, double lon) const;

public:
    RouteManager();
    bool loadLocationsFromCSV(const string &filename);
    const vector<LocationRecord> &getAllLocations() const;
    vector<LocationRecord> searchLocations(const string &query, int maxResults = 10) const;
    LocationRecord findNearestLocation(double lat, double lon) const;
    bool findLocationByName(const string &name, LocationRecord &outLoc) const;
    vector<pair<double, double>> getPathCoordinates(const vector<string> &path) const;

    void addNewLocation(const string &location);
    void addNewRoute(const string &u, const string &v, double distance);
    void showNetwork() const;

    // Dijkstra's algorithm for shortest distance path
    pair<vector<string>, double> getShortestPath(const string &start, const string &end) const;

    // BFS algorithm for minimum connections / fewest hops path
    vector<string> getBFSPath(const string &start, const string &end) const;

    // Road network routing using OSM data
    RoadRouteResult getRoadDijkstraRoute(double startLat, double startLon, double endLat, double endLon) const;
    RoadRouteResult getRoadBFSRoute(double startLat, double startLon, double endLat, double endLon) const;
    bool isRoadNetworkLoaded() const;
};

#endif
