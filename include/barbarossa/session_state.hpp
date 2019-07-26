// Copyright (c) 2019 by nsyszr.io.
// Author: Tschokko

#ifndef BARBAROSSA_SESSION_STATE_HPP_
#define BARBAROSSA_SESSION_STATE_HPP_

#include <string>

namespace barbarossa::controlchannel {

enum class SessionState : int {
  kClosed = 0,
  kEstablishing,
  kEstablished,
  kClosing,
};

std::string ToString(SessionState state);

}  // namespace barbarossa::controlchannel

#include "barbarossa/session_state.ipp"
#endif  // BARBAROSSA_SESSION_STATE_HPP_
