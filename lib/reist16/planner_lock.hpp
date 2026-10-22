#ifndef REIST16_PLANNER_LOCK_HPP
#define REIST16_PLANNER_LOCK_HPP
#include <mutex>
namespace reist16 {
inline std::mutex& planner_mutex(){static std::mutex value;return value;}
}
#endif
