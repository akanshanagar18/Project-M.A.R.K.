#ifndef USERS_MANAGEMENT_H
#define USERS_MANAGEMENT_H

#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <algorithm>
#include <sstream>
#include <cstdlib>
#include <ctime>

using namespace std;

const int TOTAL_STATIONS = 12;
static string stations[] = {
    "Dwarka Sector 21", "Dwarka", "Janakpuri West", "Rajouri Garden", 
    "Kirti Nagar", "Karol Bagh", "Rajiv Chowk", "Mandi House", 
    "Yamuna Bank", "Mayur Vihar-I", "Noida Sector 18", "Botanical Garden"
};

inline string toLower(string s) {
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

class User {
public:
    int id;
    string name;
    string phone;
    string gender;
    string password = "1234";
    
    // Ride request properties
    string source;
    string destination;
    string preferredTime;
    string transportMode;
    double budget = 0.0;
    
    bool hasRequest = false;
    bool isMatched = false;
    int matchedPartnerId = 0;
    int requestId = 0;
    
    bool wantsSharedCab = false;
    bool allowMultiple = true;
    string genderPreference = "Any";
    string boardingStation = "";

    void input() {
        cout << "\nEnter User ID: ";
        cin >> id;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Enter Name: ";
        getline(cin, name);
        cout << "Enter Phone: ";
        getline(cin, phone);
        cout << "Enter Gender (Male/Female/Other): ";
        getline(cin, gender);
    }

    void display() const {
        cout << "\n====================================\n";
        cout << "User ID       : " << id << endl;
        cout << "Name          : " << name << endl;
        cout << "Phone         : " << phone << endl;
        cout << "Gender        : " << gender << endl;
        if (hasRequest) {
            cout << "Status        : " << (isMatched ? ("Matched with User #" + to_string(matchedPartnerId)) : "Pending Match") << endl;
            cout << "Source        : " << source << endl;
            cout << "Destination   : " << destination << endl;
            cout << "Time          : " << preferredTime << endl;
            cout << "Transport     : " << transportMode << endl;
            cout << "Budget        : Rs. " << budget << endl;
            cout << "Gender Pref   : " << genderPreference << endl;
            cout << "Shared Cab    : " << (wantsSharedCab ? "Yes" : "No") << endl;
            if (wantsSharedCab)
                cout << "Boarding Stn  : " << boardingStation << endl;
            cout << "Multiple Share: " << (allowMultiple ? "Yes" : "No") << endl;
        } else {
            cout << "Ride Request  : No active request\n";
        }
        cout << "====================================\n";
    }
};

struct Node {
    User data;
    Node* next;
    Node(User u) : data(u), next(nullptr) {}
};

class UserManager {
private:
    Node* head;

    Node* find(int id) {
        Node* temp = head;
        while (temp != nullptr && temp->data.id != id)
            temp = temp->next;
        return temp;
    }

public:
    UserManager() : head(nullptr) {
        srand(static_cast<unsigned int>(time(nullptr)));
    }

    bool userExists(int id) const {
        for (Node* temp = head; temp != nullptr; temp = temp->next) {
            if (temp->data.id == id) return true;
        }
        return false;
    }

    // Generate random 4-digit ID between 1000 and 9999, guaranteed unique
    int generateUniqueId() {
        int newId;
        do {
            newId = 1000 + (rand() % 9000);
        } while (userExists(newId));
        return newId;
    }

    void clearAllUsers() {
        while (head != nullptr) {
            Node* temp = head;
            head = head->next;
            delete temp;
        }
    }

    vector<User> getAllUsers() {
        vector<User> users;
        for (Node* temp = head; temp != nullptr; temp = temp->next)
            users.push_back(temp->data);
        return users;
    }

    bool getUserById(int id, User &result) {
        Node* n = find(id);
        if (n != nullptr) {
            result = n->data;
            return true;
        }
        return false;
    }

    bool registerUserDirect(const User& u) {
        if (find(u.id) != nullptr)
            return false;

        Node* newNode = new Node(u);
        if (head == nullptr) {
            head = newNode;
        } else {
            Node* temp = head;
            while (temp->next != nullptr)
                temp = temp->next;
            temp->next = newNode;
        }
        return true;
    }

    void registerUser() {
        User u;
        cout << "\n========== REGISTER USER ==========\n";
        u.id = generateUniqueId();
        cout << "[System] Automatically generated unique 4-digit User ID: " << u.id << endl;
        cout << "Important: Please remember your User ID (" << u.id << ") because it will be needed to access your account and request rides.\n";
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Enter Name: ";
        getline(cin, u.name);
        cout << "Enter Phone: ";
        getline(cin, u.phone);
        cout << "Enter Gender (Male/Female/Other): ";
        getline(cin, u.gender);

        if (registerUserDirect(u))
            cout << "\nUser registered successfully with User ID #" << u.id << "!\n";
        else
            cout << "\nRegistration error.\n";
    }

    bool updateUserDirect(int id, const string& name, const string& phone, const string& gender) {
        Node* n = find(id);
        if (n == nullptr) return false;
        if (!name.empty()) n->data.name = name;
        if (!phone.empty()) n->data.phone = phone;
        if (!gender.empty()) n->data.gender = gender;
        return true;
    }

    bool setRideRequestDirect(int id, const string& src, const string& dst, const string& time, 
                             const string& mode, double budget, const string& genderPref, 
                             bool wantsCab, const string& station, bool allowMultiple) {
        Node* n = find(id);
        if (n == nullptr) return false;
        
        n->data.source = src;
        n->data.destination = dst;
        n->data.preferredTime = time;
        n->data.transportMode = mode;
        n->data.budget = budget;
        n->data.genderPreference = genderPref.empty() ? "Any" : genderPref;
        n->data.wantsSharedCab = wantsCab;
        n->data.boardingStation = station;
        n->data.allowMultiple = allowMultiple;
        n->data.hasRequest = true;
        n->data.isMatched = false;
        n->data.matchedPartnerId = 0;
        return true;
    }

    bool markMatched(int id, int partnerId) {
        Node* n = find(id);
        if (n == nullptr) return false;
        n->data.isMatched = true;
        n->data.matchedPartnerId = partnerId;
        return true;
    }

    bool resetMatch(int id) {
        Node* n = find(id);
        if (n == nullptr) return false;
        n->data.isMatched = false;
        n->data.matchedPartnerId = 0;
        return true;
    }

    bool clearRequest(int id) {
        Node* n = find(id);
        if (n == nullptr) return false;
        n->data.hasRequest = false;
        n->data.isMatched = false;
        n->data.matchedPartnerId = 0;
        return true;
    }

    bool deleteUser(int id) {
        Node* temp = head;
        Node* previous = nullptr;
        while (temp != nullptr && temp->data.id != id) {
            previous = temp;
            temp = temp->next;
        }
        if (temp == nullptr) return false;

        if (previous == nullptr)
            head = temp->next;
        else
            previous->next = temp->next;
        delete temp;
        return true;
    }

    int getUserCount() const {
        int count = 0;
        for (Node* temp = head; temp != nullptr; temp = temp->next)
            count++;
        return count;
    }

    int getActiveRequestCount() const {
        int count = 0;
        for (Node* temp = head; temp != nullptr; temp = temp->next)
            if (temp->data.hasRequest && !temp->data.isMatched) count++;
        return count;
    }

    int getMatchedCount() const {
        int count = 0;
        for (Node* temp = head; temp != nullptr; temp = temp->next)
            if (temp->data.isMatched) count++;
        return count;
    }

    void requestRide() {
        int id;
        cout << "\nEnter your registered User ID: ";
        cin >> id;

        Node* n = find(id);
        if (n == nullptr) {
            cout << "\nUser not found! Register first.\n";
            return;
        }

        if (n->data.hasRequest) {
            cout << "\nUser #" << id << " already has an active ride request in progress.\n";
            cout << "Route: " << n->data.source << " -> " << n->data.destination << " (" << n->data.transportMode << ")\n";
            cout << "M.A.R.K. allows one active ride request per user.\n";
            return;
        }

        cout << "\n========== REQUEST A RIDE ==========\n";
        cout << "1. Metro\n2. Train\n3. Bus\n4. Cab\n5. Auto\n6. Bike\n7. Walking\n";
        int mode;
        cout << "Enter choice: ";
        cin >> mode;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        string modes[] = {"Metro","Train","Bus","Cab","Auto","Bike","Walking"};
        string tMode = (mode >= 1 && mode <= 7) ? modes[mode - 1] : "Cab";

        string src, dst, tPref, station;
        double bg;
        cout << "Enter Source: ";
        getline(cin, src);
        cout << "Enter Destination: ";
        getline(cin, dst);
        cout << "Enter Preferred Time (HH:MM): ";
        getline(cin, tPref);
        cout << "Enter Maximum Budget: Rs. ";
        cin >> bg;

        int gChoice;
        cout << "Gender Preference:\n1. Any Gender\n2. Same Gender Only\nEnter choice: ";
        cin >> gChoice;
        string gPref = (gChoice == 2) ? "Same" : "Any";

        int cabChoice;
        cout << "Want shared cab to station? (1. Yes, 2. No): ";
        cin >> cabChoice;
        bool wantsCab = (cabChoice == 1);
        if (wantsCab) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Enter station name: ";
            getline(cin, station);
        }

        int mChoice;
        cout << "Allow multiple sharing? (1. Yes, 2. No): ";
        cin >> mChoice;
        bool allowMult = (mChoice == 1);

        setRideRequestDirect(id, src, dst, tPref, tMode, bg, gPref, wantsCab, station, allowMult);
        cout << "\nRide request saved successfully in Linked List!\n";
    }

    void displayUsers() {
        if (head == nullptr) {
            cout << "\nNo users registered.\n";
            return;
        }
        cout << "\n========== ALL USERS ==========\n";
        for (Node* temp = head; temp != nullptr; temp = temp->next)
            temp->data.display();
    }

    void searchUser() {
        int id;
        cout << "\nEnter User ID to search: ";
        cin >> id;
        Node* n = find(id);
        if (n == nullptr) {
            cout << "\nUser not found!\n";
            return;
        }
        cout << "\nUser found in Linked List!\n";
        n->data.display();
    }

    void updateUser() {
        int id;
        cout << "\nEnter User ID to update: ";
        cin >> id;
        Node* n = find(id);
        if (n == nullptr) {
            cout << "\nUser not found!\n";
            return;
        }
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        string name, phone, gender;
        cout << "Enter Name: ";
        getline(cin, name);
        cout << "Enter Phone: ";
        getline(cin, phone);
        cout << "Enter Gender: ";
        getline(cin, gender);
        updateUserDirect(id, name, phone, gender);
        cout << "\nUser updated successfully!\n";
    }

    ~UserManager() {
        while (head != nullptr) {
            Node* temp = head;
            head = head->next;
            delete temp;
        }
    }
};

#ifndef NO_STANDALONE_MAIN
int main() {
    UserManager manager;
    int choice;
    do {
        cout << "\n1. Register User\n2. Display All\n3. Search User\n4. Update User\n5. Delete User\n6. Request Ride\n0. Exit\nEnter: ";
        cin >> choice;
        switch (choice) {
            case 1: manager.registerUser(); break;
            case 2: manager.displayUsers(); break;
            case 3: manager.searchUser(); break;
            case 4: manager.updateUser(); break;
            case 5: {
                int id; cout << "Enter ID to delete: "; cin >> id;
                if (manager.deleteUser(id)) cout << "Deleted successfully!\n";
                else cout << "User not found!\n";
                break;
            }
            case 6: manager.requestRide(); break;
        }
    } while (choice != 0);
    return 0;
}
#endif

#endif
