#include "route_manager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <queue>
#include <unordered_map>
#include <algorithm>
#include <limits>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;

const double INF = numeric_limits<double>::infinity();

static double haversineKm(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371.0;
    double toRad = M_PI / 180.0;
    double dLat = (lat2 - lat1) * toRad;
    double dLon = (lon2 - lon1) * toRad;
    double a = sin(dLat / 2.0) * sin(dLat / 2.0) +
               cos(lat1 * toRad) * cos(lat2 * toRad) *
               sin(dLon / 2.0) * sin(dLon / 2.0);
    double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
    return R * c;
}

static string toLowerStr(const string& s) {
    string res = s;
    transform(res.begin(), res.end(), res.begin(), [](unsigned char c){ return tolower(c); });
    return res;
}

RouteManager::RouteManager() {
    // 1. Load comprehensive NCR location database from locations.csv
    loadLocationsFromCSV("locations.csv");

    // 2. Add fallback classic routes
    addNewRoute("Sector 62, Noida", "Sector 18, Noida", 8.2);
    addNewRoute("Sector 18, Noida", "Botanical Garden, Noida", 2.1);
    addNewRoute("Sector 62, Noida", "Botanical Garden, Noida", 9.8);
    addNewRoute("Sector 62, Noida", "Connaught Place, Delhi", 17.6);
    addNewRoute("Sector 18, Noida", "Connaught Place, Delhi", 16.0);
    addNewRoute("Botanical Garden, Noida", "Connaught Place, Delhi", 15.5);
    addNewRoute("Connaught Place, Delhi", "India Gate, Delhi", 3.2);
    addNewRoute("Botanical Garden, Noida", "Rajiv Chowk", 15.0);
    addNewRoute("Dwarka Sector 21", "Rajiv Chowk", 28.4);

    // Common query variants without commas
    addNewRoute("Sector 62 Noida", "Sector 18 Noida", 8.2);
    addNewRoute("Sector 18 Noida", "Botanical Garden Noida", 2.1);
    addNewRoute("Sector 62 Noida", "Botanical Garden", 9.8);
    addNewRoute("Sector 62 Noida", "Botanical Garden Noida", 9.8);
    addNewRoute("Sector 62", "Botanical Garden", 9.8);
    addNewRoute("Sector 62 Noida", "Connaught Place Delhi", 17.6);
    addNewRoute("Sector 62", "Connaught Place", 17.6);
    addNewRoute("Botanical Garden", "Connaught Place", 15.5);
    addNewRoute("Connaught Place", "India Gate", 3.2);

    // Compact names
    addNewRoute("Sector62", "TechPark", 3.5);
    addNewRoute("Sector62", "BotanicalGarden", 9.8);
    addNewRoute("TechPark", "ConnaughtPlace", 14.2);
    addNewRoute("BotanicalGarden", "ConnaughtPlace", 15.5);
    addNewRoute("BotanicalGarden", "RajNagar", 12.0);
    addNewRoute("ConnaughtPlace", "RajNagar", 9.5);
}

bool RouteManager::loadLocationsFromCSV(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        file.clear();
        file.open("./" + filename);
    }
    if (!file.is_open()) {
        file.clear();
        file.open("../" + filename);
    }
    if (!file.is_open()) {
        file.clear();
        file.open("locations.csv");
    }
    if (!file.is_open()) {
        cerr << "[RouteManager] Warning: Could not open " << filename << " from current, relative, or parent paths." << endl;
        return false;
    }

    locations.clear();
    string line;
    bool isHeader = true;

    while (getline(file, line)) {
        if (line.empty() || line[0] == '\r') continue;
        if (isHeader) {
            isHeader = false;
            continue;
        }

        stringstream ss(line);
        string name, type, city, latStr, lonStr;

        if (getline(ss, name, ',') &&
            getline(ss, type, ',') &&
            getline(ss, city, ',') &&
            getline(ss, latStr, ',') &&
            getline(ss, lonStr, ',')) {

            // Trim potential whitespace or carriage return
            while (!name.empty() && (name.back() == '\r' || name.back() == ' ')) name.pop_back();
            while (!type.empty() && (type.back() == '\r' || type.back() == ' ')) type.pop_back();
            while (!city.empty() && (city.back() == '\r' || city.back() == ' ')) city.pop_back();
            while (!latStr.empty() && (latStr.back() == '\r' || latStr.back() == ' ')) latStr.pop_back();
            while (!lonStr.empty() && (lonStr.back() == '\r' || lonStr.back() == ' ')) lonStr.pop_back();

            try {
                double lat = stod(latStr);
                double lon = stod(lonStr);
                locations.push_back(LocationRecord(name, type, city, lat, lon));
                addNewLocation(name);
            } catch (...) {}
        }
    }

    file.close();
    cout << "[RouteManager] Loaded " << locations.size() << " verified NCR locations from " << filename << endl;

    // Build connected road network in C++ Graph
    buildNCRRoadConnections();
    return true;
}

