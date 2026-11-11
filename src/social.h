#pragma once
#include <string>
#include <vector>
#include "ue/ue.h"

namespace p3a::social {
struct Link { int arcana; int rank; int points; };   // arcana 1 Fool .. 22 Aeon; rank 0 = not established
std::vector<Link> read();                // every link record (game thread)
std::string arcana_name(int id);
std::string person(int id);              // the partner's name as the camp shows it ("Kenji Tomochika")
std::string partner_rank(const std::string& name);   // "Magician, rank 3" when `name` is an established link's partner
bool reversed(int id);                   // the link is reversed (story flag; game thread)
bool ready(const Link& l);               // its points are enough for the next rank: the next hangout ranks it up
std::string describe(const Link& l);     // "Magician (Classmate), rank 3", then ", reversed" or ", ready to rank up"
std::string summary();                   // every established link
void tick();                             // announces new links, rank ups, points and reversals
// The rank card is up showing this link at this rank (screens/rank_up.cpp, every tick while it is up): the card
// speaks the change, so tick() leaves that rank to it. Returns what happened ("New Social Link") and what the card
// shows ("Magician Social Link, Kenji Tomochika, rank 1").
struct Card { std::string what, line; };
Card card(int arcana, int rank);
void install();                          // hooks CMM_RANKUP (the rank card's flowscript command); /social dev route
}  // namespace p3a::social
