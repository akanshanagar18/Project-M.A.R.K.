#ifndef DATABASE_H
#define DATABASE_H

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>
#include "sqlite3.h"
#include "sha256.h"
#include "users_management.cpp"
#include "ride.h"

using namespace std;

class DatabaseManager {
private:
    sqlite3* db;
    bool is_open;

public:
    DatabaseManager() : db(nullptr), is_open(false) {}

    ~DatabaseManager() {
        close();
    }

    void close() {
        if (db) {
            sqlite3_close(db);
            db = nullptr;
            is_open = false;
        }
    }

    bool isOpen() const { return is_open; }

    bool init(const string& dbPath = "mark.db") {
        int rc = sqlite3_open(dbPath.c_str(), &db);
        if (rc != SQLITE_OK) {
            cerr << "[Database] Cannot open database " << dbPath << ": " << sqlite3_errmsg(db) << "\n";
            is_open = false;
            return false;
        }
        is_open = true;

        // Enable Foreign Keys
        char* errMsg = nullptr;
        rc = sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK) {
            cerr << "[Database] Failed to enable foreign keys: " << errMsg << "\n";
            sqlite3_free(errMsg);
        }

        // Create Tables
        const char* schema = 
            "CREATE TABLE IF NOT EXISTS users ("
            "    id INTEGER PRIMARY KEY,"
            "    name TEXT NOT NULL,"
            "    phone TEXT,"
            "    gender TEXT,"
            "    password_hash TEXT,"
            "    salt TEXT,"
            "    source TEXT DEFAULT '',"
            "    destination TEXT DEFAULT '',"
            "    preferred_time TEXT DEFAULT '',"
            "    transport_mode TEXT DEFAULT '',"
            "    budget REAL DEFAULT 0.0,"
            "    has_request INTEGER DEFAULT 0,"
            "    is_matched INTEGER DEFAULT 0,"
            "    matched_partner_id INTEGER DEFAULT 0,"
            "    gender_preference TEXT DEFAULT 'Any',"
            "    wants_shared_cab INTEGER DEFAULT 0,"
            "    boarding_station TEXT DEFAULT '',"
            "    allow_multiple INTEGER DEFAULT 1,"
            "    created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
            ");"
            "CREATE TABLE IF NOT EXISTS sessions ("
            "    token TEXT PRIMARY KEY,"
            "    user_id INTEGER NOT NULL,"
            "    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
            "    FOREIGN KEY(user_id) REFERENCES users(id) ON DELETE CASCADE"
            ");"
            "CREATE TABLE IF NOT EXISTS ride_requests ("
            "    id INTEGER PRIMARY KEY,"
            "    user_id INTEGER NOT NULL,"
            "    source TEXT,"
            "    destination TEXT,"
            "    time INTEGER,"
            "    mode TEXT,"
            "    budget REAL,"
            "    status TEXT DEFAULT 'PENDING',"
            "    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
            "    FOREIGN KEY(user_id) REFERENCES users(id)"
            ");"
            "CREATE TABLE IF NOT EXISTS rides ("
            "    id INTEGER PRIMARY KEY,"
            "    user_id INTEGER,"
            "    source TEXT,"
            "    destination TEXT,"
            "    mode TEXT,"
            "    fare REAL,"
            "    status TEXT DEFAULT 'COMPLETED',"
            "    created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
            ");"
            "CREATE TABLE IF NOT EXISTS routes ("
            "    id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "    name TEXT,"
            "    source TEXT,"
            "    destination TEXT,"
            "    distance_km REAL,"
            "    hops INTEGER,"
            "    created_at DATETIME DEFAULT CURRENT_TIMESTAMP"
            ");";

        rc = sqlite3_exec(db, schema, nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK) {
            cerr << "[Database] Schema creation failed: " << errMsg << "\n";
            sqlite3_free(errMsg);
            return false;
        }

