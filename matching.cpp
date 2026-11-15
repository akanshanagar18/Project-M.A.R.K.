#ifndef MATCHING_H
#define MATCHING_H

#include <iostream>
#include <vector>
#include <queue>
#include <cmath>
#include <string>
#include <algorithm>
#include "users_management.cpp"

using namespace std;

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

inline string cleanLocationName(const string& str) {
    string s = toLower(str);
    string wordsToRemove[] = {"metro", "station", "stn", ",", ".", "-", "(", ")", "noida", "delhi"};
    for (const string& w : wordsToRemove) {
        size_t pos = 0;
        while ((pos = s.find(w, pos)) != string::npos) {
            s.replace(pos, w.length(), " ");
            pos += 1;
        }
    }
    string res = "";
    bool lastSpace = true;
    for (char c : s) {
        if (isalnum(c)) {
            res += c;
            lastSpace = false;
        } else if (!lastSpace) {
            res += ' ';
            lastSpace = true;
        }
    }
    while (!res.empty() && res.back() == ' ') res.pop_back();
    return res;
}

inline bool same(const string& a, const string& b) {
    string la = toLower(a);
    string lb = toLower(b);
    if (la == lb) return true;
    if (la.empty() || lb.empty()) return false;
    if (la.find(lb) != string::npos || lb.find(la) != string::npos) return true;

    string ca = cleanLocationName(a);
    string cb = cleanLocationName(b);
    if (!ca.empty() && !cb.empty()) {
        if (ca == cb) return true;
        if (ca.find(cb) != string::npos || cb.find(ca) != string::npos) return true;
    }

    if (la.find("sector 62") != string::npos && lb.find("sector 62") != string::npos) return true;
    if (la.find("sector 128") != string::npos && lb.find("sector 128") != string::npos) return true;
    if ((la.find("jiit") != string::npos || la.find("jaypee") != string::npos) &&
        (lb.find("jiit") != string::npos || lb.find("jaypee") != string::npos)) {
        if (la.find("62") != string::npos && lb.find("62") != string::npos) return true;
        if (la.find("128") != string::npos && lb.find("128") != string::npos) return true;
    }
    if (la.find("okhla bird") != string::npos && lb.find("okhla bird") != string::npos) return true;
    if (la.find("kalindi kunj") != string::npos && lb.find("kalindi kunj") != string::npos) return true;
    if (la.find("sector 18") != string::npos && lb.find("sector 18") != string::npos) return true;
    if (la.find("botanical garden") != string::npos && lb.find("botanical garden") != string::npos) return true;
    if (la.find("connaught place") != string::npos && lb.find("connaught place") != string::npos) return true;
    if (la.find("rajiv chowk") != string::npos && lb.find("rajiv chowk") != string::npos) return true;
    if (la.find("dwarka") != string::npos && lb.find("dwarka") != string::npos) return true;

    return false;
}

inline int findStation(const string& name) {
    string sLower = toLower(name);
    for (int i = 0; i < TOTAL_STATIONS; i++) {
        string stLower = toLower(stations[i]);
        if (stLower == sLower || sLower.find(stLower) != string::npos || stLower.find(sLower) != string::npos)
            return i;
        if (same(name, stations[i]))
            return i;
    }
    return -1;
}

inline int parseTimeToMinutes(const string& t) {
    string tLower = toLower(t);
    bool isPM = (tLower.find("pm") != string::npos);
    bool isAM = (tLower.find("am") != string::npos);

    string clean = "";
    for (char c : t) if (isdigit(c) || c == ':') clean += c;
    if (clean.empty()) return 540; // Default 09:00 AM
    
    int h = 9, m = 0;
    size_t colon = clean.find(':');
    if (colon != string::npos) {
        try {
            h = stoi(clean.substr(0, colon));
            if (colon + 1 < clean.length()) m = stoi(clean.substr(colon + 1));
        } catch (...) {}
    } else {
        try {
            int val = stoi(clean);
            if (val >= 100) {
                h = val / 100;
                m = val % 100;
            } else {
                h = val;
                m = 0;
            }
        } catch (...) {}
    }

    if (isPM && h < 12) h += 12;
    if (isAM && h == 12) h = 0;

    return (h * 60 + m) % 1440;
}

