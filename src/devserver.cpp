// Altered from Grim Dark (GD Access) by Austin Hicks; zlib, see LICENSES/. This is not the original software.
#include "devserver.h"
#include "hooks.h"
#include "log.h"
#include "seh.h"
#include "speech.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <thread>

namespace p3a::dev {
struct Route { Handler h; std::string help; };
static std::mutex g_mu;
static std::map<std::string, Route> g_routes;
static std::atomic<bool> g_run{false};
static std::atomic<int> g_inflight{0};
static SOCKET g_listen = INVALID_SOCKET;
static std::thread* g_thread = nullptr;   // never a static std::thread: one still joinable at process exit terminates
constexpr int kMaxInflight = 32;
constexpr DWORD kSocketTimeoutMs = 5000;

void route(const std::string& path, Handler h, const char* help) {
  std::lock_guard lk(g_mu);
  g_routes[path] = Route{std::move(h), help ? help : ""};
}

// "+" is NOT a space here: every client encodes spaces as %20, and key chords (k=w+d) arrive intact.
static std::string url_decode(std::string_view s) {
  std::string o;
  for (size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '%' && i + 2 < s.size()) { o += (char)strtol(std::string(s.substr(i + 1, 2)).c_str(), nullptr, 16); i += 2; }
    else o += s[i];
  }
  return o;
}
static Query parse_query(std::string_view q) {
  Query m;
  size_t p = 0;
  while (p <= q.size()) {
    size_t e = q.find('&', p); if (e == std::string_view::npos) e = q.size();
    std::string_view kv = q.substr(p, e - p);
    size_t eq = kv.find('=');
    if (!kv.empty()) m[url_decode(kv.substr(0, eq))] = eq == std::string_view::npos ? "1" : url_decode(kv.substr(eq + 1));
    p = e + 1;
  }
  return m;
}
static Request parse_target(const std::string& target) {
  Request req;
  size_t qm = target.find('?');
  req.path = target.substr(0, qm);
  if (req.path.empty() || req.path[0] != '/') req.path = "/" + req.path;
  if (qm != std::string::npos) req.q = parse_query(std::string_view(target).substr(qm + 1));
  return req;
}

std::string arg(const Request& r, const char* key, const char* def) { auto it = r.q.find(key); return it == r.q.end() ? def : it->second; }
bool has(const Request& r, const char* key) { return r.q.count(key) > 0; }
long long arg_int(const Request& r, const char* key, long long def) {
  auto it = r.q.find(key); if (it == r.q.end() || it->second.empty()) return def;
  const std::string& s = it->second;
  return strtoll(s.c_str(), nullptr, s.rfind("0x", 0) == 0 || s.rfind("-0x", 0) == 0 ? 16 : 10);
}
double arg_num(const Request& r, const char* key, double def) { auto it = r.q.find(key); return it == r.q.end() || it->second.empty() ? def : atof(it->second.c_str()); }

static std::string dispatch(const Request& req, int& status) {
  status = 200;
  if (req.path == "/" || req.path == "/help") {
    std::lock_guard lk(g_mu);
    std::string out;
    for (auto& [p, r] : g_routes) out += p + "  " + r.help + "\n";
    return out;
  }
  Handler h;
  { std::lock_guard lk(g_mu); auto it = g_routes.find(req.path); if (it != g_routes.end()) h = it->second.h; }
  if (!h) { status = 404; return "no such route (see /help)\n"; }
  // An access violation in a handler that pokes at game memory must not take the game down.
  std::string out;
  unsigned timeouts = hooks::timeouts_on_this_thread();
  auto run = [&] { out = h(req, status); };
  if (!guarded(run)) { status = 500; return "route faulted (access violation)\n"; }
  // A game-thread job that never ran: the handler's text is a placeholder, not the game's state.
  if (hooks::timeouts_on_this_thread() != timeouts) {
    status = 503;
    return std::format("game thread busy or frozen (frame {}, last tick {} ms ago)\n", hooks::frame(), (int)hooks::ms_since_tick()) + out;
  }
  return out;
}
// A route called from inside another (/state, /wait, /batch): "screen", "/world?range=300".
static std::string call(const std::string& target, int& status) {
  try { return dispatch(parse_target(target), status); } catch (const std::exception& e) { status = 500; return std::string("exception: ") + e.what() + "\n"; }
}
bool contains_ci(const std::string& h, const std::string& n) {
  return std::search(h.begin(), h.end(), n.begin(), n.end(), [](char a, char b) { return tolower((unsigned char)a) == tolower((unsigned char)b); }) != h.end();
}
std::string on_game(std::function<std::string()> fn) {
  auto out = std::make_shared<std::string>("game thread did not run the job\n");
  hooks::run_on_game_thread([out, fn] { *out = fn(); });
  return *out;
}

