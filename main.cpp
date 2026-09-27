#include "ride_manager.h"
#include <iostream>

using namespace std;


// JUST IN CASE COMPLETE FRONTEND IS NOT ALLOWED, JUST AS A BACKUP SO THTA IT CAN RUN IN TERMINAL(DEMOOOOOO CODE)
//IT DOSENT WORK RIGHT NOW, BCZ NO NEED IG


int main() {
    Ride_Manager manager;
    int choice;

    do {
        cout<< endl;
        cout << "M.A.R.K MAPPED AUTOMATION ROUTE KINEMATICS " << endl;
        cout << endl;
        cout << "1. Add New Ride Request" << endl;
        cout << "2. Process Next Ride (Matching Engine)" << endl;
        cout << "3. Undo Last Action (Stack)" << endl;
        cout << "4. View Pending Queue" << endl;
        cout << "5. View Ride History" << endl;
        cout << "6. View Statistics" << endl;
        cout << "7. Exit" << endl;
        cout << "Enter your choice (1-7): ";
        cin >> choice;

        switch (choice) {
            case 1: {
                int user_id, time;
                string initial_dest, final_dest, mode;
                double budget;

                cout << "Enter User ID: ";
                cin >> user_id;
                cout << "Enter Initial Destination: ";
                cin >> initial_dest;
                cout << "Enter Final Destination: ";
                cin >> final_dest;
                cout << "Enter Time (e.g. 930): ";
                cin >> time;
                cout << "Enter Mode (cab/metro/carpool): ";
                cin >> mode;
                cout << "Enter Budget: ";
                cin >> budget;

                manager.add_ride_req(user_id, initial_dest, final_dest, time, mode, budget);
                break;
            }
            case 2: {
                int partner_id;
                double fare;
                cout << "Enter Partner/Driver ID: ";
                cin >> partner_id;
                cout << "Enter Final Fare Charged: ";
                cin >> fare;

                manager.process_nextride(fare);
                break;
            }
            case 3:
                manager.undo();
                break;
            case 4:
                manager.displaypending_queue();
                break;
            case 5:
                manager.display_ride_history();
                break;
            case 6:
                manager.displaystats();
                break;
            case 7:
                cout << "Exiting Route Mate. Goodbye!" << endl;
                break;
            default:
                cout << "Invalid choice! Please choose between 1 and 7." << endl;
        }
    } while (choice != 7);

    return 0;
}