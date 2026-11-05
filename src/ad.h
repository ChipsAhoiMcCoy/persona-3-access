#pragma once
#include <string>

namespace p3a::ad {
// Audio description of the animated story movies (src/ad.cpp): the movie clock, and the narrator that
// follows it. Game thread.
struct Clock {
  bool playing = false;
  bool paused = false;
  std::string movie;   // the UManaMovie source's name, e.g. MS_Event_Main_100_030_M_Movi_VP9
  double time = 0;     // GetTime: seconds into the movie
  int frame = -1;      // GetFrameNumber
  int audio_track = -1;   // 0 Japanese voices, 1 English
};
Clock clock();
void tick();                                   // every frame: follow the movie, play its descriptions
std::string selftest(const std::string& filter);   // decode every packed clip and check its length (any thread)
void register_routes();
}  // namespace p3a::ad
