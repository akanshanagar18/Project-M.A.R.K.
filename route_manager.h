#ifndef ROUTE_MANAGER_H
#define ROUTE_MANAGER_H

#include "graph.h"
#include <string>
#include <vector>
#include <utility>

struct LocationRecord {
    std::string name;
    std::string type;
    std::string city;
    double lat;
    double lon;

    LocationRecord() : name(""), type(""), city(""), lat(0.0), lon(0.0) {}
    LocationRecord(const std::string& n, const std::string& t, const std::string& c, double la, double lo)
        : name(n), type(t), city(c), lat(la), lon(lo) {}
};

class RouteManager {
private:
    Graph mapGraph;
    std::vector<LocationRecord> locations;

    void buildNCRRoadConnections();

public:
    RouteManager();
    bool loadLocationsFromCSV(const std::string& filename);
    const std::vector<LocationRecord>& getAllLocations() const;
    std::vector<LocationRecord> searchLocations(const std::string& query, int maxResults = 10) const;
    LocationRecord findNearestLocation(double lat, double lon) const;
    bool findLocationByName(const std::string& name, LocationRecord& outLoc) const;
    std::vector<std::pair<double, double>> getPathCoordinates(const std::vector<std::string>& path) const;

    void addNewLocation(const std::string& location);
    void addNewRoute(const std::string& u, const std::string& v, double distance);
    void showNetwork() const;

    // Dijkstra's algorithm for shortest distance path
    std::pair<std::vector<std::string>, double> getShortestPath(const std::string& start, const std::string& end) const;

    // BFS algorithm for minimum connections / fewest hops path
    std::vector<std::string> getBFSPath(const std::string& start, const std::string& end) const;
};

#endif

