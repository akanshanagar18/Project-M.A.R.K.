#include "ride_manager.h"
#include <iostream>
using namespace std; 

void Ride_Manager::add_ride_req(int user_id, std::string initial_dest, std::string final_dest, int time, std::string mode, double budget) {
    riderequest new_req;
    new_req.id = next_request_id++;
    new_req.user_id = user_id;
    new_req.initial_destination = initial_dest;
    new_req.final_destination = final_dest;
    new_req.time = time;
    new_req.mode = mode;
    new_req.budget = budget;

    pending_queue.push(new_req);

    RideAction action;
    action.type = BOOK;
    action.req = new_req;
    undo_stack.push(action);

    cout << "Ride request added successfully"<< endl<<" RIDE Request ID: " << new_req.id << endl;
}



void Ride_Manager::process_nextride(double fare) {
    if (pending_queue.empty()) {
        cout << "No pending ride requests to process." << endl;
        return;
    }

    // take the first ride from queue for the next matching and remove it from the queue for processing
    riderequest req = pending_queue.front();
    pending_queue.pop();

    Complete_Ride record;
    record.id = next_ride_id++;
    record.user_ids.push_back(req.user_id);
    record.initial_destination = req.initial_destination;
    record.final_destination = req.final_destination;
    record.mode = req.mode;
    record.fare = fare;

    // Store int the vector
    ride_history.push_back(record);

    RideAction action;
    action.type = COMPLETE;
    action.rec = record;
    undo_stack.push(action);

    cout << "Ride processed successfully! Ride ID: " << record.id <<endl;
}



bool Ride_Manager::undo() {
    if (undo_stack.empty()) {
        cout << "no ride to undo." << endl;
        return false;
    }
    
    RideAction last_action = undo_stack.top();
    undo_stack.pop();

    if (last_action.type == BOOK) {
        int target_id = last_action.req.id;
        std::queue<riderequest> temp_queue;
        bool found = false;

        while (!pending_queue.empty()) {
            riderequest current = pending_queue.front();
            pending_queue.pop();
            if (current.id == target_id) {
                found = true;
            } else {
                temp_queue.push(current);
            }
        }
        pending_queue = temp_queue;

        if (found) {
            cout << "RIDE CANCELLED , request ID " << target_id << endl;
            return true;
        }
    } 
    else if (last_action.type == COMPLETE) {
        int target_id = last_action.rec.id;
        for (auto it = ride_history.begin(); it != ride_history.end(); ++it) {
            if (it->id == target_id) {
                ride_history.erase(it);
                cout << "RIDE CANCELLED , completed ride record ID " << target_id << endl;
                return true;
            }
        }
    }

    return false;
}



void Ride_Manager::displaypending_queue() const {
    if (pending_queue.empty()) {
        cout << "NO RIDE IN THE PENDING QUEUE" << endl;
        return;
    }
    std::queue<riderequest> temp = pending_queue;
    cout << "\nPENDING RIDE REQUESTS :" << endl;
    while (!temp.empty()) {
        riderequest r = temp.front();
        temp.pop();
        cout << "ID: " << r.id << " | User: " << r.user_id 
             << " | From: " << r.initial_destination << " To: " << r.final_destination 
             << " | Mode: " << r.mode << " | Budget(in rupees): " << r.budget << endl;
    }
    cout<< endl;
}



void Ride_Manager::display_ride_history() const {
    if (ride_history.empty()) {
        cout << "NO RIDE COMPLETED YET" << endl;
        return;
    }

    cout << "\nCOMPLETED RIDE HISTORY :" << endl;
    for (const auto& r : ride_history) {
        cout << "Ride ID: " << r.id << " | User: " << r.user_ids[0]
             << " | From: " << r.initial_destination << " To: " << r.final_destination
             << " | Mode: " << r.mode << " | Fare: $" << r.fare << endl;
    }
    cout<< endl;
}



void Ride_Manager::displaystats() const {
    double total_revenue = 0.0;
    for (const auto& r : ride_history) {
        total_revenue += r.fare;
    }

    cout << "\nRIDE STATISTICS :" << endl;
    cout << "Total Completed Rides: " << ride_history.size() << endl;
    cout << "Total Revenue Generated: $" << total_revenue << endl;
    cout << endl;
}

// FOR MEMBER 444444444444444444444444444444444444444(FOUR)

std::vector<riderequest> Ride_Manager::all_pending_requests() const {
    std::vector<riderequest> result;
    std::queue<riderequest> temp = pending_queue;
    while (!temp.empty()) {
        result.push_back(temp.front());
        temp.pop();
    }
    return result;
}