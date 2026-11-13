#ifndef GRAPH_H
#define GRAPH_H

#include <string>
#include <vector>
#include <unordered_map>

using namespace std;

struct dest {
    string target_dest;
    double distance;
};

class Graph {
private:
    unordered_map<string, vector<dest>> list;

public:
    void addLocation(const string& location);
    void addRoute(const string& u, const string& v, double distance);
    void displayNetwork() const;
    bool locationExists(const string& location) const;
    const unordered_map<string, vector<dest>>& getlist() const;
};

#endif
