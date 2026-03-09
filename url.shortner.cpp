#include "crow_all.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <sstream>

using namespace std;

// ==========================================
// MODELS (Entities)
// ==========================================
struct UrlMapping {
    string short_url;
    string long_url;
    string user_id;
    int click_count = 0;
    // In a real app we'd add creation_date, expiration_date here
};


// ==========================================
// UTILITIES (Base62 Strategy)
// ==========================================
class Base62Encoder {
public:
    // A simple method to generate a quasi-random string of a given length
    static string generate_random_string(int length = 6) {
        const string characters = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        string random_string;
        for (int i = 0; i < length; ++i) {
            random_string += characters[rand() % 62];
        }
        return random_string;
    }
};

// ==========================================
// REPOSITORY (Following DIP)
// ==========================================
// The interface for our storage. 
class IUrlRepository {
public:
    virtual void save(const UrlMapping& mapping) = 0;
    virtual UrlMapping* get_by_short_url(const string& short_url) = 0;
    virtual bool alias_exists(const string& alias) = 0;
    virtual void increment_click(const string& short_url) = 0;
    virtual ~IUrlRepository() = default;
};

// A concrete implementation using memory (a hash map)
class InMemoryUrlRepository : public IUrlRepository {
private:
    unordered_map<string, UrlMapping> db;

public:
    void save(const UrlMapping& mapping) override {
        db[mapping.short_url] = mapping;
    }

    UrlMapping* get_by_short_url(const string& short_url) override {
        if (db.find(short_url) != db.end()) {
            return &db[short_url];
        }
        return nullptr;
    }

    bool alias_exists(const string& alias) override {
        return db.find(alias) != db.end();
    }

    void increment_click(const string& short_url) override {
        if (db.find(short_url) != db.end()) {
            db[short_url].click_count++;
        }
    }
};


// ==========================================
// CONTROLLER (The Facade/Service)
// ==========================================
class UrlShortenerService {
private:
    IUrlRepository& repository;
    const string BASE_DOMAIN = "http://tinylink.co/";

public:
    // Dependency Injection!
    UrlShortenerService(IUrlRepository& repo) : repository(repo) {
        srand(time(0)); // Seed random for Base62
    }

    // 1. Core feature: Shorten a URL
    string shorten(const string& long_url, const string& user_id = "anonymous", string custom_alias = "") {
        string short_hash;

        // Handle Custom Aliases
        if (!custom_alias.empty()) {
            if (repository.alias_exists(custom_alias)) {
                return "Error: Alias '" + custom_alias + "' is already registered!";
            }
            short_hash = custom_alias;
        } 
        // Handle Random Generation
        else {
            short_hash = Base62Encoder::generate_random_string();
            // Ensure collision does not happen (very low probability, but required logic)
            while (repository.alias_exists(short_hash)) {
                short_hash = Base62Encoder::generate_random_string();
            }
        }

        // Save to Database
        UrlMapping mapping{short_hash, long_url, user_id, 0};
        repository.save(mapping);

        return BASE_DOMAIN + short_hash;
    }

    // 2. Core Feature: Expand / Redirect
    string expand(const string& full_short_url) {
        // Extract the hash from the full URL string
        string short_hash = full_short_url.substr(BASE_DOMAIN.length());

        UrlMapping* mapping = repository.get_by_short_url(short_hash);
        
        if (mapping != nullptr) {
            // Analytics handling
            repository.increment_click(short_hash);
            
            // "Redirect"
            cout << "[Redirecting...] -> " << full_short_url << " resolves to " << mapping->long_url << "\n";
            return mapping->long_url;
        }

        return "Error: URL Not Found! (404)";
    }

    // 3. Extra Feature: Analytics
    int get_click_count(const string& full_short_url) {
        string short_hash = full_short_url.substr(BASE_DOMAIN.length());
        UrlMapping* mapping = repository.get_by_short_url(short_hash);
        
        if (mapping != nullptr) {
            return mapping->click_count;
        }
        return -1;
    }
};

// ==========================================
// MAIN
// ==========================================
int main() {
    cout << "--- Initializing URL Shortener API ---\n";
    
    // Setup infrastructure
    InMemoryUrlRepository memory_db;
    UrlShortenerService service(memory_db);

    // Enable CORS middleware
    crow::App<crow::CORSHandler> app;

    // Configure CORS
    auto& cors = app.get_middleware<crow::CORSHandler>();
    cors
      .global()
      .headers("X-Custom-Header", "Upgrade-Insecure-Requests", "Content-Type")
      .methods("POST"_method, "GET"_method, "OPTIONS"_method)
      .origin("*");

    // Route: Serve Frontend
    CROW_ROUTE(app, "/")
    ([](){
        std::ifstream file("index.html");
        if (!file.is_open()) return crow::response(404, "index.html not found. Ensure it is in the same directory as the executable.");
        std::ostringstream ss;
        ss << file.rdbuf();
        crow::response res(ss.str());
        res.set_header("Content-Type", "text/html");
        return res;
    });

    // Make sure OPTIONS is accepted explicitly on /shorten
    CROW_ROUTE(app, "/shorten").methods(crow::HTTPMethod::Options)
    ([]() {
        return crow::response(204);
    });

    // Route: Shorten URL
    CROW_ROUTE(app, "/shorten").methods(crow::HTTPMethod::Post)
    ([&service](const crow::request& req){
        auto body = crow::json::load(req.body);
        if (!body) {
            return crow::response(400, "Invalid JSON");
        }

        if (!body.has("long_url")) {
            return crow::response(400, "Missing long_url");
        }

        string long_url = body["long_url"].s();
        string user_id = body.has("user_id") ? string(body["user_id"].s()) : "anonymous";
        string custom_alias = body.has("custom_alias") ? string(body["custom_alias"].s()) : "";

        string result = service.shorten(long_url, user_id, custom_alias);
        
        crow::json::wvalue res_body;
        if (result.rfind("Error", 0) == 0) {
            res_body["error"] = result;
            return crow::response(400, res_body);
        }

        // Success
        res_body["short_url"] = result;
        return crow::response(200, res_body);
    });

    // Route: Redirect
    CROW_ROUTE(app, "/<string>")
    ([&service](std::string short_hash){
        string full_url = "http://tinylink.co/" + short_hash;
        string result = service.expand(full_url);
        
        if (result.rfind("Error", 0) == 0) {
            return crow::response(404, "URL Not Found");
        }

        crow::response res(302);
        res.set_header("Location", result);
        return res;
    });

    // Route: Analytics
    CROW_ROUTE(app, "/analytics/<string>")
    ([&service](std::string short_hash){
        string full_url = "http://tinylink.co/" + short_hash;
        int clicks = service.get_click_count(full_url);
        
        if (clicks == -1) {
            return crow::response(404, "URL Not Found");
        }

        crow::json::wvalue res;
        res["short_url"] = full_url;
        res["click_count"] = clicks;
        return crow::response(200, res);
    });

    // Get PORT from environment variable (Required for Render/Heroku)
    const char* port_env = std::getenv("PORT");
    int port = port_env ? std::stoi(port_env) : 8080;

    cout << "Server starting on port " << port << "...\n";
    app.port(port).bindaddr("0.0.0.0").multithreaded().run();
    return 0;
}
