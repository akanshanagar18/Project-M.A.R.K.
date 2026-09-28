#include <iostream>
#include <string>
using namespace std;

class User {
public:
    int id;
    string name;
    string phone;
    string source;
    string destination;
    string preferredTime;
    string transportMode;
    double budget;
    void input() {
        cout << "\nEnter User ID: ";
        cin >> id;
        cin.ignore();
        cout << "Enter Name: ";
        getline(cin, name);
        cout << "Enter Phone: ";
        getline(cin, phone);
        cout << "Enter Source: ";
        getline(cin, source);
        cout << "Enter Destination: ";
        getline(cin, destination);
        cout << "Enter Preferred Time: ";
        getline(cin, preferredTime);
        cout << "Enter Transport Mode: ";
        getline(cin, transportMode);
        cout << "Enter Budget: ";
        cin >> budget;
    }
    void display() const {
        cout << "\n====================================\n";
        cout << "User ID        : " << id << endl;
        cout << "Name           : " << name << endl;
        cout << "Phone          : " << phone << endl;
        cout << "Source         : " << source << endl;
        cout << "Destination    : " << destination << endl;
        cout << "Preferred Time : " << preferredTime << endl;
        cout << "Transport      : " << transportMode << endl;
        cout << "Budget         : Rs. " << budget << endl;
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
