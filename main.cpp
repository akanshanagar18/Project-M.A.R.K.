#define NO_STANDALONE_MAIN

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <iomanip>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "users_management.cpp"
#include "matching.cpp"
#include "ride_manager.h"
#include "route_manager.h"

using namespace std;

// Global Data Structures
UserManager userManager;
Ride_Manager rideManager;
RouteManager routeManager;

// Helper: URL decoding
string urlDecode(const string& src) {
    string ret;
    char ch;
    int i, ii;
    for (i = 0; i < (int)src.length(); i++) {
        if (src[i] == '%') {
            if (i + 2 < (int)src.length()) {
                sscanf(src.substr(i + 1, 2).c_str(), "%x", &ii);
                ch = static_cast<char>(ii);
                ret += ch;
                i = i + 2;
            }
        } else if (src[i] == '+') {
            ret += ' ';
        } else {
            ret += src[i];
        }
    }
    return ret;
}

// Helper: Extract query parameter
string getQueryParam(const string& url, const string& param) {
    size_t qPos = url.find('?');
    if (qPos == string::npos) return "";
    string query = url.substr(qPos + 1);
    size_t pPos = query.find(param + "=");
    if (pPos == string::npos) return "";
    size_t start = pPos + param.length() + 1;
    size_t end = query.find('&', start);
    string val = (end == string::npos) ? query.substr(start) : query.substr(start, end - start);
    return urlDecode(val);
}

// Helper: Simple JSON string extraction
string getJsonString(const string& json, const string& key) {
    string pattern = "\"" + key + "\"";
    size_t p = json.find(pattern);
    if (p == string::npos) return "";
    size_t colon = json.find(':', p + pattern.length());
    if (colon == string::npos) return "";
    size_t startQuote = json.find('"', colon + 1);
    if (startQuote == string::npos) return "";
    size_t endQuote = json.find('"', startQuote + 1);
    if (endQuote == string::npos) return "";
    return json.substr(startQuote + 1, endQuote - startQuote - 1);
}

double getJsonDouble(const string& json, const string& key, double defaultVal = 0.0) {
    string pattern = "\"" + key + "\"";
    size_t p = json.find(pattern);
    if (p == string::npos) return defaultVal;
    size_t colon = json.find(':', p + pattern.length());
    if (colon == string::npos) return defaultVal;
    size_t start = json.find_first_of("0123456789.-", colon + 1);
    if (start == string::npos) return defaultVal;
    size_t end = json.find_first_of(",}\" \r\n", start);
    string sVal = (end == string::npos) ? json.substr(start) : json.substr(start, end - start);
    try {
        return stod(sVal);
    } catch (...) {
        return defaultVal;
    }
}

int getJsonInt(const string& json, const string& key, int defaultVal = 0) {
    return (int)getJsonDouble(json, key, defaultVal);
}

bool getJsonBool(const string& json, const string& key, bool defaultVal = false) {
    string pattern = "\"" + key + "\"";
    size_t p = json.find(pattern);
    if (p == string::npos) return defaultVal;
    size_t colon = json.find(':', p + pattern.length());
    if (colon == string::npos) return defaultVal;
    size_t valPos = json.find_first_not_of(" \t\r\n", colon + 1);
    if (valPos == string::npos) return defaultVal;
    if (json.substr(valPos, 4) == "true" || json[valPos] == '1') return true;
    if (json.substr(valPos, 5) == "false" || json[valPos] == '0') return false;
    return defaultVal;
}

string escapeJson(const string& s) {
    ostringstream o;
    for (char c : s) {
        if (c == '"') o << "\\\"";
        else if (c == '\\') o << "\\\\";
        else if (c == '\b') o << "\\b";
        else if (c == '\f') o << "\\f";
        else if (c == '\n') o << "\\n";
        else if (c == '\r') o << "\\r";
        else if (c == '\t') o << "\\t";
        else o << c;
    }
    return o.str();
}

const string USERS_FILE = "mark_users.json";
const string RIDES_FILE = "mark_rides.json";

// Convert User object to JSON
string userToJson(const User& u) {
    ostringstream ss;
    ss << "{"
       << "\"id\":" << u.id << ","
       << "\"name\":\"" << escapeJson(u.name) << "\","
       << "\"phone\":\"" << escapeJson(u.phone) << "\","
       << "\"gender\":\"" << escapeJson(u.gender) << "\","
       << "\"password\":\"" << escapeJson(u.password) << "\","
       << "\"hasRequest\":" << (u.hasRequest ? "true" : "false") << ","
       << "\"isMatched\":" << (u.isMatched ? "true" : "false") << ","
       << "\"matchedPartnerId\":" << u.matchedPartnerId << ","
       << "\"source\":\"" << escapeJson(u.source) << "\","
       << "\"destination\":\"" << escapeJson(u.destination) << "\","
       << "\"preferredTime\":\"" << escapeJson(u.preferredTime) << "\","
       << "\"transportMode\":\"" << escapeJson(u.transportMode) << "\","
       << "\"budget\":" << u.budget << ","
       << "\"genderPreference\":\"" << escapeJson(u.genderPreference) << "\","
       << "\"wantsSharedCab\":" << (u.wantsSharedCab ? "true" : "false") << ","
       << "\"boardingStation\":\"" << escapeJson(u.boardingStation) << "\","
       << "\"allowMultiple\":" << (u.allowMultiple ? "true" : "false")
       << "}";
    return ss.str();
}

User parseUserFromJson(const string& objStr) {
    User u;
    u.id = getJsonInt(objStr, "id");
    u.name = getJsonString(objStr, "name");
    u.phone = getJsonString(objStr, "phone");
    u.gender = getJsonString(objStr, "gender");
    u.password = getJsonString(objStr, "password");
    if (u.password.empty()) u.password = "1234";
    u.hasRequest = getJsonBool(objStr, "hasRequest", false);
    u.isMatched = getJsonBool(objStr, "isMatched", false);
    u.matchedPartnerId = getJsonInt(objStr, "matchedPartnerId", 0);
    u.source = getJsonString(objStr, "source");
    u.destination = getJsonString(objStr, "destination");
    u.preferredTime = getJsonString(objStr, "preferredTime");
    u.transportMode = getJsonString(objStr, "transportMode");
    u.budget = getJsonDouble(objStr, "budget", 0.0);
    u.genderPreference = getJsonString(objStr, "genderPreference");
    if (u.genderPreference.empty()) u.genderPreference = "Any";
    u.wantsSharedCab = getJsonBool(objStr, "wantsSharedCab", false);
    u.boardingStation = getJsonString(objStr, "boardingStation");
    u.allowMultiple = getJsonBool(objStr, "allowMultiple", true);
    return u;
}

