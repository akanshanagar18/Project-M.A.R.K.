#ifndef RIDE_MANAGER_H
#define RIDE_MANAGER_H

#include "ride.h"
#include <queue>
#include <stack>
#include <vector>

using namespace std;

class Ride_Manager {
private:
    queue<riderequest> pending_queue;
    stack<RideAction> undo_stack;
    vector<Complete_Ride> ride_history;
    
    int next_request_id = 101;
    int next_ride_id = 5001;

public:
    void add_ride_req(int user_id, string initial_dest, string final_dest, int time, string mode, double budget);
    void process_nextride(double fare);
    bool undo(RideAction* undoneAction = nullptr);
    
    void displaypending_queue() const;
    void display_ride_history() const;
    void displaystats() const;

    vector<riderequest> all_pending_requests() const;
    const vector<Complete_Ride>& get_history() const { return ride_history; }
    int get_queue_size() const { return (int)pending_queue.size(); }
    int get_history_size() const { return (int)ride_history.size(); }
    double get_total_revenue() const;
    bool cancel_request(int req_id);
};

#endif
