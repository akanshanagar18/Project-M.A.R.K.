#include "ride_manager.h"
#include "route_manager.h"
#include <iostream>
#include <vector>
#include <string>

using namespace std;

int main() {
    Ride_Manager manager;
    RouteManager routes; // Initialize your Route/Graph manager
    int choice;

    do {
        cout << endl;
        cout << "==========================================" << endl;
        cout << "   M.A.R.K MAPPED AUTOMATION ROUTE KINEMATICS" << endl;
        cout << "==========================================" << endl;
        cout << "--- Ride Management ---" << endl;
        cout << "1. Add New Ride Request" << endl;
        cout << "2. Process Next Ride (Matching Engine)" << endl;
        cout << "3. Undo Last Action (Stack)" << endl;
        cout << "4. View Pending Queue" << endl;
        cout << "5. View Ride History" << endl;
        cout << "6. View Statistics" << endl;
        cout << endl;
        cout << "--- Route & Graph Navigation ---" << endl;
        cout << "7. Add New Road / Route" << endl;
        cout << "8. View Transport Network Map" << endl;
        cout << "9. Find Shortest Path (Dijkstra's Algorithm)" << endl;
        cout << "10. Find Minimum Connections (BFS Path)" << endl;
        cout << endl;
        cout << "11. Exit" << endl;
        cout << "Enter your choice (1-11): ";
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
                double fare;
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
            case 7: {
                string u, v;
                double distance;
                cout << "Enter Starting Location: ";
                cin >> u;
                cout << "Enter Destination Location: ";
                cin >> v;
                cout << "Enter Road Distance (km): ";
                cin >> distance;

                routes.addNewRoute(u, v, distance);
                cout << "Route successfully added between " << u << " and " << v << "!" << endl;
                break;
            }
            case 8:
                routes.showNetwork();
                break;
            case 9: {
                string start, end;
                cout << "Enter Start Location: ";
                cin >> start;
                cout << "Enter End Location: ";
                cin >> end;

                auto result = routes.getShortestPath(start, end);
                if (result.second == -1.0) {
                    cout << "No path found between " << start << " and " << end << "!" << endl;
                } else {
                    cout << "\n--- SHORTEST PATH (Dijkstra) ---" << endl;
                    cout << "Total Distance: " << result.second << " km" << endl;
                    cout << "Route Path: ";
                    for (size_t i = 0; i < result.first.size(); ++i) {
                        cout << result.first[i];
                        if (i + 1 < result.first.size()) cout << " -> ";
                    }
                    cout << endl;
                }
                break;
            }
            case 10: {
                string start, end;
                cout << "Enter Start Location: ";
                cin >> start;
                cout << "Enter End Location: ";
                cin >> end;

                vector<string> path = routes.getBFSPath(start, end);
                if (path.empty()) {
                    cout << "No connection path found between " << start << " and " << end << "!" << endl;
                } else {
                    cout << "\n--- MINIMUM CONNECTIONS PATH (BFS) ---" << endl;
                    cout << "Path (Fewest Hops): ";
                    for (size_t i = 0; i < path.size(); ++i) {
                        cout << path[i];
                        if (i + 1 < path.size()) cout << " -> ";
                    }
                    cout << endl;
                }
                break;
            }
            case 11:
                cout << "Exiting Project M.A.R.K. Goodbye!" << endl;
                break;
            default:
                cout << "Invalid choice! Please choose between 1 and 11." << endl;
        }
    } while (choice != 11);

    return 0;
}

