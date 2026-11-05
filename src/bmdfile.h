#pragma once
#include <memory>
#include <string>
#include "core/bmd.h"
#include "ue/ue.h"

namespace p3a::bmdfile {
// A UBmdAsset's message file (mBuf) parsed; cached per asset until its buffer changes. Game thread.
std::shared_ptr<const core::bmd::File> load(ue::UObject* bmd_asset);
std::shared_ptr<const core::bmd::File> load(const char* asset_name);   // by object name ("BMD_CampSystem")
// The asset's message buffer (mBuf's data), what a page the game draws from it holds (msg::Created::buf): tells which
// file a drawn page came from, as dialog numbers repeat across files. Null when not loaded.
const void* buffer(const char* asset_name);
// A page as the game draws it: the hero's name filled in where the file has its token (function 4 2: "For Makoto.",
// "Fits Makoto."), functions removed, cleaned.
std::string text(const core::bmd::Page& p);
void install();   // /bmdfile dev route
}  // namespace p3a::bmdfile
