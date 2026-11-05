#pragma once
#include <functional>
#include <map>
#include <string>

namespace p3a::dev {
// Loopback HTTP server for driving and observing the mod while the game runs (the dev loop, after Grim Dark's).
// It starts only when P3A_PORT is set (p3.py launch and play; never a Steam launch). Requests a browser marks
// (Origin, Referer, Sec-Fetch-*) are refused: a web page could otherwise reach 127.0.0.1 too.
// Routes are plain-text GET (a POST body arrives as `body`). Each request runs on its own thread, so handlers may
// run concurrently; anything that touches game objects must go through hooks::run_on_game_thread. A handler whose
// game-thread job timed out is answered 503 whatever it returned. "+" in a query is a literal plus, not a space.
using Query = std::map<std::string, std::string>;
struct Request { std::string path; Query q; std::string body; };
using Handler = std::function<std::string(const Request&, int& status)>;

void route(const std::string& path, Handler h, const char* help = "");
void start(int port);
void stop();

// Query helpers for handlers.
std::string arg(const Request& r, const char* key, const char* def = "");
long long arg_int(const Request& r, const char* key, long long def = 0);
double arg_num(const Request& r, const char* key, double def = 0);
bool has(const Request& r, const char* key);
// Runs fn on the game thread and returns its text (a timed-out job is answered 503 whatever this returns).
std::string on_game(std::function<std::string()> fn);
bool contains_ci(const std::string& haystack, const std::string& needle);   // case-insensitive substring
}  // namespace p3a::dev
