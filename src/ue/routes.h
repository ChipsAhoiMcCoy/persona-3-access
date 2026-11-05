#pragma once
namespace p3a::ue {
void register_routes();   // /ue/* dev routes (the live object browser)
void probe_tick();        // game thread: the /ue/probe key probe
}
