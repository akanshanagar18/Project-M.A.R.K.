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

// Convert User object to JSON
string userToJson(const User& u) {
    ostringstream ss;
    ss << "{"
       << "\"id\":" << u.id << ","
       << "\"name\":\"" << escapeJson(u.name) << "\","
       << "\"phone\":\"" << escapeJson(u.phone) << "\","
       << "\"gender\":\"" << escapeJson(u.gender) << "\","
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

void initializeSampleData() {
    cout << "[Initialization] Setting up realistic sample data in C++ Data Structures...\n";

    // 1. Aryan Sharma
    User u1; u1.id = 101; u1.name = "Aryan Sharma"; u1.phone = "9876543210"; u1.gender = "Male";
    u1.source = "Sector 62, Noida"; u1.destination = "Botanical Garden, Noida";
    u1.preferredTime = "09:30"; u1.transportMode = "Metro"; u1.budget = 150.0;
    u1.hasRequest = true; u1.genderPreference = "Any"; u1.wantsSharedCab = false; u1.allowMultiple = true;
    userManager.registerUserDirect(u1);
    rideManager.add_ride_req(101, u1.source, u1.destination, 930, u1.transportMode, u1.budget);

    // 2. Priya Verma
    User u2; u2.id = 102; u2.name = "Priya Verma"; u2.phone = "9812345678"; u2.gender = "Female";
    u2.source = "Sector 62, Noida"; u2.destination = "Botanical Garden, Noida";
    u2.preferredTime = "09:40"; u2.transportMode = "Metro"; u2.budget = 120.0;
    u2.hasRequest = true; u2.genderPreference = "Any"; u2.wantsSharedCab = false; u2.allowMultiple = true;
    userManager.registerUserDirect(u2);
    rideManager.add_ride_req(102, u2.source, u2.destination, 940, u2.transportMode, u2.budget);

    // 3. Rohan Gupta
    User u3; u3.id = 103; u3.name = "Rohan Gupta"; u3.phone = "9923456789"; u3.gender = "Male";
    u3.source = "Sector 62, Noida"; u3.destination = "Connaught Place, Delhi";
    u3.preferredTime = "09:30"; u3.transportMode = "Cab"; u3.budget = 250.0;
    u3.hasRequest = true; u3.genderPreference = "Male"; u3.wantsSharedCab = true; u3.boardingStation = "Sector 62"; u3.allowMultiple = true;
    userManager.registerUserDirect(u3);
    rideManager.add_ride_req(103, u3.source, u3.destination, 930, u3.transportMode, u3.budget);

    // 4. Kavita Rao
    User u4; u4.id = 104; u4.name = "Kavita Rao"; u4.phone = "9734567890"; u4.gender = "Female";
    u4.source = "Sector 62, Noida"; u4.destination = "Connaught Place, Delhi";
    u4.preferredTime = "09:45"; u4.transportMode = "Cab"; u4.budget = 300.0;
    u4.hasRequest = true; u4.genderPreference = "Any"; u4.wantsSharedCab = true; u4.boardingStation = "Sector 62"; u4.allowMultiple = true;
    userManager.registerUserDirect(u4);
    rideManager.add_ride_req(104, u4.source, u4.destination, 945, u4.transportMode, u4.budget);

    // 5. Amit Kumar
    User u5; u5.id = 105; u5.name = "Amit Kumar"; u5.phone = "9645678901"; u5.gender = "Male";
    u5.source = "Dwarka Sector 21"; u5.destination = "Rajiv Chowk";
    u5.preferredTime = "10:00"; u5.transportMode = "Metro"; u5.budget = 80.0;
    u5.hasRequest = true; u5.genderPreference = "Any"; u5.wantsSharedCab = false; u5.allowMultiple = false;
    userManager.registerUserDirect(u5);
    rideManager.add_ride_req(105, u5.source, u5.destination, 1000, u5.transportMode, u5.budget);

    cout << "[Initialization] Complete! 5 users in Linked List, 5 ride requests in FIFO Queue.\n";
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

    if (method == "OPTIONS") {
        string res = "HTTP/1.1 204 No Content\r\n" + corsHeaders + "Content-Length: 0\r\n\r\n";
        send(clientSocket, res.c_str(), res.length(), 0);
        closesocket(clientSocket);
        return;
    }

    // Static Files
    if (method == "GET" && (path == "/" || path == "/index.html")) {
        ifstream f("index.html", ios::binary);
        if (f) {
            ostringstream ssf; ssf << f.rdbuf();
            string content = ssf.str();
            string res = "HTTP/1.1 200 OK\r\n" + corsHeaders + 
                         "Content-Type: text/html; charset=utf-8\r\n" +
                         "Content-Length: " + to_string(content.length()) + "\r\n\r\n" + content;
            send(clientSocket, res.c_str(), res.length(), 0);
        } else {
            string notFound = "HTTP/1.1 404 Not Found\r\n\r\nindex.html not found";
            send(clientSocket, notFound.c_str(), notFound.length(), 0);
        }
        closesocket(clientSocket);
        return;
    }

    if (method == "GET" && path == "/roads_data.js") {
        ifstream f("roads_data.js", ios::binary);
        if (f) {
            ostringstream ssf; ssf << f.rdbuf();
            string content = ssf.str();
            string res = "HTTP/1.1 200 OK\r\n" + corsHeaders + 
                         "Content-Type: application/javascript; charset=utf-8\r\n" +
                         "Content-Length: " + to_string(content.length()) + "\r\n\r\n" + content;
            send(clientSocket, res.c_str(), res.length(), 0);
        } else {
            string notFound = "HTTP/1.1 404 Not Found\r\n\r\nroads_data.js not found";
            send(clientSocket, notFound.c_str(), notFound.length(), 0);
        }
        closesocket(clientSocket);
        return;
    }

    if (method == "GET" && (path == "/MARK_LOGO.PNG" || path == "/MARK_LOGO.png")) {
        ifstream f("MARK_LOGO.PNG", ios::binary);
        if (f) {
            ostringstream ssf; ssf << f.rdbuf();
            string content = ssf.str();
            string res = "HTTP/1.1 200 OK\r\n" + corsHeaders + 
                         "Content-Type: image/png\r\n" +
                         "Content-Length: " + to_string(content.length()) + "\r\n\r\n" + content;
            send(clientSocket, res.c_str(), res.length(), 0);
        } else {
            string notFound = "HTTP/1.1 404 Not Found\r\n\r\n";
            send(clientSocket, notFound.c_str(), notFound.length(), 0);
        }
        closesocket(clientSocket);
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

        string responseBody = json.str();
        string res = "HTTP/1.1 200 OK\r\n" + corsHeaders +
                     "Content-Type: application/json\r\n" +
                     "Content-Length: " + to_string(responseBody.length()) + "\r\n\r\n" + responseBody;
        send(clientSocket, res.c_str(), res.length(), 0);
        closesocket(clientSocket);
        return;
    }

    // 2. GET /api/users
    if (method == "GET" && path == "/api/users") {
        vector<User> users = userManager.getAllUsers();
        ostringstream json;
        json << "[";
        for (size_t i = 0; i < users.size(); ++i) {
            json << userToJson(users[i]);
            if (i + 1 < users.size()) json << ",";
        }
        json << "]";

        string responseBody = json.str();
        string res = "HTTP/1.1 200 OK\r\n" + corsHeaders +
                     "Content-Type: application/json\r\n" +
                     "Content-Length: " + to_string(responseBody.length()) + "\r\n\r\n" + responseBody;
        send(clientSocket, res.c_str(), res.length(), 0);
        closesocket(clientSocket);
        return;
    }

    // 3. POST /api/users (Register User)
    if (method == "POST" && path == "/api/users") {
        int id = getJsonInt(body, "id");
        string name = getJsonString(body, "name");
        string phone = getJsonString(body, "phone");
        string gender = getJsonString(body, "gender");

        if (id <= 0 || name.empty()) {
            string err = "{\"success\":false,\"message\":\"Invalid user details\"}";
            string res = "HTTP/1.1 400 Bad Request\r\n" + corsHeaders + "Content-Type: application/json\r\n\r\n" + err;
            send(clientSocket, res.c_str(), res.length(), 0);
            closesocket(clientSocket);
            return;
        }

        User u;
        u.id = id;
        u.name = name;
        u.phone = phone;
        u.gender = gender.empty() ? "Other" : gender;

        bool ok = userManager.registerUserDirect(u);
        cout << "[Linked List] " << (ok ? "Registered user #" : "Failed registration (duplicate ID) #") 
             << id << " (" << name << ")\n";

        ostringstream json;
        json << "{\"success\":" << (ok ? "true" : "false") << ","
             << "\"message\":\"" << (ok ? "User registered successfully into Linked List!" : "User ID already exists!") << "\"}";

        string responseBody = json.str();
        string res = "HTTP/1.1 200 OK\r\n" + corsHeaders +
                     "Content-Type: application/json\r\n" +
                     "Content-Length: " + to_string(responseBody.length()) + "\r\n\r\n" + responseBody;
        send(clientSocket, res.c_str(), res.length(), 0);
        closesocket(clientSocket);
        return;
    }

    // 4. GET /api/users/search?id=...
    if (method == "GET" && path == "/api/users/search") {
        int id = stoi(getQueryParam(url, "id"));
        User u;
        bool found = userManager.getUserById(id, u);
        cout << "[Linked List] Searching for user ID #" << id << (found ? " (Found)" : " (Not Found)") << "\n";

        ostringstream json;
        if (found) {
            json << "{\"success\":true,\"user\":" << userToJson(u) << "}";
        } else {
            json << "{\"success\":false,\"message\":\"User #" << id << " not found in Linked List.\"}";
        }

        string responseBody = json.str();
        string res = "HTTP/1.1 200 OK\r\n" + corsHeaders +
                     "Content-Type: application/json\r\n" +
                     "Content-Length: " + to_string(responseBody.length()) + "\r\n\r\n" + responseBody;
        send(clientSocket, res.c_str(), res.length(), 0);
        closesocket(clientSocket);
        return;
    }

    // 5. POST /api/users/update
    if (method == "POST" && path == "/api/users/update") {
        int id = getJsonInt(body, "id");
        string name = getJsonString(body, "name");
        string phone = getJsonString(body, "phone");
        string gender = getJsonString(body, "gender");

        bool ok = userManager.updateUserDirect(id, name, phone, gender);
        cout << "[Linked List] " << (ok ? "Updated user #" : "Failed update for user #") << id << "\n";

        ostringstream json;
        json << "{\"success\":" << (ok ? "true" : "false") << ","
             << "\"message\":\"" << (ok ? "User updated successfully in Linked List!" : "User not found") << "\"}";

        string responseBody = json.str();
        string res = "HTTP/1.1 200 OK\r\n" + corsHeaders +
                     "Content-Type: application/json\r\n" +
                     "Content-Length: " + to_string(responseBody.length()) + "\r\n\r\n" + responseBody;
        send(clientSocket, res.c_str(), res.length(), 0);
        closesocket(clientSocket);
        return;
    }

    // 6. POST /api/users/delete?id=...
    if (method == "POST" && path == "/api/users/delete") {
        int id = stoi(getQueryParam(url, "id"));
        bool ok = userManager.deleteUser(id);
        cout << "[Linked List] " << (ok ? "Deleted user node #" : "Could not delete user #") << id << "\n";

        ostringstream json;
        json << "{\"success\":" << (ok ? "true" : "false") << ","
             << "\"message\":\"" << (ok ? "User deleted successfully from Linked List!" : "User not found") << "\"}";

        string responseBody = json.str();
        string res = "HTTP/1.1 200 OK\r\n" + corsHeaders +
                     "Content-Type: application/json\r\n" +
                     "Content-Length: " + to_string(responseBody.length()) + "\r\n\r\n" + responseBody;
        send(clientSocket, res.c_str(), res.length(), 0);
        closesocket(clientSocket);
        return;
    }

    // 7. POST /api/rides/request (Request a Ride)
    if (method == "POST" && path == "/api/rides/request") {
        int userId = getJsonInt(body, "userId");
        string src = getJsonString(body, "source");
        string dst = getJsonString(body, "destination");
        string time = getJsonString(body, "preferredTime");
        string mode = getJsonString(body, "transportMode");
        double budget = getJsonDouble(body, "budget", 200.0);
        string genderPref = getJsonString(body, "genderPreference");
        bool wantsCab = getJsonBool(body, "wantsSharedCab", false);
        string station = getJsonString(body, "boardingStation");
        bool allowMultiple = getJsonBool(body, "allowMultiple", true);

        User u;
        if (!userManager.getUserById(userId, u)) {
            string err = "{\"success\":false,\"message\":\"User #" + to_string(userId) + " not registered. Register first!\"}";
            string res = "HTTP/1.1 400 Bad Request\r\n" + corsHeaders + "Content-Type: application/json\r\n\r\n" + err;
            send(clientSocket, res.c_str(), res.length(), 0);
            closesocket(clientSocket);
            return;
        }

        // Set request in Linked List
        userManager.setRideRequestDirect(userId, src, dst, time, mode, budget, genderPref, wantsCab, station, allowMultiple);

        // Convert time to numeric (e.g. 930) for Ride_Manager
        int numericTime = 930;
        try {
            numericTime = parseTimeToMinutes(time);
        } catch (...) {}

        // Push to FIFO Queue & Undo Stack
        rideManager.add_ride_req(userId, src, dst, numericTime, mode, budget);
        cout << "[Queue FIFO] Enqueued ride request for User #" << userId << " (" << src << " ➔ " << dst << ")\n";

        ostringstream json;
        json << "{\"success\":true,\"message\":\"Ride request saved & pushed to FIFO Queue!\"}";

        string responseBody = json.str();
        string res = "HTTP/1.1 200 OK\r\n" + corsHeaders +
                     "Content-Type: application/json\r\n" +
                     "Content-Length: " + to_string(responseBody.length()) + "\r\n\r\n" + responseBody;
        send(clientSocket, res.c_str(), res.length(), 0);
        closesocket(clientSocket);
        return;
    }

    // 8. GET /api/rides/matches?userId=... or POST /api/rides/matches (Find Ride Matches using Priority Queue)
    if ((method == "GET" && path == "/api/rides/matches") || (method == "POST" && path == "/api/rides/matches")) {
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
            string err = "{\"success\":false,\"message\":\"User #" + to_string(userId) + " not found.\"}";
            string res = "HTTP/1.1 404 Not Found\r\n" + corsHeaders + "Content-Type: application/json\r\n\r\n" + err;
            send(clientSocket, res.c_str(), res.length(), 0);
            closesocket(clientSocket);
            return;
        }

        if (!user.hasRequest) {
            string err = "{\"success\":false,\"message\":\"User #" + to_string(userId) + " has no active ride request.\"}";
            string res = "HTTP/1.1 200 OK\r\n" + corsHeaders + "Content-Type: application/json\r\n\r\n" + err;
            send(clientSocket, res.c_str(), res.length(), 0);
            closesocket(clientSocket);
            return;
        }

        // Run Priority Queue Matching Engine in C++
        MatchQueue pq = matchRides(user, userManager.getAllUsers());
        cout << "[Priority Queue] Evaluated ride matches for User #" << userId 
             << " -> Found " << pq.size() << " compatible matches.\n";

        ostringstream json;
        json << "{\"success\":true,\"userId\":" << userId << ",\"matches\":[";
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

        string responseBody = json.str();
        string res = "HTTP/1.1 200 OK\r\n" + corsHeaders +
                     "Content-Type: application/json\r\n" +
                     "Content-Length: " + to_string(responseBody.length()) + "\r\n\r\n" + responseBody;
        send(clientSocket, res.c_str(), res.length(), 0);
        closesocket(clientSocket);
        return;
    }

    // 9. POST /api/rides/confirm_match (Confirm Match between 2 users)
    if (method == "POST" && path == "/api/rides/confirm_match") {
        int u1 = getJsonInt(body, "user1");
        int u2 = getJsonInt(body, "user2");

        userManager.markMatched(u1, u2);
        userManager.markMatched(u2, u1);
        cout << "[Matching Engine] Confirmed match between User #" << u1 << " and User #" << u2 << "\n";

        ostringstream json;
        json << "{\"success\":true,\"message\":\"Match confirmed successfully between User #" << u1 << " and User #" << u2 << "!\"}";
        string responseBody = json.str();
        string res = "HTTP/1.1 200 OK\r\n" + corsHeaders +
                     "Content-Type: application/json\r\n" +
                     "Content-Length: " + to_string(responseBody.length()) + "\r\n\r\n" + responseBody;
        send(clientSocket, res.c_str(), res.length(), 0);
        closesocket(clientSocket);
        return;
    }

    // 10. POST /api/rides/process (Process next ride from FIFO Queue)
    if (method == "POST" && path == "/api/rides/process") {
        double fare = getJsonDouble(body, "fare", 180.0);
        if (rideManager.get_queue_size() == 0) {
            string err = "{\"success\":false,\"message\":\"No pending ride requests in FIFO queue.\"}";
            string res = "HTTP/1.1 200 OK\r\n" + corsHeaders + "Content-Type: application/json\r\n\r\n" + err;
            send(clientSocket, res.c_str(), res.length(), 0);
            closesocket(clientSocket);
            return;
        }

        rideManager.process_nextride(fare);
        cout << "[Ride Manager] Processed next ride from FIFO Queue with fare $" << fare << " -> Stored in History Vector & Logged in Stack\n";

        ostringstream json;
        json << "{\"success\":true,\"message\":\"Ride processed, stored in History Vector, and logged to Undo Stack!\"}";
        string responseBody = json.str();
        string res = "HTTP/1.1 200 OK\r\n" + corsHeaders +
                     "Content-Type: application/json\r\n" +
                     "Content-Length: " + to_string(responseBody.length()) + "\r\n\r\n" + responseBody;
        send(clientSocket, res.c_str(), res.length(), 0);
        closesocket(clientSocket);
        return;
    }

    // 11. POST /api/rides/undo (Undo Last Action via Stack)
    if (method == "POST" && path == "/api/rides/undo") {
        bool ok = rideManager.undo();
        cout << "[Stack LIFO] Undo action triggered: " << (ok ? "Success (Restored state)" : "Stack is empty") << "\n";

        ostringstream json;
        json << "{\"success\":" << (ok ? "true" : "false") << ","
             << "\"message\":\"" << (ok ? "Undo successful: Popped last action from Stack!" : "Undo Stack is empty!") << "\"}";
        string responseBody = json.str();
        string res = "HTTP/1.1 200 OK\r\n" + corsHeaders +
                     "Content-Type: application/json\r\n" +
                     "Content-Length: " + to_string(responseBody.length()) + "\r\n\r\n" + responseBody;
        send(clientSocket, res.c_str(), res.length(), 0);
        closesocket(clientSocket);
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

        string responseBody = json.str();
        string res = "HTTP/1.1 200 OK\r\n" + corsHeaders +
                     "Content-Type: application/json\r\n" +
                     "Content-Length: " + to_string(responseBody.length()) + "\r\n\r\n" + responseBody;
        send(clientSocket, res.c_str(), res.length(), 0);
        closesocket(clientSocket);
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

        string responseBody = json.str();
        string res = "HTTP/1.1 200 OK\r\n" + corsHeaders +
                     "Content-Type: application/json\r\n" +
                     "Content-Length: " + to_string(responseBody.length()) + "\r\n\r\n" + responseBody;
        send(clientSocket, res.c_str(), res.length(), 0);
        closesocket(clientSocket);
        return;
    }

    // 14. GET /api/routes/dijkstra
    if (method == "GET" && path == "/api/routes/dijkstra") {
        string start = getQueryParam(url, "start");
        if (start.empty()) start = getQueryParam(url, "src");
        string end = getQueryParam(url, "end");
        if (end.empty()) end = getQueryParam(url, "dst");
        auto result = routeManager.getShortestPath(start, end);
        cout << "[Graph Dijkstra] Evaluated shortest path between " << start << " and " << end 
             << " -> Distance: " << result.second << " km\n";

        ostringstream json;
        json << "{"
             << "\"success\":" << (result.second != -1.0 ? "true" : "false") << ","
             << "\"distance\":" << result.second << ","
             << "\"distanceKm\":" << result.second << ","
             << "\"path\":[";
        for (size_t i = 0; i < result.first.size(); ++i) {
            json << "\"" << escapeJson(result.first[i]) << "\"";
            if (i + 1 < result.first.size()) json << ",";
        }
        json << "]}";

        string responseBody = json.str();
        string res = "HTTP/1.1 200 OK\r\n" + corsHeaders +
                     "Content-Type: application/json\r\n" +
                     "Content-Length: " + to_string(responseBody.length()) + "\r\n\r\n" + responseBody;
        send(clientSocket, res.c_str(), res.length(), 0);
        closesocket(clientSocket);
        return;
    }

    // 15. GET /api/routes/bfs
    if (method == "GET" && path == "/api/routes/bfs") {
        string start = getQueryParam(url, "start");
        if (start.empty()) start = getQueryParam(url, "src");
        string end = getQueryParam(url, "end");
        if (end.empty()) end = getQueryParam(url, "dst");
        vector<string> pathNodes = routeManager.getBFSPath(start, end);
        cout << "[Graph BFS] Evaluated minimum connections between " << start << " and " << end 
             << " -> Hops: " << (pathNodes.empty() ? 0 : pathNodes.size() - 1) << "\n";

        ostringstream json;
        json << "{"
             << "\"success\":" << (!pathNodes.empty() ? "true" : "false") << ","
             << "\"hops\":" << (pathNodes.empty() ? 0 : (int)pathNodes.size() - 1) << ","
             << "\"path\":[";
        for (size_t i = 0; i < pathNodes.size(); ++i) {
            json << "\"" << escapeJson(pathNodes[i]) << "\"";
            if (i + 1 < pathNodes.size()) json << ",";
        }
        json << "]}";

        string responseBody = json.str();
        string res = "HTTP/1.1 200 OK\r\n" + corsHeaders +
                     "Content-Type: application/json\r\n" +
                     "Content-Length: " + to_string(responseBody.length()) + "\r\n\r\n" + responseBody;
        send(clientSocket, res.c_str(), res.length(), 0);
        closesocket(clientSocket);
        return;
    }

    // Default 404
    string notFound = "HTTP/1.1 404 Not Found\r\n" + corsHeaders + "Content-Length: 0\r\n\r\n";
    send(clientSocket, notFound.c_str(), notFound.length(), 0);
    closesocket(clientSocket);
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

    initializeSampleData();

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