void RouteManager::buildNCRRoadConnections() {
    int n = locations.size();
    if (n < 2) return;

    // Connect each location to its nearest neighboring locations within 5.5 km
    for (int i = 0; i < n; ++i) {
        vector<pair<double, int>> neighbors;
        for (int j = 0; j < n; ++j) {
            if (i == j) continue;
            double dist = haversineKm(locations[i].lat, locations[i].lon, locations[j].lat, locations[j].lon);
            neighbors.push_back({dist, j});
        }
        sort(neighbors.begin(), neighbors.end());

        // Connect at least 3 nearest neighbors, and any neighbor within 5.5 km (up to 6)
        int connected = 0;
        for (size_t k = 0; k < neighbors.size() && connected < 6; ++k) {
            double d = neighbors[k].first;
            int j = neighbors[k].second;
            if (connected < 3 || d <= 5.5) {
                // Road distance accounts for curvature (detour factor ~ 1.25)
                double roadDist = round((d * 1.25) * 100.0) / 100.0;
                addNewRoute(locations[i].name, locations[j].name, max(roadDist, 0.5));
                connected++;
            }
        }
    }

    // Connect key arterial transit and expressway corridors across NCR
    vector<pair<string, string>> corridors = {
        {"Rajiv Chowk Metro Station", "Connaught Place"},
        {"Rajiv Chowk Metro Station", "New Delhi Metro Station"},
        {"New Delhi Metro Station", "New Delhi Railway Station"},
        {"Rajiv Chowk Metro Station", "Barakhamba Road Metro Station"},
        {"Barakhamba Road Metro Station", "Mandi House Metro Station"},
        {"Mandi House Metro Station", "Supreme Court Pragati Maidan Metro Station"},
        {"Supreme Court Pragati Maidan Metro Station", "Indraprastha Metro Station"},
        {"Indraprastha Metro Station", "Yamuna Bank Metro Station"},
        {"Yamuna Bank Metro Station", "Akshardham Metro Station"},
        {"Akshardham Metro Station", "Akshardham Temple"},
        {"Akshardham Metro Station", "Mayur Vihar Phase 1 Metro Station"},
        {"Mayur Vihar Phase 1 Metro Station", "Mayur Vihar Pocket 1 Metro Station"},
        {"Mayur Vihar Pocket 1 Metro Station", "New Ashok Nagar Metro Station"},
        {"New Ashok Nagar Metro Station", "Noida Sector 15 Metro Station"},
        {"Noida Sector 15 Metro Station", "Noida Sector 16 Metro Station"},
        {"Noida Sector 16 Metro Station", "Noida Sector 18 Metro Station"},
        {"Noida Sector 18 Metro Station", "Botanical Garden Metro Station"},
        {"Botanical Garden Metro Station", "Golf Course Metro Station"},
        {"Golf Course Metro Station", "Noida City Centre Metro Station"},
        {"Noida City Centre Metro Station", "Noida Sector 34 Metro Station"},
        {"Noida Sector 34 Metro Station", "Noida Sector 52 Metro Station"},
        {"Noida Sector 52 Metro Station", "Noida Sector 61 Metro Station"},
        {"Noida Sector 61 Metro Station", "Noida Sector 59 Metro Station"},
        {"Noida Sector 59 Metro Station", "Noida Sector 62 Metro Station"},
        {"Noida Sector 62 Metro Station", "Noida Electronic City Metro Station"},
        {"Noida Sector 62 Metro Station", "Noida Sector 62"},
        {"Noida Sector 62", "Indirapuram"},
        {"Indirapuram", "Shipra Mall Indirapuram"},
        {"Indirapuram", "Vaishali"},
        {"Vaishali", "Vaishali Metro Station"},
        {"Vaishali Metro Station", "Kaushambi Metro Station"},
        {"Kaushambi Metro Station", "Anand Vihar ISBT Metro Station"},
        {"Anand Vihar ISBT Metro Station", "Anand Vihar Railway Terminal"},
        {"Rajiv Chowk Metro Station", "Central Secretariat Metro Station"},
        {"Central Secretariat Metro Station", "Dilli Haat INA Metro Station"},
        {"Dilli Haat INA Metro Station", "AIIMS Metro Station"},
        {"AIIMS Metro Station", "Hauz Khas Metro Station"},
        {"Hauz Khas Metro Station", "Saket Metro Station"},
        {"Saket Metro Station", "Qutab Minar Metro Station"},
        {"Qutab Minar Metro Station", "Chhatarpur Metro Station"},
        {"Chhatarpur Metro Station", "Sultanpur Metro Station"},
        {"Sultanpur Metro Station", "Guru Dronacharya Metro Station"},
        {"Guru Dronacharya Metro Station", "Sikanderpur Metro Station"},
        {"Sikanderpur Metro Station", "DLF Cyber City Rapid Metro Station"},
        {"DLF Cyber City Rapid Metro Station", "Cyber City"},
        {"Sikanderpur Metro Station", "MG Road Metro Station"},
        {"MG Road Metro Station", "IFFCO Chowk Metro Station"},
        {"IFFCO Chowk Metro Station", "Millennium City Centre Gurugram Metro Station"},
        {"Janakpuri West Metro Station", "Dwarka Mor Metro Station"},
        {"Dwarka Mor Metro Station", "Dwarka Sector 12 Metro Station"},
        {"Dwarka Sector 12 Metro Station", "Dwarka Sector 10 Metro Station"},
        {"Dwarka Sector 10 Metro Station", "Dwarka Sector 21 Metro Station"},
        {"Dwarka Sector 21 Metro Station", "Dwarka Sector 21"},
        {"Dwarka Sector 21 Metro Station", "IGI Airport Terminal 3 Metro Station"},
        {"IGI Airport Terminal 3 Metro Station", "Indira Gandhi International Airport Terminal 3"},
        {"Kashmere Gate Metro Station", "ISBT Kashmere Gate"},
        {"Kashmere Gate Metro Station", "Chandni Chowk Metro Station"},
        {"Kashmere Gate Metro Station", "Dilshad Garden Metro Station"},
        {"Dilshad Garden Metro Station", "Shaheed Nagar"},
        {"Mohan Nagar Metro Station", "Hindon River Metro Station"},
        {"Hindon River Metro Station", "Shaheed Sthal New Bus Adda Metro Station"},
        {"Shaheed Sthal New Bus Adda Metro Station", "Raj Nagar"},
        {"Central Secretariat Metro Station", "Lajpat Nagar Metro Station"},
        {"Lajpat Nagar Metro Station", "Nehru Place Metro Station"},
        {"Nehru Place Metro Station", "Kalkaji Mandir Metro Station"},
        {"Kalkaji Mandir Metro Station", "Sarita Vihar Metro Station"},
        {"Sarita Vihar Metro Station", "Badarpur Border Metro Station"},
        {"Badarpur Border Metro Station", "Old Faridabad Metro Station"},
        {"Old Faridabad Metro Station", "Neelam Chowk Ajronda Metro Station"},
        {"Neelam Chowk Ajronda Metro Station", "Bata Chowk Metro Station"},
        {"Bata Chowk Metro Station", "Escorts Mujesar Metro Station"},
        {"Escorts Mujesar Metro Station", "Raja Nahar Singh Ballabhgarh Metro Station"},
        {"Noida Sector 52 Metro Station", "Noida Sector 51 Metro Station"},
        {"Noida Sector 51 Metro Station", "Noida Sector 76 Metro Station"},
        {"Noida Sector 76 Metro Station", "Noida Sector 137 Metro Station"},
        {"Noida Sector 137 Metro Station", "Noida Sector 142 Metro Station"},
        {"Noida Sector 142 Metro Station", "Pari Chowk Metro Station"},
        {"Pari Chowk Metro Station", "Pari Chowk"},
        {"Pari Chowk Metro Station", "Alpha 1 Metro Station"},
        {"Alpha 1 Metro Station", "Delta 1 Metro Station"},
        {"Delta 1 Metro Station", "GNIDA Office Metro Station"},
        {"GNIDA Office Metro Station", "Depot Metro Station"}
    };

    for (const auto& c : corridors) {
        LocationRecord l1, l2;
        if (findLocationByName(c.first, l1) && findLocationByName(c.second, l2)) {
            double d = haversineKm(l1.lat, l1.lon, l2.lat, l2.lon);
            double roadDist = round((max(d, 0.4) * 1.2) * 100.0) / 100.0;
            addNewRoute(c.first, c.second, roadDist);
        }
    }
}

