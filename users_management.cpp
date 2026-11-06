
#include <iostream>
#include <string>
#include <vector>
#include <limits>
#include <algorithm>
using namespace std;

const int TOTAL_STATIONS = 12;
string stations[] = {"Dwarka Sector 21","Dwarka","Janakpuri West","Rajouri Garden","Kirti Nagar","Karol Bagh","Rajiv Chowk","Mandi House","Yamuna Bank","Mayur Vihar-I","Noida Sector 18","Botanical Garden"};

string toLower(string s) {
    transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

class User {
public:
    int id;
    string name, phone, gender;
    string source, destination, preferredTime, transportMode;
    double budget = 0;
    bool hasRequest = false, wantsSharedCab = false, allowMultiple = false;
    string genderPreference, boardingStation;

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
            cout << "Source        : " << source << endl;
            cout << "Destination   : " << destination << endl;
            cout << "Time          : " << preferredTime << endl;
            cout << "Transport     : " << transportMode << endl;
            cout << "Budget        : Rs. " << budget << endl;
            cout << "Gender Preference: " << genderPreference << endl;
            cout << "Shared Cab    : " << (wantsSharedCab ? "Yes" : "No") << endl;
            if (wantsSharedCab)
                cout << "Boarding Station: " << boardingStation << endl;
            cout << "Multiple Sharing: " << (allowMultiple ? "Yes" : "No") << endl;
        }
        else
            cout << "Ride Request  : No active request\n";
        cout << "====================================\n";
    }
};

struct Node {
    User data;
    Node* next;
    Node(User u) {
        data = u;
        next = nullptr;
    }
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
    UserManager() { head = nullptr; }

    vector<User> getAllUsers() {
        vector<User> users;
        for (Node* temp = head; temp != nullptr; temp = temp->next)
            users.push_back(temp->data);
        return users;
    }

    bool getUserById(int id, User &result) {
        Node* n = find(id);
        if (n != nullptr)
            result = n->data;
        return n != nullptr;
    }

    void registerUser() {
        User u;
        cout << "\n========== REGISTER USER ==========\n";
        u.input();

        if (find(u.id) != nullptr) {
            cout << "\nUser ID already exists!\n";
            return;
        }

        Node* newNode = new Node(u);
        if (head == nullptr)
            head = newNode;
        else {
            Node* temp = head;
            while (temp->next != nullptr)
                temp = temp->next;
            temp->next = newNode;
        }
        cout << "\nUser registered successfully!\n";
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
        User &u = n->data;

        cout << "\n========== REQUEST A RIDE ==========\n";
        cout << "\nSelect Transport Mode:\n";
        cout << "1. Metro\n2. Train\n3. Bus\n4. Cab\n5. Auto\n6. Bike\n7. Walking\n";

        int mode;
        cout << "Enter choice: ";
        cin >> mode;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        string modes[] = {"Metro","Train","Bus","Cab","Auto","Bike","Walking"};
        u.transportMode = modes[mode - 1];

        cout << "\nEnter Source: ";
        getline(cin, u.source);
        cout << "Enter Destination: ";
        getline(cin, u.destination);
        cout << "Enter Preferred Time (HH:MM): ";
        getline(cin, u.preferredTime);
        cout << "Enter Maximum Budget: Rs. ";
        cin >> u.budget;

        cout << "\nGender Preference:\n1. Any Gender\n2. Same Gender Only\n";
        int genderChoice;
        cout << "Enter choice: ";
        cin >> genderChoice;
        u.genderPreference = (genderChoice == 1) ? "Any" : "Same";

        cout << "\nDo you want a shared cab to/from a Metro/Train/Bus station?\n";
        cout << "1. Yes\n2. No\n";
        int cabChoice;
        cout << "Enter choice: ";
        cin >> cabChoice;
        u.wantsSharedCab = (cabChoice == 1);

        if (u.wantsSharedCab) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Enter preferred Metro/Train/Bus station: ";
            getline(cin, u.boardingStation);
        }
        else
            u.boardingStation = "";

        cout << "\nAllow sharing with multiple passengers?\n1. Yes\n2. No\n";
        int multipleChoice;
        cout << "Enter choice: ";
        cin >> multipleChoice;
        u.allowMultiple = (multipleChoice == 1);

        u.hasRequest = true;
        cout << "\nRide request saved successfully!\n";
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
        cout << "\nUser found!\n";
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
        cout << "\nEnter updated information:\n";
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Enter Name: ";
        getline(cin, n->data.name);
        cout << "Enter Phone: ";
        getline(cin, n->data.phone);
        cout << "Enter Gender: ";
        getline(cin, n->data.gender);
        cout << "\nUser updated successfully!\n";
    }

    void deleteUser() {
        int id;
        cout << "\nEnter User ID to delete: ";
        cin >> id;

        Node* temp = head;
        Node* previous = nullptr;
        while (temp != nullptr && temp->data.id != id) {
            previous = temp;
            temp = temp->next;
        }
        if (temp == nullptr) {
            cout << "\nUser not found!\n";
            return;
        }
        if (previous == nullptr)
            head = temp->next;
        else
            previous->next = temp->next;
        delete temp;
        cout << "\nUser deleted successfully!\n";
    }

    ~UserManager() {
        while (head != nullptr) {
            Node* temp = head;
            head = head->next;
            delete temp;
        }
    }
};