        cout << "[Database] SQLite initialized successfully with persistent database: " << dbPath << "\n";
        return true;
    }

    // ------------------- USER OPERATIONS -------------------

    bool registerUser(const User& u, const string& plainPassword = "") {
        if (!is_open) return false;

        string salt = SHA256::generateSalt(16);
        string pwdToHash = plainPassword.empty() ? to_string(u.id) : plainPassword;
        string hash = SHA256::hashPassword(pwdToHash, salt);

        const char* sql = "INSERT INTO users (id, name, phone, gender, password_hash, salt, "
                          "source, destination, preferred_time, transport_mode, budget, "
                          "has_request, is_matched, matched_partner_id, gender_preference, "
                          "wants_shared_cab, boarding_station, allow_multiple) "
                          "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            cerr << "[Database] prepare failed: " << sqlite3_errmsg(db) << "\n";
            return false;
        }

        sqlite3_bind_int(stmt, 1, u.id);
        sqlite3_bind_text(stmt, 2, u.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, u.phone.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, u.gender.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, hash.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 6, salt.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 7, u.source.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 8, u.destination.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 9, u.preferredTime.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 10, u.transportMode.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 11, u.budget);
        sqlite3_bind_int(stmt, 12, u.hasRequest ? 1 : 0);
        sqlite3_bind_int(stmt, 13, u.isMatched ? 1 : 0);
        sqlite3_bind_int(stmt, 14, u.matchedPartnerId);
        sqlite3_bind_text(stmt, 15, u.genderPreference.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 16, u.wantsSharedCab ? 1 : 0);
        sqlite3_bind_text(stmt, 17, u.boardingStation.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 18, u.allowMultiple ? 1 : 0);

        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (rc != SQLITE_DONE) {
            cerr << "[Database] User registration failed: " << sqlite3_errmsg(db) << "\n";
            return false;
        }
        return true;
    }

    bool authenticateUser(int id, const string& password, string& outToken, User& outUser) {
        if (!is_open) return false;

        const char* sql = "SELECT password_hash, salt, name, phone, gender, source, destination, "
                          "preferred_time, transport_mode, budget, has_request, is_matched, "
                          "matched_partner_id, gender_preference, wants_shared_cab, "
                          "boarding_station, allow_multiple FROM users WHERE id = ?;";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }

        sqlite3_bind_int(stmt, 1, id);

        if (sqlite3_step(stmt) != SQLITE_ROW) {
            sqlite3_finalize(stmt);
            return false;
        }

        string storedHash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        string salt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));

        string computedHash = SHA256::hashPassword(password, salt);
        if (computedHash != storedHash) {
            sqlite3_finalize(stmt);
            return false; // Invalid password
        }

        // Reconstruct User
        outUser.id = id;
        outUser.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        outUser.phone = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        outUser.gender = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        outUser.source = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        outUser.destination = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        outUser.preferredTime = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        outUser.transportMode = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        outUser.budget = sqlite3_column_double(stmt, 9);
        outUser.hasRequest = (sqlite3_column_int(stmt, 10) == 1);
        outUser.isMatched = (sqlite3_column_int(stmt, 11) == 1);
        outUser.matchedPartnerId = sqlite3_column_int(stmt, 12);
        outUser.genderPreference = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 13));
        outUser.wantsSharedCab = (sqlite3_column_int(stmt, 14) == 1);
        outUser.boardingStation = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 15));
        outUser.allowMultiple = (sqlite3_column_int(stmt, 16) == 1);

        sqlite3_finalize(stmt);

        // Generate session token
        outToken = SHA256::generateToken();
        createSession(outToken, id);
        return true;
    }

    bool createSession(const string& token, int userId) {
        if (!is_open) return false;
        const char* sql = "INSERT OR REPLACE INTO sessions (token, user_id) VALUES (?, ?);";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, userId);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return (rc == SQLITE_DONE);
    }

    bool validateSession(const string& token, int& outUserId) {
        if (!is_open || token.empty()) return false;
        const char* sql = "SELECT user_id FROM sessions WHERE token = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            outUserId = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);
            return true;
        }
        sqlite3_finalize(stmt);
        return false;
    }

    void deleteSession(const string& token) {
        if (!is_open || token.empty()) return;
        const char* sql = "DELETE FROM sessions WHERE token = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            sqlite3_bind_text(stmt, 1, token.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(stmt);
            sqlite3_finalize(stmt);
        }
    }

    vector<User> loadAllUsers() {
        vector<User> users;
        if (!is_open) return users;

        const char* sql = "SELECT id, name, phone, gender, source, destination, preferred_time, "
                          "transport_mode, budget, has_request, is_matched, matched_partner_id, "
                          "gender_preference, wants_shared_cab, boarding_station, allow_multiple "
                          "FROM users ORDER BY id ASC;";

        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return users;
        }

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            User u;
            u.id = sqlite3_column_int(stmt, 0);
            const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            const char* phone = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            const char* gender = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            const char* src = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            const char* dst = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            const char* time = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
            const char* mode = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
            
            u.name = name ? name : "";
            u.phone = phone ? phone : "";
            u.gender = gender ? gender : "Other";
            u.source = src ? src : "";
            u.destination = dst ? dst : "";
            u.preferredTime = time ? time : "";
            u.transportMode = mode ? mode : "";
            u.budget = sqlite3_column_double(stmt, 8);
            u.hasRequest = (sqlite3_column_int(stmt, 9) == 1);
            u.isMatched = (sqlite3_column_int(stmt, 10) == 1);
            u.matchedPartnerId = sqlite3_column_int(stmt, 11);
            
            const char* gPref = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
            u.genderPreference = gPref ? gPref : "Any";
            u.wantsSharedCab = (sqlite3_column_int(stmt, 13) == 1);
            const char* bStn = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 14));
            u.boardingStation = bStn ? bStn : "";
            u.allowMultiple = (sqlite3_column_int(stmt, 15) == 1);

            users.push_back(u);
        }
        sqlite3_finalize(stmt);
        return users;
    }

    bool updateUserProfile(int id, const string& name, const string& phone, const string& gender) {
        if (!is_open) return false;
        const char* sql = "UPDATE users SET name = COALESCE(NULLIF(?, ''), name), "
                          "phone = COALESCE(NULLIF(?, ''), phone), "
                          "gender = COALESCE(NULLIF(?, ''), gender) WHERE id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, phone.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, gender.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 4, id);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return (rc == SQLITE_DONE);
    }

    bool updateUserRideState(const User& u) {
        if (!is_open) return false;
        const char* sql = "UPDATE users SET source = ?, destination = ?, preferred_time = ?, "
                          "transport_mode = ?, budget = ?, has_request = ?, is_matched = ?, "
                          "matched_partner_id = ?, gender_preference = ?, wants_shared_cab = ?, "
                          "boarding_station = ?, allow_multiple = ? WHERE id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

        sqlite3_bind_text(stmt, 1, u.source.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, u.destination.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, u.preferredTime.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, u.transportMode.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 5, u.budget);
        sqlite3_bind_int(stmt, 6, u.hasRequest ? 1 : 0);
        sqlite3_bind_int(stmt, 7, u.isMatched ? 1 : 0);
        sqlite3_bind_int(stmt, 8, u.matchedPartnerId);
        sqlite3_bind_text(stmt, 9, u.genderPreference.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 10, u.wantsSharedCab ? 1 : 0);
        sqlite3_bind_text(stmt, 11, u.boardingStation.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 12, u.allowMultiple ? 1 : 0);
        sqlite3_bind_int(stmt, 13, u.id);

        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return (rc == SQLITE_DONE);
    }

    bool deleteUser(int id) {
        if (!is_open) return false;
        const char* sql = "DELETE FROM users WHERE id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_int(stmt, 1, id);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return (rc == SQLITE_DONE);
    }

    // ------------------- RIDE REQUESTS OPERATIONS -------------------

    bool saveRideRequest(const riderequest& req) {
        if (!is_open) return false;
        const char* sql = "INSERT OR REPLACE INTO ride_requests (id, user_id, source, destination, time, mode, budget, status) "
                          "VALUES (?, ?, ?, ?, ?, ?, ?, 'PENDING');";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

        sqlite3_bind_int(stmt, 1, req.id);
        sqlite3_bind_int(stmt, 2, req.user_id);
        sqlite3_bind_text(stmt, 3, req.initial_destination.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, req.final_destination.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 5, req.time);
        sqlite3_bind_text(stmt, 6, req.mode.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 7, req.budget);

        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return (rc == SQLITE_DONE);
    }

    vector<riderequest> loadAllPendingRequests() {
        vector<riderequest> list;
        if (!is_open) return list;

        const char* sql = "SELECT id, user_id, source, destination, time, mode, budget "
                          "FROM ride_requests WHERE status = 'PENDING' ORDER BY id ASC;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            riderequest req;
            req.id = sqlite3_column_int(stmt, 0);
            req.user_id = sqlite3_column_int(stmt, 1);
            const char* src = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            const char* dst = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            req.initial_destination = src ? src : "";
            req.final_destination = dst ? dst : "";
            req.time = sqlite3_column_int(stmt, 4);
            const char* mode = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
            req.mode = mode ? mode : "";
            req.budget = sqlite3_column_double(stmt, 6);
            list.push_back(req);
        }
        sqlite3_finalize(stmt);
        return list;
    }

    bool updateRideRequestStatus(int reqId, const string& status) {
        if (!is_open) return false;
        const char* sql = "UPDATE ride_requests SET status = ? WHERE id = ?;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, reqId);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return (rc == SQLITE_DONE);
    }

    // ------------------- COMPLETED RIDES OPERATIONS -------------------

    bool saveCompletedRide(const Complete_Ride& ride) {
        if (!is_open) return false;
        const char* sql = "INSERT OR REPLACE INTO rides (id, user_id, source, destination, mode, fare, status) "
                          "VALUES (?, ?, ?, ?, ?, ?, 'COMPLETED');";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

        sqlite3_bind_int(stmt, 1, ride.id);
        int uid = ride.user_ids.empty() ? 0 : ride.user_ids[0];
        sqlite3_bind_int(stmt, 2, uid);
        sqlite3_bind_text(stmt, 3, ride.initial_destination.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, ride.final_destination.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, ride.mode.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 6, ride.fare);

        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return (rc == SQLITE_DONE);
    }

    vector<Complete_Ride> loadAllCompletedRides() {
        vector<Complete_Ride> list;
        if (!is_open) return list;

        const char* sql = "SELECT id, user_id, source, destination, mode, fare FROM rides ORDER BY id ASC;";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            Complete_Ride r;
            r.id = sqlite3_column_int(stmt, 0);
            int uid = sqlite3_column_int(stmt, 1);
            if (uid > 0) r.user_ids.push_back(uid);
            const char* src = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            const char* dst = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            r.initial_destination = src ? src : "";
            r.final_destination = dst ? dst : "";
            const char* mode = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            r.mode = mode ? mode : "";
            r.fare = sqlite3_column_double(stmt, 5);
            list.push_back(r);
        }
        sqlite3_finalize(stmt);
        return list;
    }

    // ------------------- ROUTES OPERATIONS -------------------

    bool saveRoute(const string& name, const string& src, const string& dst, double distKm, int hops) {
        if (!is_open) return false;
        const char* sql = "INSERT INTO routes (name, source, destination, distance_km, hops) VALUES (?, ?, ?, ?, ?);";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, src.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, dst.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 4, distKm);
        sqlite3_bind_int(stmt, 5, hops);
        int rc = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return (rc == SQLITE_DONE);
    }

    int getMaxUserId() {
        if (!is_open) return 1000;
        const char* sql = "SELECT MAX(id) FROM users;";
        sqlite3_stmt* stmt = nullptr;
        int maxId = 1000;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                maxId = sqlite3_column_int(stmt, 0);
                if (maxId < 1000) maxId = 1000;
            }
            sqlite3_finalize(stmt);
        }
        return maxId;
    }

    int getMaxRequestId() {
        if (!is_open) return 100;
        const char* sql = "SELECT MAX(id) FROM ride_requests;";
        sqlite3_stmt* stmt = nullptr;
        int maxId = 100;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                maxId = sqlite3_column_int(stmt, 0);
                if (maxId < 100) maxId = 100;
            }
            sqlite3_finalize(stmt);
        }
        return maxId;
    }

    int getMaxRideId() {
        if (!is_open) return 5000;
        const char* sql = "SELECT MAX(id) FROM rides;";
        sqlite3_stmt* stmt = nullptr;
        int maxId = 5000;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                maxId = sqlite3_column_int(stmt, 0);
                if (maxId < 5000) maxId = 5000;
            }
            sqlite3_finalize(stmt);
        }
        return maxId;
    }
};

#endif