const vector<LocationRecord>& RouteManager::getAllLocations() const {
    return locations;
}

static string expandAbbreviations(const string& q) {
    string s = toLowerStr(q);
    // Replace punctuation with spaces
    for (char& c : s) {
        if (c == ',' || c == '.' || c == '-' || c == '/' || c == '_') c = ' ';
    }
    // Clean multiple spaces
    string clean = "";
    stringstream ss(s);
    string word;
    while (ss >> word) {
        if (word == "sec" || word == "sect") word = "sector";
        else if (word == "stn" || word == "st") word = "station";
        else if (word == "cp") word = "connaught place";
        else if (word == "ndls") word = "new delhi railway";
        else if (word == "dli") word = "old delhi";
        else if (word == "nzm") word = "hazrat nizamuddin";
        else if (word == "anvt") word = "anand vihar";
        else if (word == "igi") word = "indira gandhi";
        else if (word == "gn" || word == "gr noida") word = "greater noida";
        else if (word == "gzb") word = "ghaziabad";
        else if (word == "ggn" || word == "gurgaon") word = "gurugram";
        else if (word == "fbd") word = "faridabad";
        clean += (clean.empty() ? "" : " ") + word;
    }
    return clean.empty() ? s : clean;
}

vector<LocationRecord> RouteManager::searchLocations(const string& query, int maxResults) const {
    vector<LocationRecord> results;
    if (query.empty()) return results;

    string qRaw = toLowerStr(query);
    string qExpanded = expandAbbreviations(query);

    struct ScoredMatch {
        int score;
        LocationRecord loc;
        ScoredMatch(int s, const LocationRecord& l) : score(s), loc(l) {}
    };
    vector<ScoredMatch> scored;

    for (const auto& loc : locations) {
        string nameLower = toLowerStr(loc.name);
        string cityLower = toLowerStr(loc.city);
        string typeLower = toLowerStr(loc.type);
        int score = 0;

        // Exact match
        if (nameLower == qRaw || nameLower == qExpanded) {
            score = 120;
        }
        // Name starts with query
        else if (nameLower.find(qRaw) == 0 || nameLower.find(qExpanded) == 0) {
            score = 100;
        }
        // Word in name starts with query (e.g. "62" or "Sector" or "Delhi")
        else if (nameLower.find(" " + qRaw) != string::npos || nameLower.find(" " + qExpanded) != string::npos) {
            score = 90;
        }
        // Substring inside location name
        else if (nameLower.find(qRaw) != string::npos || nameLower.find(qExpanded) != string::npos) {
            score = 80;
        }
        // City match + partial name
        else if (cityLower.find(qRaw) == 0 || cityLower.find(qExpanded) == 0) {
            score = 70;
        }
        // Substring in City
        else if (cityLower.find(qRaw) != string::npos || cityLower.find(qExpanded) != string::npos) {
            score = 60;
        }
        // Type match (e.g. "metro", "hospital", "mall")
        else if (typeLower.find(qRaw) != string::npos || typeLower.find(qExpanded) != string::npos) {
            score = 50;
        }

        if (score > 0) {
            scored.push_back(ScoredMatch(score, loc));
        }
    }

    // Sort by highest score first, then shorter names first
    sort(scored.begin(), scored.end(), [](const ScoredMatch& a, const ScoredMatch& b) {
        if (a.score != b.score) return a.score > b.score;
        return a.loc.name.length() < b.loc.name.length();
    });

    for (size_t i = 0; i < scored.size() && (int)results.size() < maxResults; ++i) {
        results.push_back(scored[i].loc);
    }

    return results;
}

