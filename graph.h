#ifndef GRAPH_H
#define GRAPH_H

#include <string>
#include <vector>
#include <unordered_map>

struct dest {
    std::string target_dest;
    double distance;
};

class Graph {
private:
    std::unordered_map<std::string, std::vector<dest>> list;

public:
    void addLocation(const std::string& location);
    void addRoute(const std::string& u, const std::string& v, double distance);
    void displayNetwork() const;
    bool locationExists(const std::string& location) const;
    const std::unordered_map<std::string, std::vector<dest>>& getlist() const;
};

#endif