#ifndef RIDE_H
#define RIDE_H

#include <string>
#include <vector>

using namespace std;

enum ActionType {
    BOOK,
    COMPLETE
};

struct riderequest {
    int id;
    int user_id; // jo user id pass bnaega us se leni hogi ye id
    string initial_destination;
    string final_destination;
    int time;            
    string mode;    
    double budget;
};

// completed ride record
struct Complete_Ride {
    int id;
    vector<int> user_ids;
    string initial_destination;
    string final_destination;
    string mode;
    double fare;
};

//for the Undo Stack
struct RideAction {
    ActionType type;
    riderequest req;     
    Complete_Ride rec;     
};

#endif
