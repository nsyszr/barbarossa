// Copyright (c) 2019 by nsyszr.io.
// Author: Tschokko

#include <csignal>

#include "spdlog/spdlog.h"
#include "zmq.hpp"

namespace barbarossa {

// We use only one global shared context for inproc communication
zmq::context_t gInProcContext(1);

//
// Signal handling
//
volatile std::sig_atomic_t gQuitSignal;

void SignalHandler(int signal) {
  spdlog::debug("Signal handler called: {}", signal);

  gQuitSignal = signal;

  // Close the inproc_context leads to an exception in all listening threads.
  // Since we set the quit_signal above the threads stops executing.
  gInProcContext.close();
}

bool InstallSignalHandler() {
  try {
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);
  } catch (...) {
    return false;
  }
  return true;
}

}  // namespace barbarossa
