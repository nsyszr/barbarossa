#ifndef GLOBALS_HPP_
#define GLOBALS_HPP_

#include <csignal>

#include "zmq.hpp"

namespace barbarossa {

extern volatile std::sig_atomic_t gQuitSignal;

extern zmq::context_t gInProcContext;
bool InstallSignalHandler();

}  // namespace barbarossa

#endif  // GLOBALS_HPP_