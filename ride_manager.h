#ifndef RIDE_MANAGER_H
#define RIDE_MANAGER_H

#include "ride.h"
#include <queue>
#include <stack>
#include <vector>

class Ride_Manager {
private:
    std::queue<riderequest> pending_queue;
    std::stack<RideAction> undo_stack;
    std::vector<Complete_Ride> ride_history;
    
    int next_request_id = 101;
    int next_ride_id = 5001;

public:
    void add_ride_req(int user_id, std::string initial_dest, std::string final_dest, int time, std::string mode, double budget);
    void process_nextride(double fare);
    bool undo();
    
    void displaypending_queue() const;
    void display_ride_history() const;
    void displaystats() const;

    std::vector<riderequest> all_pending_requests() const;
    const std::vector<Complete_Ride>& get_history() const { return ride_history; }
    int get_queue_size() const { return (int)pending_queue.size(); }
    int get_history_size() const { return (int)ride_history.size(); }
    double get_total_revenue() const;
    bool cancel_request(int req_id);
};

#endif