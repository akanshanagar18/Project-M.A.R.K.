#ifndef MATCHING_H
#define MATCHING_H

#include <iostream>
#include <vector>
#include <queue>
#include <cmath>
#include <string>
#include <algorithm>
#include "users_management.cpp"

const int MAX_TIME_DIFFERENCE = 45; // Minutes
const int FARE_PER_STOP = 10;
const int SHARED_CAB_FARE = 80;

class match_class {
public:
    User candidate;
    int sharedSegments = 0;
    int score = 0;
    int timeDifference = 0;
    int estimatedFare = 0;
    string matchType = "";
};

struct compare {
    bool operator()(const match_class& a, const match_class& b) {
        if (a.score != b.score)
            return a.score < b.score; // Max-heap: higher score first
        return a.timeDifference > b.timeDifference; // Min-heap: smaller time difference first
    }
};

typedef priority_queue<match_class, vector<match_class>, compare> MatchQueue;

inline int findStation(const string& name) {
    string sLower = toLower(name);
    for (int i = 0; i < TOTAL_STATIONS; i++) {
        string stLower = toLower(stations[i]);
        if (stLower == sLower || sLower.find(stLower) != string::npos || stLower.find(sLower) != string::npos)
            return i;
    }
    return -1;
}

inline int parseTimeToMinutes(const string& t) {
    string clean = "";
    for (char c : t) if (isdigit(c) || c == ':') clean += c;
    if (clean.empty()) return 0;

    size_t colon = clean.find(':');
    if (colon != string::npos) {
        int h = stoi(clean.substr(0, colon));
        int m = (colon + 1 < clean.length()) ? stoi(clean.substr(colon + 1)) : 0;
        return (h * 60 + m) % 1440;
    } else {
        int val = stoi(clean);
        if (val >= 100) {
            int h = val / 100;
            int m = val % 100;
            return (h * 60 + m) % 1440;
        } else {
            return (val * 60) % 1440;
        }
    }
}

inline int timeDifference(const string& a, const string& b) {
    int m1 = parseTimeToMinutes(a);
    int m2 = parseTimeToMinutes(b);
    int diff = abs(m1 - m2);
    return min(diff, 1440 - diff);
}

inline bool same(const string& a, const string& b) {
    return toLower(a) == toLower(b);
}

inline bool isPublic(const string& mode) {
    return mode == "Metro" || mode == "Train" || mode == "Bus";
}

inline bool evaluateMatch(const User &u, const User &c, match_class &m) {
    int score = 0;
    int shared = 0;
    string type = "";
    bool sameSrc = same(u.source, c.source);
    bool sameDst = same(u.destination, c.destination);

    // 1. MATCHING ON SAME TRANSPORT MODE
    if (toLower(u.transportMode) == toLower(c.transportMode)) {
        if (toLower(u.transportMode) == "metro") {
            int s1 = findStation(u.source), d1 = findStation(u.destination);
            int s2 = findStation(c.source), d2 = findStation(c.destination);

            if (s1 >= 0 && d1 >= 0 && s2 >= 0 && d2 >= 0) {
                bool dir1 = (d1 >= s1);
                bool dir2 = (d2 >= s2);

                // Must be traveling in the same direction on the metro line
                if (dir1 == dir2) {
                    int minStop = max(min(s1, d1), min(s2, d2));
                    int maxStop = min(max(s1, d1), max(s2, d2));
                    shared = max(0, maxStop - minStop);

                    if (shared > 0) {
                        if (sameSrc && sameDst) {
                            score = 100;
                            type = "Same Metro Route (Identical Boarding & Alighting)";
                        } else if (shared >= abs(s1 - d1) || shared >= abs(s2 - d2)) {
                            score = 90;
                            type = "Same Metro Route (Full Sub-corridor match, " + to_string(shared) + " stations)";
                        } else {
                            score = 75;
                            type = "Metro Co-commute (" + to_string(shared) + " overlapping stations)";
                        }
                    }
                }
            } else {
                // Stations outside the metro array: compare string names directly
                if (sameSrc && sameDst) {
                    shared = 1;
                    score = 100;
                    type = "Identical Metro Stations Match";
                } else if (sameSrc || sameDst) {
                    shared = 1;
                    score = 75;
                    type = "Partial Metro Station Overlap";
                }
            }
        } else {
            // Cab, Carpool, Auto, Train, Bus
            bool partialAllowed = (toLower(u.transportMode) != "bike" && toLower(u.transportMode) != "walking");
            if (sameSrc && sameDst) {
                shared = 1;
                score = 100;
                type = "Exact Direct Route Match (" + u.source + " ➔ " + u.destination + ")";
            } else if (partialAllowed && sameSrc) {
                shared = 1;
                score = 85;
                type = "Common Pickup Location (" + u.source + ")";
            } else if (partialAllowed && sameDst) {
                shared = 1;
                score = 85;
                type = "Common Drop-off Destination (" + u.destination + ")";
            }
        }
    }

    // 2. SHARED CAB / CARPOOL TO STATION
    if (score == 0 && u.wantsSharedCab && c.wantsSharedCab) {
        if (!u.boardingStation.empty() && same(u.boardingStation, c.boardingStation)) {
            score = 65;
            shared = 1;
            type = "Shared Cab to Transit Hub (" + u.boardingStation + ")";
        } else if (sameDst) {
            score = 55;
            shared = 1;
            type = "Shared Cab from Transit Destination (" + u.destination + ")";
        } else if (sameSrc) {
            score = 50;
            shared = 1;
            type = "Shared Cab from Common Origin (" + u.source + ")";
        }
    }

    // 3. PUBLIC TRANSIT COMPATIBLE COMBINATIONS (Metro + Bus/Train)
    if (score == 0 && isPublic(u.transportMode) && isPublic(c.transportMode)) {
        if (sameSrc && sameDst) {
            score = 70;
            shared = 1;
            type = "Multi-modal Same Route Commute";
        } else if (sameSrc || sameDst) {
            score = 40;
            shared = 1;
            type = "Compatible Public Transit Segment";
        }
    }

    if (score == 0)
        return false;

    // 4. FARE ESTIMATION & BUDGET COMPATIBILITY
    bool isCabType = (toLower(u.transportMode) == "cab" || toLower(u.transportMode) == "auto" ||
                      toLower(u.transportMode) == "carpool" || score == 65 || score == 55 || score == 50);

    int fare = isCabType ? SHARED_CAB_FARE : FARE_PER_STOP * max(shared, 1);

    // Both users' budgets must cover the estimated fare
    if (fare > u.budget || fare > c.budget)
        return false;

    m.candidate = c;
    m.sharedSegments = shared;
    m.score = score;
    m.estimatedFare = fare;
    m.matchType = type;
    return true;
}

