#pragma once
#include <functional>
#include <map>
#include <string>
#include "core/query.h"

namespace p3a::dev {
// Loopback HTTP server for driving and observing the mod while the game runs (the dev loop, after Grim Dark's).
// It starts only when P3A_PORT is set (p3.py launch and play; never a Steam launch). Requests a browser marks
// (Origin, Referer, Sec-Fetch-*) are refused: a web page could otherwise reach 127.0.0.1 too.
// Routes are plain-text GET (a POST body arrives as `body`). Each request runs on its own thread, so handlers may
// run concurrently; anything that touches game objects must go through hooks::run_on_game_thread. A handler whose
// game-thread job timed out is answered 503 whatever it returned. "+" in a query is a literal plus, not a space.
// The request's text and its parsing are core/query (engine-free, unit-tested).
using Query = core::Query;
using Request = core::Request;
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
using core::contains_ci;   // case-insensitive substring (haystack, needle)
}  // namespace p3a::dev