inline int timeDifference(const string& a, const string& b) {
    int m1 = parseTimeToMinutes(a);
    int m2 = parseTimeToMinutes(b);
    int diff = abs(m1 - m2);
    return min(diff, 1440 - diff);
}

inline bool isPublic(const string& mode) {
    string m = toLower(mode);
    return m.find("metro") != string::npos || m.find("train") != string::npos || m.find("bus") != string::npos;
}

inline bool evaluateMatch(const User &u, const User &c, match_class &m) {
    int score = 0;
    int shared = 0;
    string type = "";
    bool sameSrc = same(u.source, c.source);
    bool sameDst = same(u.destination, c.destination);

    string uMode = toLower(u.transportMode);
    string cMode = toLower(c.transportMode);

    bool uAny = (uMode == "all" || uMode.find("all") != string::npos || uMode.find("any") != string::npos);
    bool cAny = (cMode == "all" || cMode.find("all") != string::npos || cMode.find("any") != string::npos);

    bool uERick = (uMode.find("e-rick") != string::npos || uMode.find("erick") != string::npos);
    bool cERick = (cMode.find("e-rick") != string::npos || cMode.find("erick") != string::npos);

    bool uAuto = (!uERick && (uMode.find("auto") != string::npos || uMode.find("rickshaw") != string::npos));
    bool cAuto = (!cERick && (cMode.find("auto") != string::npos || cMode.find("rickshaw") != string::npos));

    bool uCab = (uMode.find("cab") != string::npos || uMode.find("taxi") != string::npos || uMode.find("carpool") != string::npos || uMode.find("van") != string::npos);
    bool cCab = (cMode.find("cab") != string::npos || cMode.find("taxi") != string::npos || cMode.find("carpool") != string::npos || cMode.find("van") != string::npos);

    bool uMetro = (uMode.find("metro") != string::npos || uMode.find("train") != string::npos || uMode.find("transit") != string::npos);
    bool cMetro = (cMode.find("metro") != string::npos || cMode.find("train") != string::npos || cMode.find("transit") != string::npos);

    bool sameMode = (uMode == cMode) ||
                    (uCab && cCab) ||
                    (uAuto && cAuto) ||
                    (uERick && cERick) ||
                    ((uAuto || uERick) && (cAuto || cERick)) ||
                    (uMetro && cMetro) ||
                    uAny || cAny;

    // 1. MATCHING ON SAME OR COMPATIBLE TRANSPORT MODE
    if (sameMode) {
        if (uMetro && cMetro && !uAny && !cAny) {
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
            // Cab, Carpool, Auto, E-Rickshaw, All Preferred
            bool partialAllowed = (uMode != "bike" && uMode != "walking");
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
    int fare = SHARED_CAB_FARE;
    if (uERick || cERick) {
        fare = 30;
    } else if (uAuto || cAuto) {
        fare = 50;
    } else if (uMetro && cMetro) {
        fare = FARE_PER_STOP * max(shared, 1);
    } else if (uCab || cCab) {
        fare = SHARED_CAB_FARE;
    } else {
        fare = 50;
    }
    
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

inline bool isSameGender(const string& g1, const string& g2) {
    string a = toLower(g1);
    string b = toLower(g2);
    while (!a.empty() && (a.back() == ' ' || a.back() == '\t' || a.back() == '\r' || a.back() == '\n')) a.pop_back();
    while (!b.empty() && (b.back() == ' ' || b.back() == '\t' || b.back() == '\r' || b.back() == '\n')) b.pop_back();
    while (!a.empty() && (a.front() == ' ' || a.front() == '\t')) a.erase(a.begin());
    while (!b.empty() && (b.front() == ' ' || b.front() == '\t')) b.erase(b.begin());

    if (a.empty() || b.empty()) return false;
    return a == b;
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

        // Gender preference check: if either requests "Same", genders must match exactly
        bool uSame = (toLower(user.genderPreference) == "same" || toLower(user.genderPreference).find("same") != string::npos);
        bool cSame = (toLower(c.genderPreference) == "same" || toLower(c.genderPreference).find("same") != string::npos);
        if ((uSame || cSame) && !isSameGender(user.gender, c.gender))
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