inline MatchQueue matchRides(const User& user, const vector<User>& candidates) {
    MatchQueue pq;

    for (const auto& c : candidates) {
        // Skip self or users without active request
        if (c.id == user.id || !c.hasRequest)
            continue;

        // Skip users that are already matched (unless both explicitly allow multiple)
        if (c.isMatched && (!user.allowMultiple || !c.allowMultiple))
            continue;
        if (user.isMatched && (!user.allowMultiple || !c.allowMultiple))
            continue;

        // Time difference evaluation
        int diff = timeDifference(user.preferredTime, c.preferredTime);
        if (diff > MAX_TIME_DIFFERENCE)
            continue;

        // Gender preference check: if either requests "Same", genders must match
        bool uSame = (toLower(user.genderPreference) == "same" || toLower(user.genderPreference) == "same gender only");
        bool cSame = (toLower(c.genderPreference) == "same" || toLower(c.genderPreference) == "same gender only");
        if ((uSame || cSame) && !same(user.gender, c.gender))
            continue;

        match_class result;
        if (!evaluateMatch(user, c, result))
            continue;

        result.timeDifference = diff;
        pq.push(result);
    }
    return pq;
}

inline void displayResults(MatchQueue pq) {
    if (pq.empty()) {
        cout << "\nNo compatible rides found.\n";
        return;
    }

    cout << "\n========== MATCH RESULTS ==========\n";
    int rank = 1;
    while (!pq.empty()) {
        match_class m = pq.top();
        pq.pop();

        cout << "\nMatch Rank      : " << rank++;
        cout << "\nCandidate       : " << m.candidate.name << " (ID #" << m.candidate.id << ")";
        cout << "\nTransport       : " << m.candidate.transportMode;
        cout << "\nRoute           : " << m.candidate.source << " ➔ " << m.candidate.destination;
        cout << "\nMatch Type      : " << m.matchType;
        cout << "\nCompatibility   : " << m.score << " / 100";
        cout << "\nTime Difference : " << m.timeDifference << " minutes";
        cout << "\nEstimated Fare  : Rs. " << m.estimatedFare;
        cout << "\n-----------------------------------\n";
    }
}

inline void findMatchesCLI(UserManager &manager) {
    int id;
    cout << "\nEnter your User ID: ";
    cin >> id;

    User user;
    if (!manager.getUserById(id, user)) {
        cout << "\nUser not found! Register first.\n";
        return;
    }
    if (!user.hasRequest) {
        cout << "\nYou have no active ride request. Request a ride first.\n";
        return;
    }

    MatchQueue pq = matchRides(user, manager.getAllUsers());
    displayResults(pq);
}

#ifndef NO_STANDALONE_MAIN
int main() {
    UserManager manager;
    int choice;
    do {
        cout << "\n1. Register User\n2. Request Ride\n3. Find Matches\n0. Exit\nEnter: ";
        cin >> choice;
        if (choice == 1) manager.registerUser();
        else if (choice == 2) manager.requestRide();
        else if (choice == 3) findMatchesCLI(manager);
    } while (choice != 0);
    return 0;
}
#endif

#endif