LocationRecord RouteManager::findNearestLocation(double lat, double lon) const {
    LocationRecord best;
    double minDist = INF;
    for (const auto& loc : locations) {
        double d = haversineKm(lat, lon, loc.lat, loc.lon);
        if (d < minDist) {
            minDist = d;
            best = loc;
        }
    }
    return best;
}

bool RouteManager::findLocationByName(const string& name, LocationRecord& outLoc) const {
    string target = toLowerStr(name);
    // 1. Exact match
    for (const auto& loc : locations) {
        if (toLowerStr(loc.name) == target) {
            outLoc = loc;
            return true;
        }
    }
    // 2. Contains match
    for (const auto& loc : locations) {
        string ln = toLowerStr(loc.name);
        if (ln.find(target) != string::npos || target.find(ln) != string::npos) {
            outLoc = loc;
            return true;
        }
    }
    return false;
}

vector<pair<double, double>> RouteManager::getPathCoordinates(const vector<string>& path) const {
    vector<pair<double, double>> coords;
    for (const auto& nodeName : path) {
        LocationRecord loc;
        if (findLocationByName(nodeName, loc)) {
            coords.push_back({loc.lat, loc.lon});
        }
    }
    return coords;
}

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
pair<vector<string>, double> RouteManager::getShortestPath(const string& startIn, const string& endIn) const {
    string start = startIn;
    string end = endIn;

    // Resolve node names if not exact match in mapGraph
    if (!mapGraph.locationExists(start)) {
        LocationRecord loc;
        if (findLocationByName(start, loc) && mapGraph.locationExists(loc.name)) {
            start = loc.name;
        }
    }
    if (!mapGraph.locationExists(end)) {
        LocationRecord loc;
        if (findLocationByName(end, loc) && mapGraph.locationExists(loc.name)) {
            end = loc.name;
        }
    }

    if (!mapGraph.locationExists(start) || !mapGraph.locationExists(end)) {
        return {{}, -1.0};
    }

    if (start == end) {
        return {{start}, 0.0};
    }

    const auto& adjList = mapGraph.getlist();

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

        auto it = adjList.find(u);
        if (it == adjList.end()) continue;

        for (const auto& d : it->second) {
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
    if (distances.find(end) == distances.end() || distances[end] == INF) {
        return {path, -1.0};
    }

    string curr = end;
    while (curr != start) {
        path.push_back(curr);
        auto it = previous.find(curr);
        if (it == previous.end()) break;
        curr = it->second;
    }
    path.push_back(start);
    reverse(path.begin(), path.end());

    double totalDist = round(distances[end] * 100.0) / 100.0;
    return {path, totalDist};
}

// Breadth-First Search (BFS) using Queue for Minimum Connections / Hops
vector<string> RouteManager::getBFSPath(const string& startIn, const string& endIn) const {
    string start = startIn;
    string end = endIn;

    if (!mapGraph.locationExists(start)) {
        LocationRecord loc;
        if (findLocationByName(start, loc) && mapGraph.locationExists(loc.name)) {
            start = loc.name;
        }
    }
    if (!mapGraph.locationExists(end)) {
        LocationRecord loc;
        if (findLocationByName(end, loc) && mapGraph.locationExists(loc.name)) {
            end = loc.name;
        }
    }

    vector<string> path;
    if (!mapGraph.locationExists(start) || !mapGraph.locationExists(end)) {
        return path;
    }

    if (start == end) {
        return {start};
    }

    const auto& adjList = mapGraph.getlist();

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

        auto it = adjList.find(u);
        if (it == adjList.end()) continue;

        for (const auto& d : it->second) {
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
        auto it = parent.find(curr);
        if (it == parent.end()) break;
        curr = it->second;
    }
    path.push_back(start);
    reverse(path.begin(), path.end());

    return path;
}