// Routes built from the others: one snapshot for a script step, a server-side wait instead of sleep-and-poll, and
// several routes in one request.
static void builtin_routes() {
  route("/state", [](const Request& r, int& status) {
    std::string out;
    for (const char* t : {"/screen", "/msg", "/world", "/nav"}) {
      int st = 200;
      std::string s = call(t, st);
      if (st != 200) status = st;
      out += std::format("== {}{}\n{}", t + 1, st == 200 ? "" : std::format(" ({})", st), s);
      if (!out.empty() && out.back() != '\n') out += '\n';
    }
    auto& ring = speech::history();
    uint64_t cur = ring.cursor(), n = (uint64_t)arg_int(r, "speech", 5);
    out += "== speech\n";
    for (auto& [i, l] : ring.since(cur > n ? cur - n : 0).first) out += std::format("{}: {}\n", i, l);
    return out;
  }, "?speech=5 screen, msg, world, nav and the last spoken lines in one call");
  route("/wait", [](const Request& r, int& status) {
    // /wait?on=screen&match=Title[&not=1][&timeout=10][&every=100]: polls the route until its text contains (or with
    // not=1 lacks) the match. on=speech matches only lines spoken after `since` (default: when the wait began).
    std::string on = arg(r, "on", "screen"), m = arg(r, "match");
    bool neg = arg(r, "not", "0") == "1";
    double secs = arg_num(r, "timeout", 10);
    DWORD every = (DWORD)std::clamp<long long>(arg_int(r, "every", 100), 20, 5000);
    bool speech_on = on == "speech" || on == "/speech";
    uint64_t since = (uint64_t)arg_int(r, "since", (long long)speech::history().cursor());
    ULONGLONG t0 = GetTickCount64(), limit = (ULONGLONG)(secs * 1000);
    std::string last;
    while (g_run) {
      int st = 200;
      if (speech_on) {
        last.clear();
        for (auto& [i, l] : speech::history().since(since).first) last += std::format("{}: {}\n", i, l);
      } else {
        last = call(on, st);
      }
      if (st == 200 && contains_ci(last, m) != neg) return std::format("matched after {} ms\n", GetTickCount64() - t0) + last;
      if (st == 404) { status = 404; return last; }
      if (GetTickCount64() - t0 >= limit) break;
      Sleep(every);
    }
    status = 408;
    return std::format("timeout after {} ms\n", GetTickCount64() - t0) + last;
  }, "?on=screen|msg|world|nav|speech|<route>&match=text[&not=1&timeout=s&since=] wait until a route shows the text");
  route("/batch", [](const Request& r, int& status) {
    // POST body or ?r=: one route per line ("|" also separates in ?r=); "sleep:ms" pauses.
    std::string body = r.body.empty() ? arg(r, "r") : r.body;
    std::replace(body.begin(), body.end(), '|', '\n');
    std::string out;
    size_t s = 0;
    while (s < body.size() && g_run) {
      size_t e = body.find('\n', s); if (e == std::string::npos) e = body.size();
      std::string line = body.substr(s, e - s);
      s = e + 1;
      while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
      if (line.empty()) continue;
      if (line.rfind("sleep:", 0) == 0) { Sleep((DWORD)atoi(line.c_str() + 6)); continue; }
      int st = 200;
      std::string res = call(line, st);
      if (st != 200) status = st;
      out += std::format("== {} {}\n{}", line, st, res);
      if (!res.empty() && res.back() != '\n') out += '\n';
    }
    return out;
  }, "POST lines or ?r=a|b routes in order (sleep:ms pauses)");
}

