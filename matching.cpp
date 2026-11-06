
#include <iostream>
#include <queue>
#include <cmath>
#include "users_management.cpp"

const int MAX_TIME_DIFFERENCE = 30;
const int FARE_PER_STOP = 10;
const int SHARED_CAB_FARE = 80;

class match_class {
public:
    User candidate;
    int sharedSegments;
    int score;
    int timeDifference;
    int estimatedFare;
    string matchType;
};

struct compare {
    bool operator()(match_class a, match_class b) {
        if (a.score != b.score)
            return a.score < b.score;
        return a.timeDifference > b.timeDifference;
    }
};

typedef priority_queue<match_class, vector<match_class>, compare> MatchQueue;

int findStation(string name) {
    for (int i = 0; i < TOTAL_STATIONS; i++)
        if (toLower(stations[i]) == toLower(name))
            return i;
    return -1;
}

int timeDifference(string a, string b) {
    int h1 = stoi(a.substr(0, 2));
    int m1 = stoi(a.substr(3, 2));
    int h2 = stoi(b.substr(0, 2));
    int m2 = stoi(b.substr(3, 2));

    int diff = abs((h1 * 60 + m1) - (h2 * 60 + m2));
    return min(diff, 1440 - diff);
}

bool same(string a, string b) {
    return toLower(a) == toLower(b);
}

bool isPublic(string mode) {
    return mode == "Metro" || mode == "Train" || mode == "Bus";
}

bool evaluateMatch(const User &u, const User &c, match_class &m) {
    int score = 0;
    int shared = 0;
    string type;
    bool sameSrc = same(u.source, c.source);
    bool sameDst = same(u.destination, c.destination);

    
    if (u.transportMode == c.transportMode) {
        if (u.transportMode == "Metro") {
            
            int s1 = findStation(u.source), d1 = findStation(u.destination);
            int s2 = findStation(c.source), d2 = findStation(c.destination);
            if (s1 >= 0 && d1 >= 0 && s2 >= 0 && d2 >= 0) {
                shared = max(0, min(max(s1, d1), max(s2, d2)) - max(min(s1, d1), min(s2, d2)));
                if (shared >= abs(s1 - d1) && shared > 0) {
                    score = 100;
                    type = "Same mode + complete route match";
                }
                else if (shared > 0) {
                    score = 80;
                    type = "Same mode + partial route match";
                }
            }
        }
        else {
           
            bool partialAllowed = (u.transportMode != "Bike" && u.transportMode != "Walking");
            if (sameSrc && sameDst) {
                shared = 1;
                score = 100;
                type = "Same mode + complete route match";
            }
            else if (partialAllowed && (sameSrc || sameDst)) {
                shared = 1;
                score = 80;
                type = "Same mode + partial route match";
            }
        }
    }

   
    if (score == 0 && u.wantsSharedCab && c.wantsSharedCab) {
        if (u.boardingStation != "" && same(u.boardingStation, c.boardingStation)) {
            score = 60;
            type = "Shared cab to common boarding station";
        }
        else if (sameDst) {
            score = 50;
            type = "Shared cab from common arrival station";
        }
    }

    
    if (score == 0 && isPublic(u.transportMode) && isPublic(c.transportMode) && (sameSrc || sameDst)) {
        score = 20;
        type = "Other compatible combination";
    }

    if (score == 0)
        return false;


    bool cab = (score == 60 || score == 50);
    int fare = cab ? SHARED_CAB_FARE : FARE_PER_STOP * max(shared, 1);
    if (fare > u.budget || fare > c.budget)
        return false;

    m.candidate = c;
    m.sharedSegments = shared;
    m.score = score;
    m.estimatedFare = fare;
    m.matchType = type;
    return true;
}

MatchQueue matchRides(User user, vector<User> candidates) {
    MatchQueue pq;

    for (int i = 0; i < (int)candidates.size(); i++) {
        User c = candidates[i];
        if (c.id == user.id || !c.hasRequest)
            continue;

        int diff = timeDifference(user.preferredTime, c.preferredTime);
        if (diff > MAX_TIME_DIFFERENCE)
            continue;

     
        if ((user.genderPreference == "Same" || c.genderPreference == "Same") && !same(user.gender, c.gender))
            continue;

        match_class result;
        if (!evaluateMatch(user, c, result))
            continue;

        result.timeDifference = diff;
        pq.push(result);
    }
    return pq;
}

void displayResults(MatchQueue pq) {
    if (pq.empty()) {
        cout << "\nNo compatible rides found.\n";
        return;
    }

    cout << "\n========== MATCH RESULTS ==========\n";
    int rank = 1;
    while (!pq.empty()) {
        match_class m = pq.top();
        pq.pop();

        cout << "\nMatch Rank: " << rank++;
        cout << "\nCandidate: " << m.candidate.name << " (ID " << m.candidate.id << ")";
        cout << "\nTransport: " << m.candidate.transportMode;
        cout << "\nRoute: " << m.candidate.source << " -> " << m.candidate.destination;
        cout << "\nMatch Type: " << m.matchType;
        cout << "\nShared Segments: " << m.sharedSegments;
        cout << "\nMatch Score: " << m.score << "/100";
        cout << "\nTime Difference: " << m.timeDifference << " minutes";
        cout << "\nEstimated Fare: Rs. " << m.estimatedFare;
        cout << "\nShared Cab: " << ((m.score == 60 || m.score == 50) ? "YES" : "NO") << "\n";
        cout << "-----------------------------------\n";
    }
}

void findMatches(UserManager &manager) {
    int id;
    cout << "\nEnter your User ID: ";
    cin >> id;

    User user;
    if (!manager.getUserById(id, user)) {
        cout << "\nUser not found! Register first.\n";
        return;
    }
    if (!user.hasRequest) {
        cout << "\nYou have no active ride request. Use option 6 first.\n";
        return;
    }

    MatchQueue pq = matchRides(user, manager.getAllUsers());
    displayResults(pq);
}

int main() {
    UserManager manager;
    int choice;

    do {
        cout << "\n\n========================================\n";
        cout << "       PROJECT M.A.R.K.               \n";
        cout << "========================================\n";
        cout << "1. Register User\n";
        cout << "2. Display All Users\n";
        cout << "3. Search User\n";
        cout << "4. Update User Profile\n";
        cout << "5. Delete User\n";
        cout << "6. Request a Ride\n";
        cout << "7. Find Ride Matches\n";
        cout << "0. Exit\n";
        cout << "========================================\n";
        cout << "Enter your choice: ";
        cin >> choice;

        switch (choice) {
            case 1: manager.registerUser(); break;
            case 2: manager.displayUsers(); break;
            case 3: manager.searchUser(); break;
            case 4: manager.updateUser(); break;
            case 5: manager.deleteUser(); break;
            case 6: manager.requestRide(); break;
            case 7: findMatches(manager); break;
        }
    } while (choice != 0);

    return 0;
}