vector<User> loadUsersFromFile(const string& filename) {
    vector<User> users;
    ifstream inFile(filename);
    if (!inFile.is_open()) return users;
    
    string content((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
    inFile.close();

    size_t pos = 0;
    while (pos < content.length()) {
        size_t start = content.find('{', pos);
        if (start == string::npos) break;
        
        size_t end = string::npos;
        int depth = 0;
        bool inQuotes = false;
        for (size_t i = start; i < content.length(); ++i) {
            char c = content[i];
            if (c == '"' && (i == 0 || content[i - 1] != '\\')) {
                inQuotes = !inQuotes;
            } else if (!inQuotes) {
                if (c == '{') depth++;
                else if (c == '}') {
                    depth--;
                    if (depth == 0) {
                        end = i;
                        break;
                    }
                }
            }
        }
        
        if (end == string::npos) break;
        string objStr = content.substr(start, end - start + 1);
        User u = parseUserFromJson(objStr);
        if (u.id > 0 && !u.name.empty()) {
            users.push_back(u);
        }
        pos = end + 1;
    }
    return users;
}

void saveUsersToFile(const string& filename, const vector<User>& users) {
    ofstream outFile(filename);
    if (!outFile.is_open()) {
        cerr << "[Storage Error] Could not open " << filename << " for writing.\n";
        return;
    }
    outFile << "[\n";
    for (size_t i = 0; i < users.size(); ++i) {
        outFile << "  " << userToJson(users[i]);
        if (i + 1 < users.size()) outFile << ",";
        outFile << "\n";
    }
    outFile << "]\n";
    outFile.close();
}

void loadRidesFromFile(const string& filename, vector<riderequest>& outPending, vector<Complete_Ride>& outHistory) {
    ifstream inFile(filename);
    if (!inFile.is_open()) return;
    string content((istreambuf_iterator<char>(inFile)), istreambuf_iterator<char>());
    inFile.close();

    size_t pendingPos = content.find("\"pending\"");
    size_t historyPos = content.find("\"history\"");
    
    if (pendingPos != string::npos) {
        size_t arrStart = content.find('[', pendingPos);
        size_t arrEnd = (historyPos != string::npos) ? historyPos : content.length();
        if (arrStart != string::npos && arrStart < arrEnd) {
            size_t pos = arrStart;
            while (pos < arrEnd) {
                size_t oStart = content.find('{', pos);
                if (oStart == string::npos || oStart >= arrEnd) break;
                size_t oEnd = content.find('}', oStart);
                if (oEnd == string::npos || oEnd >= arrEnd) break;
                string obj = content.substr(oStart, oEnd - oStart + 1);
                
                riderequest r;
                r.id = getJsonInt(obj, "id");
                r.user_id = getJsonInt(obj, "user_id");
                r.initial_destination = getJsonString(obj, "initial_dest");
                r.final_destination = getJsonString(obj, "final_dest");
                r.time = getJsonInt(obj, "time", 900);
                r.mode = getJsonString(obj, "mode");
                if (r.mode.empty()) r.mode = "Cab";
                r.budget = getJsonDouble(obj, "budget", 150.0);
                
                if (r.id > 0 && r.user_id > 0) {
                    outPending.push_back(r);
                }
                pos = oEnd + 1;
            }
        }
    }

    if (historyPos != string::npos) {
        size_t arrStart = content.find('[', historyPos);
        if (arrStart != string::npos) {
            size_t pos = arrStart;
            while (pos < content.length()) {
                size_t oStart = content.find('{', pos);
                if (oStart == string::npos) break;
                size_t oEnd = content.find('}', oStart);
                if (oEnd == string::npos) break;
                string obj = content.substr(oStart, oEnd - oStart + 1);

                Complete_Ride cr;
                cr.id = getJsonInt(obj, "id");
                cr.initial_destination = getJsonString(obj, "initial_dest");
                cr.final_destination = getJsonString(obj, "final_dest");
                cr.mode = getJsonString(obj, "mode");
                cr.fare = getJsonDouble(obj, "fare", 0.0);
                
                size_t uidsPos = obj.find("\"user_ids\"");
                if (uidsPos != string::npos) {
                    size_t uStart = obj.find('[', uidsPos);
                    size_t uEnd = obj.find(']', uStart);
                    if (uStart != string::npos && uEnd != string::npos) {
                        string uidsStr = obj.substr(uStart + 1, uEnd - uStart - 1);
                        stringstream ss(uidsStr);
                        string item;
                        while (getline(ss, item, ',')) {
                            try {
                                int uid = stoi(item);
                                if (uid > 0) cr.user_ids.push_back(uid);
                            } catch (...) {}
                        }
                    }
                }
                if (cr.user_ids.empty()) {
                    int singleUid = getJsonInt(obj, "user_id");
                    if (singleUid > 0) cr.user_ids.push_back(singleUid);
                }
                if (cr.id > 0) {
                    outHistory.push_back(cr);
                }
                pos = oEnd + 1;
            }
        }
    }
}

void saveRidesToFile(const string& filename, const vector<riderequest>& pending, const vector<Complete_Ride>& history) {
    ofstream outFile(filename);
    if (!outFile.is_open()) return;
    
    outFile << "{\n  \"pending\": [\n";
    for (size_t i = 0; i < pending.size(); ++i) {
        outFile << "    {"
                << "\"id\":" << pending[i].id << ","
                << "\"user_id\":" << pending[i].user_id << ","
                << "\"initial_dest\":\"" << escapeJson(pending[i].initial_destination) << "\","
                << "\"final_dest\":\"" << escapeJson(pending[i].final_destination) << "\","
                << "\"time\":" << pending[i].time << ","
                << "\"mode\":\"" << escapeJson(pending[i].mode) << "\","
                << "\"budget\":" << pending[i].budget
                << "}";
        if (i + 1 < pending.size()) outFile << ",";
        outFile << "\n";
    }
    outFile << "  ],\n  \"history\": [\n";
    for (size_t i = 0; i < history.size(); ++i) {
        outFile << "    {"
                << "\"id\":" << history[i].id << ","
                << "\"user_ids\":[";
        for (size_t j = 0; j < history[i].user_ids.size(); ++j) {
            outFile << history[i].user_ids[j];
            if (j + 1 < history[i].user_ids.size()) outFile << ",";
        }
        outFile << "],"
                << "\"initial_dest\":\"" << escapeJson(history[i].initial_destination) << "\","
                << "\"final_dest\":\"" << escapeJson(history[i].final_destination) << "\","
                << "\"mode\":\"" << escapeJson(history[i].mode) << "\","
                << "\"fare\":" << history[i].fare
                << "}";
        if (i + 1 < history.size()) outFile << ",";
        outFile << "\n";
    }
    outFile << "  ]\n}\n";
    outFile.close();
}

void syncUsersToDisk() {
    saveUsersToFile(USERS_FILE, userManager.getAllUsers());
}

void syncRidesToDisk() {
    saveRidesToFile(RIDES_FILE, rideManager.all_pending_requests(), rideManager.get_history());
}

void initializePersistence() {
    cout << "[Storage] Checking persistent storage files...\n";
    
    // 1. Users
    vector<User> savedUsers = loadUsersFromFile(USERS_FILE);
    if (!savedUsers.empty()) {
        cout << "[Storage] Found " << savedUsers.size() << " saved users in " << USERS_FILE << ". Loading into Linked List...\n";
        for (const auto& u : savedUsers) {
            userManager.registerUserDirect(u);
        }
    } else {
        cout << "[Storage] Initializing default sample users into Linked List and saving to " << USERS_FILE << "...\n";
        User u1; u1.id = 1001; u1.name = "Aryan Sharma"; u1.phone = "9876543210"; u1.gender = "Male";
        u1.source = "Sector 62, Noida"; u1.destination = "Botanical Garden, Noida";
        u1.preferredTime = "09:30"; u1.transportMode = "Metro"; u1.budget = 150.0;
        u1.hasRequest = true; u1.genderPreference = "Any"; u1.wantsSharedCab = false; u1.allowMultiple = true;
        userManager.registerUserDirect(u1);

        User u2; u2.id = 1002; u2.name = "Priya Verma"; u2.phone = "9812345678"; u2.gender = "Female";
        u2.source = "Sector 62, Noida"; u2.destination = "Botanical Garden, Noida";
        u2.preferredTime = "09:40"; u2.transportMode = "Metro"; u2.budget = 120.0;
        u2.hasRequest = true; u2.genderPreference = "Any"; u2.wantsSharedCab = false; u2.allowMultiple = true;
        userManager.registerUserDirect(u2);

        User u3; u3.id = 1003; u3.name = "Rohan Gupta"; u3.phone = "9923456789"; u3.gender = "Male";
        u3.source = "Sector 62, Noida"; u3.destination = "Connaught Place, Delhi";
        u3.preferredTime = "09:30"; u3.transportMode = "Cab"; u3.budget = 250.0;
        u3.hasRequest = true; u3.genderPreference = "Male"; u3.wantsSharedCab = true; u3.boardingStation = "Sector 62"; u3.allowMultiple = true;
        userManager.registerUserDirect(u3);

        User u4; u4.id = 1004; u4.name = "Kavita Rao"; u4.phone = "9734567890"; u4.gender = "Female";
        u4.source = "Sector 62, Noida"; u4.destination = "Connaught Place, Delhi";
        u4.preferredTime = "09:45"; u4.transportMode = "Cab"; u4.budget = 300.0;
        u4.hasRequest = true; u4.genderPreference = "Any"; u4.wantsSharedCab = true; u4.boardingStation = "Sector 62"; u4.allowMultiple = true;
        userManager.registerUserDirect(u4);

        User u5; u5.id = 1005; u5.name = "Amit Kumar"; u5.phone = "9645678901"; u5.gender = "Male";
        u5.source = "Dwarka Sector 21"; u5.destination = "Rajiv Chowk";
        u5.preferredTime = "10:00"; u5.transportMode = "Metro"; u5.budget = 80.0;
        u5.hasRequest = true; u5.genderPreference = "Any"; u5.wantsSharedCab = false; u5.allowMultiple = false;
        userManager.registerUserDirect(u5);

        syncUsersToDisk();
    }

    // 2. Rides
    vector<riderequest> savedPending;
    vector<Complete_Ride> savedHistory;
    loadRidesFromFile(RIDES_FILE, savedPending, savedHistory);
    
    if (!savedPending.empty() || !savedHistory.empty()) {
        cout << "[Storage] Found " << savedPending.size() << " pending rides and " << savedHistory.size() << " history records in " << RIDES_FILE << ". Loading into Ride Manager...\n";
        for (const auto& r : savedPending) rideManager.load_pending_request(r);
        for (const auto& h : savedHistory) rideManager.load_history_record(h);
    } else {
        cout << "[Storage] Initializing default ride requests in FIFO Queue and saving to " << RIDES_FILE << "...\n";
        rideManager.add_ride_req(1001, "Sector 62, Noida", "Botanical Garden, Noida", 930, "Metro", 150.0);
        rideManager.add_ride_req(1002, "Sector 62, Noida", "Botanical Garden, Noida", 940, "Metro", 120.0);
        rideManager.add_ride_req(1003, "Sector 62, Noida", "Connaught Place, Delhi", 930, "Cab", 250.0);
        rideManager.add_ride_req(1004, "Sector 62, Noida", "Connaught Place, Delhi", 945, "Cab", 300.0);
        rideManager.add_ride_req(1005, "Dwarka Sector 21", "Rajiv Chowk", 1000, "Metro", 80.0);
        syncRidesToDisk();
    }
    
    cout << "[Storage] Persistent Storage System ready!\n";
}

void handleClient(SOCKET clientSocket) {
    char buffer[16384];
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead <= 0) {
        closesocket(clientSocket);
        return;
    }
    buffer[bytesRead] = '\0';
    string request(buffer);

    // Check Expect: 100-continue (e.g., from .NET / PowerShell)
    if (request.find("100-continue") != string::npos) {
        string cont = "HTTP/1.1 100 Continue\r\n\r\n";
        send(clientSocket, cont.c_str(), (int)cont.length(), 0);
    }

    // Parse method and path
    stringstream ss(request);
    string method, url, proto;
    ss >> method >> url >> proto;

    string path = url;
    size_t qPos = path.find('?');
    if (qPos != string::npos) path = path.substr(0, qPos);

    // Find body (if POST/PUT)
    string body = "";
    size_t bodyPos = request.find("\r\n\r\n");
    if (bodyPos != string::npos) {
        body = request.substr(bodyPos + 4);
    }

    // Check Content-Length to ensure complete body is received
    size_t clPos = request.find("Content-Length:");
    if (clPos == string::npos) clPos = request.find("content-length:");
    if (clPos != string::npos) {
        size_t clStart = request.find_first_of("0123456789", clPos);
        if (clStart != string::npos) {
            int expectedLen = 0;
            try { expectedLen = stoi(request.substr(clStart)); } catch(...) {}
            while ((int)body.length() < expectedLen) {
                char moreBuf[2048];
                int moreBytes = recv(clientSocket, moreBuf, sizeof(moreBuf) - 1, 0);
                if (moreBytes <= 0) break;
                moreBuf[moreBytes] = '\0';
                body.append(moreBuf, moreBytes);
            }
        }
    }

    // CORS & JSON Headers with persistent Connection: close
    string corsHeaders = 
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type\r\n"
        "Connection: close\r\n";

    auto sendJsonResponse = [&](const string& status, const string& jsonBody) {
        string res = "HTTP/1.1 " + status + "\r\n" + corsHeaders +
                     "Content-Type: application/json\r\n" +
                     "Content-Length: " + to_string(jsonBody.length()) + "\r\n\r\n" + jsonBody;
        send(clientSocket, res.c_str(), (int)res.length(), 0);
        shutdown(clientSocket, SD_SEND);
        closesocket(clientSocket);
    };

    auto sendFileResponse = [&](const string& contentType, const string& content) {
        string res = "HTTP/1.1 200 OK\r\n" + corsHeaders + 
                     "Content-Type: " + contentType + "\r\n" +
                     "Content-Length: " + to_string(content.length()) + "\r\n\r\n" + content;
        send(clientSocket, res.c_str(), (int)res.length(), 0);
        shutdown(clientSocket, SD_SEND);
        closesocket(clientSocket);
    };

    auto sendNotFound = [&]() {
        string res = "HTTP/1.1 404 Not Found\r\n" + corsHeaders + "Content-Length: 0\r\n\r\n";
        send(clientSocket, res.c_str(), (int)res.length(), 0);
        shutdown(clientSocket, SD_SEND);
        closesocket(clientSocket);
    };

    if (method == "OPTIONS") {
        string res = "HTTP/1.1 204 No Content\r\n" + corsHeaders + "Content-Length: 0\r\n\r\n";
        send(clientSocket, res.c_str(), (int)res.length(), 0);
        shutdown(clientSocket, SD_SEND);
        closesocket(clientSocket);
        return;
    }

    // Static Files
    if (method == "GET" && (path == "/" || path == "/index.html")) {
        ifstream f("index.html", ios::binary);
        if (f) {
            ostringstream ssf; ssf << f.rdbuf();
            sendFileResponse("text/html; charset=utf-8", ssf.str());
        } else {
            sendNotFound();
        }
        return;
    }

    if (method == "GET" && path == "/roads_data.js") {
        ifstream f("roads_data.js", ios::binary);
        if (f) {
            ostringstream ssf; ssf << f.rdbuf();
            sendFileResponse("application/javascript; charset=utf-8", ssf.str());
        } else {
            sendNotFound();
        }
        return;
    }

    if (method == "GET" && path == "/locations_data.js") {
        ifstream f("locations_data.js", ios::binary);
        if (f) {
            ostringstream ssf; ssf << f.rdbuf();
            sendFileResponse("application/javascript; charset=utf-8", ssf.str());
        } else {
            sendNotFound();
        }
        return;
    }

    if (method == "GET" && (path == "/MARK_LOGO.PNG" || path == "/MARK_LOGO.png")) {
        ifstream f("MARK_LOGO.PNG", ios::binary);
        if (f) {
            ostringstream ssf; ssf << f.rdbuf();
            sendFileResponse("image/png", ssf.str());
        } else {
            sendNotFound();
        }
        return;
    }

    if (method == "GET" && path == "/locations.csv") {
        ifstream f("locations.csv", ios::binary);
        if (f) {
            ostringstream ssf; ssf << f.rdbuf();
            sendFileResponse("text/csv; charset=utf-8", ssf.str());
        } else {
            sendNotFound();
        }
        return;
    }

    // API ENDPOINTS

    // 1. GET /api/stats
    if (method == "GET" && path == "/api/stats") {
        int totalUsers = userManager.getUserCount();
        int activeRequests = userManager.getActiveRequestCount();
        int matchedCount = userManager.getMatchedCount();
        int completedRides = rideManager.get_history_size();
        double revenue = rideManager.get_total_revenue();

        ostringstream json;
        json << "{"
             << "\"totalUsers\":" << totalUsers << ","
             << "\"activeRequests\":" << activeRequests << ","
             << "\"successfulMatches\":" << matchedCount << ","
             << "\"completedRides\":" << completedRides << ","
             << "\"totalRevenue\":" << revenue << ","
             << "\"pendingQueueSize\":" << rideManager.get_queue_size()
             << "}";

        sendJsonResponse("200 OK", json.str());
        return;
    }

    // 2. GET /api/users
    if (method == "GET" && (path == "/api/users" || path == "/api/users/list")) {
        vector<User> users = userManager.getAllUsers();
        ostringstream json;
        json << "[";
        for (size_t i = 0; i < users.size(); ++i) {
            json << userToJson(users[i]);
            if (i + 1 < users.size()) json << ",";
        }
        json << "]";

        sendJsonResponse("200 OK", json.str());
        return;
    }

    // 3. POST /api/users (Register User with Automatic Unique 4-Digit ID)
    if (method == "POST" && (path == "/api/users" || path == "/api/users/add")) {
        string name = getJsonString(body, "name");
        string phone = getJsonString(body, "phone");
        string gender = getJsonString(body, "gender");
        string email = getJsonString(body, "email");
        if (gender.empty()) gender = getJsonString(body, "type");
        if (gender.empty() && !email.empty()) gender = email;
        if (gender.empty()) gender = "Daily Commuter";

        if (name.empty()) {
            sendJsonResponse("400 Bad Request", "{\"success\":false,\"message\":\"Name cannot be empty.\"}");
            return;
        }

        // Automatic unique 4-digit ID generation (1000 - 9999)
        int newId = userManager.generateUniqueId();

        User u;
        u.id = newId;
        u.name = name;
        u.phone = phone;
        u.gender = gender;

        bool ok = userManager.registerUserDirect(u);
        if (ok) syncUsersToDisk();
        cout << "[User Directory] Automatically generated ID #" << newId 
             << " -> Registered user: " << name << " (Phone: " << phone << ")\n";

        ostringstream json;
        json << "{\"success\":" << (ok ? "true" : "false") << ","
             << "\"id\":" << newId << ","
             << "\"userId\":" << newId << ","
             << "\"user\":" << userToJson(u) << ","
             << "\"message\":\"User registered successfully! Your unique 4-digit User ID is " << newId << ". Please remember this ID to log in and access your account.\"}";

        sendJsonResponse("200 OK", json.str());
        return;
    }

    // 4. GET /api/users/search?id=...
    if (method == "GET" && path == "/api/users/search") {
        string idStr = getQueryParam(url, "id");
        int id = 0;
        try { if (!idStr.empty()) id = stoi(idStr); } catch (...) {}
        User u;
        bool found = (id > 0) && userManager.getUserById(id, u);
        cout << "[Linked List] Searching for user ID #" << id << (found ? " (Found)" : " (Not Found)") << "\n";

        ostringstream json;
        if (found) {
            json << "{\"success\":true,\"user\":" << userToJson(u) << "}";
        } else {
            json << "{\"success\":false,\"message\":\"User #" << id << " not found in Linked List.\"}";
        }

        sendJsonResponse("200 OK", json.str());
        return;
    }

    // 5. POST /api/users/update
    if (method == "POST" && path == "/api/users/update") {
        int id = getJsonInt(body, "id");
        string name = getJsonString(body, "name");
        string phone = getJsonString(body, "phone");
        string gender = getJsonString(body, "gender");

        bool ok = userManager.updateUserDirect(id, name, phone, gender);
        if (ok) syncUsersToDisk();
        cout << "[Linked List] " << (ok ? "Updated user #" : "Failed update for user #") << id << "\n";

        ostringstream json;
        json << "{\"success\":" << (ok ? "true" : "false") << ","
             << "\"message\":\"" << (ok ? "User updated successfully in Linked List!" : "User not found") << "\"}";

        sendJsonResponse("200 OK", json.str());
        return;
    }

    // 6. POST /api/users/delete?id=... or body id
    if (method == "POST" && path == "/api/users/delete") {
        string idStr = getQueryParam(url, "id");
        int id = 0;
        try { if (!idStr.empty()) id = stoi(idStr); } catch (...) {}
        if (id == 0) id = getJsonInt(body, "id");

        bool ok = (id > 0) && userManager.deleteUser(id);
        if (ok) {
            auto pending = rideManager.all_pending_requests();
            for (const auto& r : pending) {
                if (r.user_id == id) {
                    rideManager.cancel_request(r.id);
                }
            }
            syncUsersToDisk();
            syncRidesToDisk();
        }
        cout << "[Linked List] " << (ok ? "Deleted user node #" : "Could not delete user #") << id << "\n";

        ostringstream json;
        json << "{\"success\":" << (ok ? "true" : "false") << ","
             << "\"message\":\"" << (ok ? "User deleted successfully from Linked List!" : "User not found") << "\"}";

        sendJsonResponse("200 OK", json.str());
        return;
    }

    // 7. POST /api/rides/request (Request a Ride for Logged-in User)
    if (method == "POST" && path == "/api/rides/request") {
        int userId = getJsonInt(body, "userId");
        string src = getJsonString(body, "source");
        string dst = getJsonString(body, "destination");
        string time = getJsonString(body, "preferredTime");
        if (time.empty()) time = getJsonString(body, "time");
        if (time.empty()) time = "09:00 AM";

        string mode = getJsonString(body, "transportMode");
        if (mode.empty()) mode = getJsonString(body, "mode");
        if (mode.empty()) mode = "Cab";

        double budget = getJsonDouble(body, "budget", 200.0);
        string genderPref = getJsonString(body, "genderPreference");
        if (genderPref.empty()) genderPref = "Any";
        bool wantsCab = getJsonBool(body, "wantsSharedCab", true);
        string station = getJsonString(body, "boardingStation");
        if (station.empty()) station = getJsonString(body, "station");
        bool allowMultiple = getJsonBool(body, "allowMultiple", true);

        User u;
        if (!userManager.getUserById(userId, u)) {
            sendJsonResponse("400 Bad Request", "{\"success\":false,\"message\":\"User #" + to_string(userId) + " not registered. Register first!\"}");
            return;
        }

        // Prevent duplicate active ride requests per user
        if (u.hasRequest) {
            sendJsonResponse("400 Bad Request", "{\"success\":false,\"message\":\"User #" + to_string(userId) + " already has an active ride request in progress. Only 1 active request is allowed per user.\"}");
            return;
        }

        // Set request in Linked List
        userManager.setRideRequestDirect(userId, src, dst, time, mode, budget, genderPref, wantsCab, station, allowMultiple);

        // Convert time to numeric HHMM (e.g. 900, 930) for Ride_Manager
        int hh = 9, mm = 0;
        string digitsOnly = "";
        for (char c : time) if (isdigit(c) || c == ':') digitsOnly += c;
        size_t col = digitsOnly.find(':');
        if (col != string::npos) {
            try {
                hh = stoi(digitsOnly.substr(0, col));
                if (col + 1 < digitsOnly.length()) mm = stoi(digitsOnly.substr(col + 1));
            } catch (...) {}
        } else if (!digitsOnly.empty()) {
            try {
                int val = stoi(digitsOnly);
                if (val >= 100) { hh = val / 100; mm = val % 100; }
                else { hh = val; mm = 0; }
            } catch (...) {}
        }
        string timeLower = toLower(time);
        if (timeLower.find("pm") != string::npos && hh < 12) hh += 12;
        if (timeLower.find("am") != string::npos && hh == 12) hh = 0;
        int numericTime = hh * 100 + mm;
        if (numericTime == 0) numericTime = 900;

        // Push to FIFO Queue & Undo Stack
        rideManager.add_ride_req(userId, src, dst, numericTime, mode, budget);
        syncUsersToDisk();
        syncRidesToDisk();
        cout << "[Queue FIFO] Enqueued ride request for User #" << userId << " (" << src << " -> " << dst << ", Time: " << numericTime << ", Mode: " << mode << ")\n";

        ostringstream json;
        json << "{\"success\":true,\"message\":\"Ride request saved & pushed to FIFO Queue!\"}";

        sendJsonResponse("200 OK", json.str());
        return;
    }

    // 8. GET /api/rides/matches or /api/matches/find (Find Ride Matches for Logged-in User)
    if ((method == "GET" && (path == "/api/rides/matches" || path == "/api/matches/find" || path == "/api/matches")) ||
        (method == "POST" && (path == "/api/rides/matches" || path == "/api/matches/find" || path == "/api/matches"))) {
        int userId = 0;
        if (method == "GET") {
            string uidStr = getQueryParam(url, "userId");
            if (uidStr.empty()) uidStr = getQueryParam(url, "id");
            try { if (!uidStr.empty()) userId = stoi(uidStr); } catch (...) { userId = 0; }
        } else {
            userId = getJsonInt(body, "userId");
            if (userId == 0) userId = getJsonInt(body, "id");
        }

        User user;

        if (userId <= 0 || !userManager.getUserById(userId, user)) {
            sendJsonResponse("404 Not Found", "{\"success\":false,\"message\":\"User #" + to_string(userId) + " not found.\"}");
            return;
        }

        if (!user.hasRequest) {
            sendJsonResponse("200 OK", "{\"success\":false,\"message\":\"User #" + to_string(userId) + " has no active ride request. Please submit a ride request first.\"}");
            return;
        }

        // Run Priority Queue Matching Engine in C++
        MatchQueue pq = matchRides(user, userManager.getAllUsers());
        cout << "[Priority Queue] Evaluated ride matches for Logged-in User #" << userId 
             << " -> Found " << pq.size() << " compatible matches.\n";

        ostringstream json;
        json << "{\"success\":true,\"userId\":" << userId << ",\"totalMatches\":" << pq.size() << ",\"matches\":[";
        bool first = true;
        while (!pq.empty()) {
            match_class m = pq.top();
            pq.pop();
            if (!first) json << ",";
            first = false;

            json << "{"
                 << "\"candidate\":" << userToJson(m.candidate) << ","
                 << "\"score\":" << m.score << ","
                 << "\"matchType\":\"" << escapeJson(m.matchType) << "\","
                 << "\"sharedSegments\":" << m.sharedSegments << ","
                 << "\"timeDifference\":" << m.timeDifference << ","
                 << "\"estimatedFare\":" << m.estimatedFare
                 << "}";
        }
        json << "]}";

        sendJsonResponse("200 OK", json.str());
        return;
    }

    // 9. POST /api/rides/confirm_match or /api/matches/accept
    if (method == "POST" && (path == "/api/rides/confirm_match" || path == "/api/matches/accept")) {
        int u1 = getJsonInt(body, "user1");
        if (u1 == 0) u1 = getJsonInt(body, "userId");
        int u2 = getJsonInt(body, "user2");
        if (u2 == 0) u2 = getJsonInt(body, "partnerId");

        userManager.markMatched(u1, u2);
        userManager.markMatched(u2, u1);
        syncUsersToDisk();
        cout << "[Matching Engine] Confirmed match between User #" << u1 << " and User #" << u2 << "\n";

        ostringstream json;
        json << "{\"success\":true,\"message\":\"Match confirmed successfully between User #" << u1 << " and User #" << u2 << "!\"}";
        sendJsonResponse("200 OK", json.str());
        return;
    }

    // 10. POST /api/rides/process (Process next ride from FIFO Queue)
    if (method == "POST" && path == "/api/rides/process") {
        double fare = getJsonDouble(body, "fare", 180.0);
        if (rideManager.get_queue_size() == 0) {
            sendJsonResponse("200 OK", "{\"success\":false,\"message\":\"No pending ride requests in FIFO queue.\"}");
            return;
        }

        rideManager.process_nextride(fare);
        syncUsersToDisk();
        syncRidesToDisk();
        cout << "[Ride Manager] Processed next ride from FIFO Queue with fare $" << fare << " -> Stored in History Vector & Logged in Stack\n";

        ostringstream json;
        json << "{\"success\":true,\"message\":\"Ride processed, stored in History Vector, and logged to Undo Stack!\"}";
        sendJsonResponse("200 OK", json.str());
        return;
    }

    // 11. POST /api/rides/undo (Undo Last Action via Stack)
    if (method == "POST" && path == "/api/rides/undo") {
        RideAction undoneAction;
        bool ok = rideManager.undo(&undoneAction);
        if (ok) {
            if (undoneAction.type == BOOK) {
                userManager.clearRequest(undoneAction.req.user_id);
                cout << "[Linked List] Cleared active ride request for User #" << undoneAction.req.user_id << " due to undo.\n";
            } else if (undoneAction.type == COMPLETE) {
                for (int uid : undoneAction.rec.user_ids) {
                    userManager.resetMatch(uid);
                }
            }
        }
        if (ok) {
            syncUsersToDisk();
            syncRidesToDisk();
        }
        cout << "[Stack LIFO] Undo action triggered: " << (ok ? "Success (Restored state)" : "Stack is empty") << "\n";

        ostringstream json;
        json << "{\"success\":" << (ok ? "true" : "false") << ","
             << "\"message\":\"" << (ok ? "Undo successful: Popped last action from Stack!" : "Undo Stack is empty!") << "\"}";
        sendJsonResponse("200 OK", json.str());
        return;
    }

    // 11.5 POST /api/rides/cancel or /api/rides/clear (Cancel an active ride request for a user)
    if (method == "POST" && (path == "/api/rides/cancel" || path == "/api/rides/clear")) {
        int uid = getJsonInt(body, "userId");
        if (uid == 0) {
            string uidStr = getQueryParam(url, "userId");
            try { if (!uidStr.empty()) uid = stoi(uidStr); } catch (...) {}
        }
        if (uid > 0) {
            userManager.clearRequest(uid);
            auto pending = rideManager.all_pending_requests();
            for (const auto& r : pending) {
                if (r.user_id == uid) {
                    rideManager.cancel_request(r.id);
                }
            }
            cout << "[Ride Dispatcher] Cancelled active ride request for User #" << uid << "\n";
            syncUsersToDisk();
            syncRidesToDisk();
            sendJsonResponse("200 OK", "{\"success\":true,\"message\":\"Active ride request cancelled successfully.\"}");
            return;
        }
        sendJsonResponse("400 Bad Request", "{\"success\":false,\"message\":\"User ID required.\"}");
        return;
    }

    // 12. GET /api/rides/queue (Get pending queue items)
    if (method == "GET" && path == "/api/rides/queue") {
        auto q = rideManager.all_pending_requests();
        ostringstream json;
        json << "[";
        for (size_t i = 0; i < q.size(); ++i) {
            json << "{"
                 << "\"id\":" << q[i].id << ","
                 << "\"user_id\":" << q[i].user_id << ","
                 << "\"initial_dest\":\"" << escapeJson(q[i].initial_destination) << "\","
                 << "\"final_dest\":\"" << escapeJson(q[i].final_destination) << "\","
                 << "\"time\":" << q[i].time << ","
                 << "\"mode\":\"" << escapeJson(q[i].mode) << "\","
                 << "\"budget\":" << q[i].budget
                 << "}";
            if (i + 1 < q.size()) json << ",";
        }
        json << "]";

        sendJsonResponse("200 OK", json.str());
        return;
    }

    // 13. GET /api/rides/history (Get completed rides vector)
    if (method == "GET" && path == "/api/rides/history") {
        const auto& h = rideManager.get_history();
        ostringstream json;
        json << "[";
        for (size_t i = 0; i < h.size(); ++i) {
            json << "{"
                 << "\"id\":" << h[i].id << ","
                 << "\"user_id\":" << (h[i].user_ids.empty() ? 0 : h[i].user_ids[0]) << ","
                 << "\"initial_dest\":\"" << escapeJson(h[i].initial_destination) << "\","
                 << "\"final_dest\":\"" << escapeJson(h[i].final_destination) << "\","
                 << "\"mode\":\"" << escapeJson(h[i].mode) << "\","
                 << "\"fare\":" << h[i].fare
                 << "}";
            if (i + 1 < h.size()) json << ",";
        }
        json << "]";

        sendJsonResponse("200 OK", json.str());
        return;
    }

    // 13.5 GET /api/locations/search?q=... or GET /api/locations
    if (method == "GET" && (path == "/api/locations/search" || path == "/api/locations")) {
        string q = getQueryParam(url, "q");
        if (q.empty()) q = getQueryParam(url, "query");

        vector<LocationRecord> matches;
        if (!q.empty()) {
            matches = routeManager.searchLocations(q, 15);
        } else {
            matches = routeManager.getAllLocations();
        }

        ostringstream json;
        json << "{"
             << "\"success\":true,"
             << "\"query\":\"" << escapeJson(q) << "\","
             << "\"total\":" << matches.size() << ","
             << "\"locations\":[";
        for (size_t i = 0; i < matches.size(); ++i) {
            json << "{"
                 << "\"name\":\"" << escapeJson(matches[i].name) << "\","
                 << "\"type\":\"" << escapeJson(matches[i].type) << "\","
                 << "\"city\":\"" << escapeJson(matches[i].city) << "\","
                 << "\"lat\":" << matches[i].lat << ","
                 << "\"lon\":" << matches[i].lon
                 << "}";
            if (i + 1 < matches.size()) json << ",";
        }
        json << "]}";

        sendJsonResponse("200 OK", json.str());
        return;
    }

    // 14. GET /api/routes/dijkstra — Shortest distance using Dijkstra on OSM road graph
    if (method == "GET" && path == "/api/routes/dijkstra") {
        string sLatStr = getQueryParam(url, "startLat");
        string sLonStr = getQueryParam(url, "startLon");
        string eLatStr = getQueryParam(url, "endLat");
        string eLonStr = getQueryParam(url, "endLon");

        if (sLatStr.empty() || sLonStr.empty() || eLatStr.empty() || eLonStr.empty()) {
            sendJsonResponse("400 Bad Request", "{\"success\":false,\"error\":\"Missing startLat/startLon/endLat/endLon\"}");
            return;
        }

        try {
            double sLat = stod(sLatStr), sLon = stod(sLonStr);
            double eLat = stod(eLatStr), eLon = stod(eLonStr);
            RoadRouteResult result = routeManager.getRoadDijkstraRoute(sLat, sLon, eLat, eLon);
            cout << "[Dijkstra] Route from (" << sLat << "," << sLon << ") to (" << eLat << "," << eLon
                 << ") -> " << (result.success ? "OK" : "FAIL") << " dist=" << result.distanceKm << " km, hops=" << result.hops << "\n";

            ostringstream json;
            json << fixed << setprecision(7);
            json << "{\"success\":" << (result.success ? "true" : "false") << ","
                 << "\"distanceKm\":" << setprecision(2) << result.distanceKm << ","
                 << "\"distance\":" << result.distanceKm << ","
                 << "\"hops\":" << result.hops << ","
                 << "\"path\":[";
            for (size_t i = 0; i < result.path.size(); ++i) {
                json << "\"" << escapeJson(result.path[i]) << "\"";
                if (i + 1 < result.path.size()) json << ",";
            }
            json << "],\"coordinates\":[";
            json << setprecision(7);
            for (size_t i = 0; i < result.coordinates.size(); ++i) {
                json << "[" << result.coordinates[i].first << "," << result.coordinates[i].second << "]";
                if (i + 1 < result.coordinates.size()) json << ",";
            }
            json << "]}";
            sendJsonResponse("200 OK", json.str());
        } catch (...) {
            sendJsonResponse("400 Bad Request", "{\"success\":false,\"error\":\"Invalid coordinates\"}");
        }
        return;
    }

    // 15. GET /api/routes/bfs — Minimum connections using BFS on OSM road graph
    if (method == "GET" && path == "/api/routes/bfs") {
        string sLatStr = getQueryParam(url, "startLat");
        string sLonStr = getQueryParam(url, "startLon");
        string eLatStr = getQueryParam(url, "endLat");
        string eLonStr = getQueryParam(url, "endLon");

        if (sLatStr.empty() || sLonStr.empty() || eLatStr.empty() || eLonStr.empty()) {
            sendJsonResponse("400 Bad Request", "{\"success\":false,\"error\":\"Missing startLat/startLon/endLat/endLon\"}");
            return;
        }

        try {
            double sLat = stod(sLatStr), sLon = stod(sLonStr);
            double eLat = stod(eLatStr), eLon = stod(eLonStr);
            RoadRouteResult result = routeManager.getRoadBFSRoute(sLat, sLon, eLat, eLon);
            cout << "[BFS] Route from (" << sLat << "," << sLon << ") to (" << eLat << "," << eLon
                 << ") -> " << (result.success ? "OK" : "FAIL") << " hops=" << result.hops << "\n";

            ostringstream json;
            json << fixed << setprecision(7);
            json << "{\"success\":" << (result.success ? "true" : "false") << ","
                 << "\"distanceKm\":" << setprecision(2) << result.distanceKm << ","
                 << "\"hops\":" << result.hops << ","
                 << "\"path\":[";
            for (size_t i = 0; i < result.path.size(); ++i) {
                json << "\"" << escapeJson(result.path[i]) << "\"";
                if (i + 1 < result.path.size()) json << ",";
            }
            json << "],\"coordinates\":[";
            json << setprecision(7);
            for (size_t i = 0; i < result.coordinates.size(); ++i) {
                json << "[" << result.coordinates[i].first << "," << result.coordinates[i].second << "]";
                if (i + 1 < result.coordinates.size()) json << ",";
            }
            json << "]}";
            sendJsonResponse("200 OK", json.str());
        } catch (...) {
            sendJsonResponse("400 Bad Request", "{\"success\":false,\"error\":\"Invalid coordinates\"}");
        }
        return;
    }

    // Default 404
    sendNotFound();
}

int main(int argc, char* argv[]) {
    // Check if user requested CLI mode
    if (argc > 1 && string(argv[1]) == "-cli") {
        Ride_Manager manager;
        RouteManager routes;
        int choice;
        do {
            cout << "\n=== M.A.R.K CLI MODE ===\n";
            cout << "1. Add Ride Request\n2. Process Next Ride\n3. Undo\n4. View Queue\n5. View History\n6. Stats\n7. Exit\nChoice: ";
            cin >> choice;
            if (choice == 1) {
                int id, time; string s, d, m; double b;
                cout << "ID: "; cin >> id; cout << "Src: "; cin >> s; cout << "Dst: "; cin >> d;
                cout << "Time: "; cin >> time; cout << "Mode: "; cin >> m; cout << "Budget: "; cin >> b;
                manager.add_ride_req(id, s, d, time, m, b);
            } else if (choice == 2) {
                double f; cout << "Fare: "; cin >> f; manager.process_nextride(f);
            } else if (choice == 3) {
                manager.undo();
            } else if (choice == 4) {
                manager.displaypending_queue();
            } else if (choice == 5) {
                manager.display_ride_history();
            } else if (choice == 6) {
                manager.displaystats();
            }
        } while (choice != 7);
        return 0;
    }

    // Server mode
    cout << "=================================================================\n";
    cout << "   M.A.R.K. | Mapped Automation Route Kinematics (C++ Backend)   \n";
    cout << "=================================================================\n";
    cout << " Data Structures Implemented in C++:                             \n";
    cout << "   - Linked List     : Dynamic User Management                   \n";
    cout << "   - Queue (FIFO)    : Pending Ride Dispatching                  \n";
    cout << "   - Priority Queue  : Smart Multi-Parameter Ride Matching       \n";
    cout << "   - Stack (LIFO)    : Transactional Undo System                 \n";
    cout << "   - Vector          : Complete Ride History Records             \n";
    cout << "   - Graph & Dijkstra: Shortest Distance Route Navigation        \n";
    cout << "   - Graph & BFS     : Minimum Hops Route Navigation             \n";
    cout << "=================================================================\n";

    initializePersistence();

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        cerr << "Error: WSAStartup failed.\n";
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        cerr << "Error: Failed to create server socket.\n";
        WSACleanup();
        return 1;
    }

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "Error: Port 8080 is in use. Trying port 8000...\n";
        serverAddr.sin_port = htons(8000);
        if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            cerr << "Error: Bind failed on ports 8080 and 8000.\n";
            closesocket(serverSocket);
            WSACleanup();
            return 1;
        }
    }

    if (listen(serverSocket, 10) == SOCKET_ERROR) {
        cerr << "Error: Listen failed.\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    int port = ntohs(serverAddr.sin_port);
    cout << "\n[Server Online] Listening on http://localhost:" << port << "\n";
    cout << "[Server Online] Open your browser at: http://localhost:" << port << "\n";
    cout << "[Server Online] Ready to receive frontend requests. Press Ctrl+C to stop.\n";
    cout << "=================================================================\n\n";

    while (true) {
        sockaddr_in clientAddr;
        int clientLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientLen);
        if (clientSocket != INVALID_SOCKET) {
            handleClient(clientSocket);
        }
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
