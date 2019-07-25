// Copyright (c) 2019 by nsyszr.io.
// Author: Tschokko

#ifndef BARBAROSSA_GLOBALS_HPP_
#define BARBAROSSA_GLOBALS_HPP_

#include <csignal>

#include "zmq.hpp"

namespace barbarossa {

extern volatile std::sig_atomic_t gQuitSignal;

extern zmq::context_t gInProcContext;
bool InstallSignalHandler();

}  // namespace barbarossa

#endif  // BARBAROSSA_GLOBALS_HPP_
