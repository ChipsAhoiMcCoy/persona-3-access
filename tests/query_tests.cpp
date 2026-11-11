// The dev server's request text: targets, queries, percent escapes, integers and the case-insensitive match.
#include "doctest.h"
#include <string>
#include "core/query.h"

using namespace p3a::core;

TEST_CASE("url_decode: percent escapes, and a plus stays a plus") {
  CHECK(url_decode("a%20b") == "a b");
  CHECK(url_decode("w+d") == "w+d");
  CHECK(url_decode("%41%7a") == "Az");
  CHECK(url_decode("") == "");
}

TEST_CASE("url_decode: an escape needs both its digits") {
  CHECK(url_decode("x%41") == "xA");
  CHECK(url_decode("x%4") == "x%4");   // cut short: left as written
  CHECK(url_decode("100%") == "100%");
}

TEST_CASE("parse_query: keys, values and bare keys") {
  Query q = parse_query("a=1&b=x%20y&c");
  CHECK(q.size() == 3);
  CHECK(q["a"] == "1");
  CHECK(q["b"] == "x y");
  CHECK(q["c"] == "1");   // a key alone reads as "1"
}

TEST_CASE("parse_query: empty parts, empty values, repeated keys") {
  CHECK(parse_query("").empty());
  Query q = parse_query("&a=&&a=2&k=w+d&e=a=b");
  CHECK(q.size() == 3);
  CHECK(q["a"] == "2");     // the later one replaces the earlier
  CHECK(q["k"] == "w+d");
  CHECK(q["e"] == "a=b");   // only the first "=" splits
  CHECK(parse_query("v=").at("v") == "");
}

TEST_CASE("parse_target: path and query") {
  Request r = parse_target("/world?range=300&all");
  CHECK(r.path == "/world");
  CHECK(r.q.at("range") == "300");
  CHECK(r.q.at("all") == "1");
  CHECK(r.body.empty());
}

TEST_CASE("parse_target: a path without its slash gets one") {
  CHECK(parse_target("screen").path == "/screen");
  CHECK(parse_target("screen").q.empty());
  CHECK(parse_target("").path == "/");
  CHECK(parse_target("?x=2").path == "/");
  CHECK(parse_target("?x=2").q.at("x") == "2");
  CHECK(parse_target("/ue/obj?").q.empty());
}

TEST_CASE("parse_int: decimal, or hexadecimal with 0x") {
  CHECK(parse_int("42") == 42);
  CHECK(parse_int("-7") == -7);
  CHECK(parse_int("0x50c") == 0x50c);
  CHECK(parse_int("-0x10") == -16);
  CHECK(parse_int("10abc") == 10);
  CHECK(parse_int("ff") == 0);    // hexadecimal only with its prefix
  CHECK(parse_int("abc") == 0);
}

TEST_CASE("contains_ci") {
  CHECK(contains_ci("BP_BtlTopUI_C", "btltop"));
  CHECK(contains_ci("Title menu", "MENU"));
  CHECK_FALSE(contains_ci("Title", "menu"));
  CHECK_FALSE(contains_ci("ab", "abc"));
  CHECK(contains_ci(std::string("abc"), std::string("")));   // an empty needle is in any text...
  CHECK_FALSE(contains_ci("", ""));                           // ...but an empty one
}
