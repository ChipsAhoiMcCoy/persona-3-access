#pragma once
// msg_render.cpp: the game's BMD pages and choice entries as text (used by msg.cpp and bmdpages.cpp).
#include <cstdint>
#include <string>
#include <vector>
#include "ue/ue.h"

namespace p3a::msg {
void set_renderer(void* bmd_page_to_fstring);   // BmdPageToFString, found by msg::install
void* renderer();
std::vector<std::string> choices_of(ue::UObject* sel);   // each entry of a USelItem's list
std::string dump_tokens(uintptr_t page);        // a page's line and letter records (dev)
void log_if_icons(uintptr_t page, const std::string& text);   // logs a page holding icons, once
}  // namespace p3a::msg
