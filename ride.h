#ifndef RIDE_H
#define RIDE_H

#include <string>
#include <vector>

enum ActionType {
    BOOK,
    COMPLETE
};

struct riderequest {
    int id;
    int user_id; // jo user id pass bnaega us se leni hogi ye id
    std::string initial_destination;
    std::string final_destination;
    int time;            
    std::string mode;    
    double budget;
};

// completed ride record
struct Complete_Ride {
    int id;
    std::vector<int> user_ids;
    std::string initial_destination;
    std::string final_destination;
    std::string mode;
    double fare;
};

//for the Undo Stack
struct RideAction {
    ActionType type;
    riderequest req;     
    Complete_Ride rec;     
};

#endif