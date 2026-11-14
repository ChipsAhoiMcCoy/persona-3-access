cmake_minimum_required(VERSION 3.24)
project(arcana LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT MSVC)
  message(FATAL_ERROR "arcana must be built with MSVC (it shares the game's MSVC x64 ABI)")
endif()
# Static CRT: a player needs no VC++ redistributable for the mod (prism.dll carries its own runtime).
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
add_compile_definitions(WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS UNICODE _UNICODE)
add_compile_options(/utf-8 /W4 /permissive- /Zc:__cplusplus /EHsc)
# tools/package.py: the same sources give the same binaries (no build stamps)
option(P3A_REPRODUCIBLE "Deterministic compiler and linker output" OFF)
if(P3A_REPRODUCIBLE)
  add_compile_options(/Brepro)
  add_link_options(/Brepro)
endif()

# --- Microsoft Detours (vendored source) ---
set(DETOURS_SRC ${CMAKE_SOURCE_DIR}/third_party/Detours/src)
add_library(detours STATIC
  ${DETOURS_SRC}/detours.cpp ${DETOURS_SRC}/modules.cpp ${DETOURS_SRC}/disasm.cpp
  ${DETOURS_SRC}/image.cpp ${DETOURS_SRC}/creatwth.cpp)
target_include_directories(detours PUBLIC ${DETOURS_SRC})
target_compile_options(detours PRIVATE /W0)

# --- Prism (prebuilt SDK release) ---
set(PRISM_SDK ${CMAKE_SOURCE_DIR}/third_party/prism-bin/prism-sdk-v0.18.1)
set(PRISM_BIN ${PRISM_SDK}/windows/x64/dynamic/release)
set(PRISM_LIB ${PRISM_BIN}/lib/prism.lib)
set(PRISM_DLL ${PRISM_BIN}/bin/prism.dll)
get_filename_component(PRISM_DLL_NAME ${PRISM_DLL} NAME)

# --- Steam Audio (prebuilt C SDK, Apache-2.0): HRTF for the positional cues (src/hrtf.cpp) ---
set(STEAMAUDIO_SDK ${CMAKE_SOURCE_DIR}/third_party/steamaudio/steamaudio-4.8.1)
set(PHONON_LIB ${STEAMAUDIO_SDK}/lib/windows-x64/phonon.lib)
set(PHONON_DLL ${STEAMAUDIO_SDK}/lib/windows-x64/phonon.dll)

# --- engine-free core (unit-tested, no Windows or game headers) ---
file(GLOB P3CORE_SOURCES CONFIGURE_DEPENDS src/core/*.cpp)
add_library(p3core STATIC ${P3CORE_SOURCES})
target_include_directories(p3core PUBLIC src)

file(GLOB P3CORE_TESTS CONFIGURE_DEPENDS tests/*.cpp)
add_executable(p3core_tests ${P3CORE_TESTS})
target_include_directories(p3core_tests PRIVATE third_party/doctest)
target_link_libraries(p3core_tests PRIVATE p3core)

# --- the mod DLL ---
file(GLOB P3A_SOURCES CONFIGURE_DEPENDS src/*.cpp src/ue/*.cpp src/screens/*.cpp)
add_library(arcana SHARED ${P3A_SOURCES})
target_include_directories(arcana PRIVATE src ${PRISM_SDK}/include ${STEAMAUDIO_SDK}/include third_party/miniaudio)
target_link_libraries(arcana PRIVATE p3core detours ${PRISM_LIB} ${PHONON_LIB} user32 ws2_32 ole32 delayimp gdi32)
# phonon.dll delay-loaded too: without it (or if it fails) the cues fall back to stereo panning.
target_link_options(arcana PRIVATE /DELAYLOAD:${PRISM_DLL_NAME} /DELAYLOAD:phonon.dll /MAP)   # arcana.map: crash-log offsets -> functions
add_custom_command(TARGET arcana POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PRISM_DLL} $<TARGET_FILE_DIR:arcana>
  COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PHONON_DLL} $<TARGET_FILE_DIR:arcana>)
# assets/ (wall tones, the audio description clips) on every build, not only when the DLL relinks: a re-pack of
# assets/ad with no code change must reach the build too
add_custom_target(arcana_assets ALL
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:arcana>/assets)

# --- dsound.dll proxy: dropped into P3R\Binaries\Win64, loads the mod before the game's code runs ---
add_library(dsound_proxy SHARED src/proxy/dsound_proxy.cpp src/proxy/dsound.def)
set_target_properties(dsound_proxy PROPERTIES OUTPUT_NAME dsound)
