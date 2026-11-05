
#include <iostream>
#include <vector>
#include <queue>
#include <string>
#include <cmath>
#include <algorithm>

using namespace std;

const int MAX_TIME_DIFFERENCE = 30;

string stations[] = {"Dwarka Sector 21","Dwarka","Janakpuri West","Rajouri Garden","Kirti Nagar","Karol Bagh","Rajiv Chowk","Mandi House","Yamuna Bank","Mayur Vihar-I","Noida Sector 18","Botanical Garden"};

const int TOTAL_STATIONS = 12;

class RideRequest
{
public:
    int id;
    string name;
    int source;
    int destination;
    string time;

    vector<int> route;
};

class match_class
{
public:
    RideRequest candidate;
    int sharedSegments;
    int score;
    int timeDifference;
    bool sharedCab;
};

struct compare
{
    bool operator()(match_class a, match_class b)
    {
        if (a.score != b.score)
            return a.score < b.score;
        return a.timeDifference > b.timeDifference;
    }
};

vector<int> generateRoute(int source, int destination)
{
    vector<int> route;
    int step = (source < destination) ? 1 : -1;
    for (int i = source; ; i += step)
    {
        route.push_back(i);
        if (i == destination)
            break;
    }
    return route;
}

int findSharedSegments(vector<int> a, vector<int> b)
{
    int maximum = 0;
    for (int i = 0; i < (int)a.size(); i++)
    {
        for (int j = 0; j < (int)b.size(); j++)
        {
            int count = 0;
            while (i + count < (int)a.size() &&
                   j + count < (int)b.size() &&
                   a[i + count] == b[j + count])
            {
                count++;
            }
            maximum = max(maximum, count);
            count = 0;

            while (i + count < (int)a.size() &&
                   j - count >= 0 &&
                   a[i + count] == b[j - count])
            {
                count++;
            }

            maximum = max(maximum, count);
        }
    }
    return maximum - 1;
}

int timeDifference(string a, string b)
{
    int h1, m1, h2, m2;
    char c1, c2;

    string t1 = a, t2 = b;
    h1 = stoi(t1.substr(0, 2));
    m1 = stoi(t1.substr(3, 2));

    h2 = stoi(t2.substr(0, 2));
    m2 = stoi(t2.substr(3, 2));

    int first = h1 * 60 + m1;
    int second = h2 * 60 + m2;

    int diff = abs(first - second);

    return min(diff, 1440 - diff);
}

void inputRequest(RideRequest &r)
{
    cout << "\nEnter ID: ";
    cin >> r.id;

    cout << "Enter name: ";
    cin >> r.name;

    cout << "\nAvailable Stations:\n";

    for (int i = 0; i < TOTAL_STATIONS; i++)
    {
        cout << i + 1 << ". " << stations[i] << "\n";
    }

    cout << "\nEnter source station number: ";
    cin >> r.source;

    cout << "Enter destination station number: ";
    cin >> r.destination;

    cout << "Enter departure time (HH:MM): ";
    cin >> r.time;
    r.source--;
    r.destination--;

    r.route = generateRoute(r.source, r.destination);
}



priority_queue<match_class, vector<match_class>, compare>
matchRides(RideRequest user, vector<RideRequest> candidates)
{
    priority_queue<match_class, vector<match_class>, compare> pq;

    for (int i = 0; i < (int)candidates.size(); i++)
    {
        RideRequest candidate = candidates[i];
        if (candidate.id == user.id)
            continue;
        int diff = timeDifference(user.time, candidate.time);

        if (diff > MAX_TIME_DIFFERENCE)
            continue;
        int shared = findSharedSegments(
            user.route,
            candidate.route
        );
        if (shared <= 0)
            continue;

        int totalSegments = (int)user.route.size() - 1;

        double percentage =
            (shared * 100.0) / totalSegments;

        int score;

        if (percentage >= 100)
            score = 100;
        else if (percentage >= 50)
            score = 60;
        else
            score = 40;

        match_class result;

        result.candidate = candidate;
        result.sharedSegments = shared;
        result.score = score;
        result.timeDifference = diff;
        result.sharedCab = true;
        pq.push(result);
    }

    return pq;
}
void displayResults(
    priority_queue<match_class, vector<match_class>, compare> pq)
{
    if (pq.empty())
    {
        cout << "\nNo compatible rides found.\n";
        return;
    }
    cout << "\n========== MATCH RESULTS ==========\n";
    int rank = 1;
    while (!pq.empty())
    {
        match_class m = pq.top();
        pq.pop();
        cout << "\nMatch Rank: " << rank++;
        cout << "\nCandidate: " << m.candidate.name;
        cout << "\nShared Route: ";
        for (int i = 0; i < m.candidate.route.size(); i++)
        {
            cout << stations[m.candidate.route[i]];

            if (i + 1 < m.candidate.route.size())
                cout << " -> ";
        }
        cout << "\nShared Segments: " << m.sharedSegments;
        cout << "\nMatch Score: " << m.score << "/100";
        cout << "\nTime Difference: "
             << m.timeDifference << " minutes";
        cout << "\nShared Cab: YES\n";
        cout << "-----------------------------------\n";
    }
}

int main()
{
    cout << "================================\n";
    cout << "       PROJECT M.A.R.K.\n";
    cout << "       RIDE MATCHING\n";
    cout << "================================\n";
    RideRequest user;
    cout << "\nENTER YOUR DETAILS\n";
    inputRequest(user);
    if (user.source < 0 || user.source >= TOTAL_STATIONS ||
        user.destination < 0 || user.destination >= TOTAL_STATIONS ||
        user.source == user.destination)
    {
        cout << "\nInvalid station selection.\n";
        return 0;
    }
    int n;
    cout << "\nEnter number of candidates: ";
    cin >> n;
    if (n < 0)
    {
        cout << "Invalid number of candidates.\n";
        return 0;
    }
    vector<RideRequest> candidates;
    for (int i = 0; i < n; i++)
    {
        RideRequest r;
        cout << "\nENTER CANDIDATE " << i + 1 << " DETAILS\n";
        inputRequest(r);
        if (r.source < 0 || r.source >= TOTAL_STATIONS ||
            r.destination < 0 || r.destination >= TOTAL_STATIONS ||
            r.source == r.destination)
        {
            cout << "Invalid request. Candidate skipped.\n";
            continue;
        }

        candidates.push_back(r);
    }
    auto pq = matchRides(user, candidates);
    displayResults(pq);
    return 0;
}
