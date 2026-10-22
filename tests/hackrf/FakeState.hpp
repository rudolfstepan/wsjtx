#pragma once
#include <atomic>
#include <cstdint>
struct FakeHackrfState {
 std::atomic<int> initialized{0},opens{0},closes{0},mode{0},rxStarts{0},txStarts{0},rxStops{0},txStops{0},overlap{0};
 std::atomic<int> amp{-1},bias{-1},rate{0},filter{0},lna{0},vga{0},gain{0},failTune{0},devices{1};
 std::atomic<uint64_t> frequency{0},txBytes{0};
};
