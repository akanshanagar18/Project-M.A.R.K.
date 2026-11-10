#include "ride_manager.h"
#include <iostream>
using namespace std;

int Ride_Manager::add_ride_req(int user_id, std::string initial_dest, std::string final_dest, int time, std::string mode, double budget) {
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
    return new_req.id;
}

bool Ride_Manager::process_nextride_record(double fare, Complete_Ride& outRecord, riderequest& outReq) {
    if (pending_queue.empty()) {
        cout << "No pending ride requests to process." << endl;
        return false;
    }

    outReq = pending_queue.front();
    pending_queue.pop();

    outRecord.id = next_ride_id++;
    outRecord.user_ids.push_back(outReq.user_id);
    outRecord.initial_destination = outReq.initial_destination;
    outRecord.final_destination = outReq.final_destination;
    outRecord.mode = outReq.mode;
    outRecord.fare = fare;

    ride_history.push_back(outRecord);

    RideAction action;
    action.type = COMPLETE;
    action.rec = outRecord;
    undo_stack.push(action);

    cout << "Ride processed successfully! Ride ID: " << outRecord.id <<endl;
    return true;
}

void Ride_Manager::process_nextride(double fare) {
    Complete_Ride rec;
    riderequest req;
    process_nextride_record(fare, rec, req);
}

void Ride_Manager::restore_state(const std::vector<riderequest>& reqs, const std::vector<Complete_Ride>& hist, int maxReqId, int maxRideId) {
    while (!pending_queue.empty()) pending_queue.pop();
    for (const auto& r : reqs) {
        pending_queue.push(r);
    }
    ride_history = hist;
    if (maxReqId >= next_request_id) next_request_id = maxReqId + 1;
    if (maxRideId >= next_ride_id) next_ride_id = maxRideId + 1;
    cout << "[Ride Manager] Restored " << reqs.size() << " pending requests & "
         << hist.size() << " history records from SQLite.\n";
}

bool Ride_Manager::undo_action(RideAction& poppedAction) {
    if (undo_stack.empty()) {
        cout << "no ride to undo." << endl;
        return false;
    }

    poppedAction = undo_stack.top();
    undo_stack.pop();

    if (poppedAction.type == BOOK) {
        int target_id = poppedAction.req.id;
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
    else if (poppedAction.type == COMPLETE) {
        int target_id = poppedAction.rec.id;
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

bool Ride_Manager::undo() {
    RideAction a;
    return undo_action(a);
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

double Ride_Manager::get_total_revenue() const {
    double total = 0.0;
    for (const auto& r : ride_history) total += r.fare;
    return total;
}

bool Ride_Manager::cancel_request(int req_id) {
    std::queue<riderequest> temp;
    bool found = false;
    while (!pending_queue.empty()) {
        riderequest r = pending_queue.front();
        pending_queue.pop();
        if (r.id == req_id) {
            found = true;
        } else {
            temp.push(r);
        }
    }
    pending_queue = temp;
    return found;
}