static void serve_one(SOCKET c) {
  DWORD to = kSocketTimeoutMs;
  setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, (const char*)&to, sizeof to);
  setsockopt(c, SOL_SOCKET, SO_SNDTIMEO, (const char*)&to, sizeof to);
  std::string data;
  char buf[8192];
  size_t header_end = std::string::npos;
  size_t content_length = 0;
  bool from_browser = false;
  while (data.size() < (8u << 20)) {
    int n = recv(c, buf, sizeof buf, 0);
    if (n <= 0) break;   // closed, or the 5 s timeout: a stalled client never holds the server
    data.append(buf, n);
    if (header_end == std::string::npos) {
      header_end = data.find("\r\n\r\n");
      if (header_end != std::string::npos) {
        std::string lower = data.substr(0, header_end);
        for (auto& ch : lower) ch = (char)tolower((unsigned char)ch);
        size_t cl = lower.find("content-length:");
        if (cl != std::string::npos) content_length = strtoul(lower.c_str() + cl + 15, nullptr, 10);
        // A web page the player has open can reach 127.0.0.1 too (an <img> or a fetch to /loadlib): browsers mark
        // those requests with these headers, which p3.py and curl never send.
        for (const char* h : {"\r\norigin:", "\r\nreferer:", "\r\nsec-fetch-site:", "\r\nsec-fetch-mode:"})
          if (lower.find(h) != std::string::npos) from_browser = true;
      }
    }
    if (header_end != std::string::npos && data.size() >= header_end + 4 + content_length) break;
  }
  if (header_end == std::string::npos || from_browser) { closesocket(c); return; }
  size_t sp1 = data.find(' '), sp2 = data.find(' ', sp1 + 1);
  std::string target = sp1 == std::string::npos ? "/" : data.substr(sp1 + 1, sp2 - sp1 - 1);
  Request req = parse_target(target);
  req.body = data.substr(header_end + 4);
  int status = 200;
  std::string body;
  ULONGLONG t0 = GetTickCount64();
  try { body = dispatch(req, status); } catch (const std::exception& e) { status = 500; body = std::string("exception: ") + e.what() + "\n"; }
  std::string resp = std::format("HTTP/1.1 {} X\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: {}\r\nX-Elapsed-Ms: {}\r\nConnection: close\r\n\r\n",
                                 status, body.size(), GetTickCount64() - t0) + body;
  const char* p = resp.data(); size_t left = resp.size();
  while (left) { int n = send(c, p, (int)(left > 65536 ? 65536 : left), 0); if (n <= 0) break; p += n; left -= n; }
  closesocket(c);
}

static void reject_busy(SOCKET c) {
  static const char kBusy[] = "HTTP/1.1 503 X\r\nContent-Type: text/plain\r\nContent-Length: 20\r\nConnection: close\r\n\r\nserver busy, retry\r\n";
  send(c, kBusy, (int)sizeof kBusy - 1, 0);
  closesocket(c);
}

void start(int port) {
  if (g_run) return;
  WSADATA wsa; WSAStartup(MAKEWORD(2, 2), &wsa);
  g_listen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  sockaddr_in a{}; a.sin_family = AF_INET; a.sin_port = htons((u_short)port); a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  BOOL yes = TRUE; setsockopt(g_listen, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (const char*)&yes, sizeof yes);
  if (bind(g_listen, (sockaddr*)&a, sizeof a) != 0 || listen(g_listen, 16) != 0) {
    log::writef("dev: bind/listen on 127.0.0.1:{} failed ({})", port, WSAGetLastError());
    closesocket(g_listen); g_listen = INVALID_SOCKET; return;
  }
  builtin_routes();
  g_run = true;
  // One thread per connection: a slow route (a long key sequence, a heap search, a frozen game thread) never holds
  // up the others. The counter lets stop() wait until no thread is running this DLL's code.
  g_thread = new std::thread([] {
    while (g_run) {
      SOCKET c = accept(g_listen, nullptr, nullptr);
      if (c == INVALID_SOCKET) { if (g_run) Sleep(10); continue; }
      if (g_inflight >= kMaxInflight) { reject_busy(c); continue; }
      ++g_inflight;
      std::thread([c] { serve_one(c); --g_inflight; }).detach();
    }
  });
  log::writef("dev: server on 127.0.0.1:{}", port);
}

void stop() {
  if (!g_run) return;
  g_run = false;
  closesocket(g_listen); g_listen = INVALID_SOCKET;
  if (g_thread) { g_thread->join(); delete g_thread; g_thread = nullptr; }
  for (int i = 0; i < 200 && g_inflight > 0; ++i) Sleep(50);
  if (g_inflight > 0) log::writef("dev: {} request(s) still running at stop", g_inflight.load());
  log::write("dev: server stopped");
}
}  // namespace p3a::dev
