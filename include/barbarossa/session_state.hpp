// Copyright (c) 2019 by nsyszr.io.
// Author: Tschokko

#ifndef BARBAROSSA_SESSION_STATE_HPP_
#define BARBAROSSA_SESSION_STATE_HPP_

namespace barbarossa::controlchannel {

enum class SessionState : int {
  kClosed = 0,
  kEstablishing,
  kEstablished,
  kClosing,
  kFailed,
};

}  // namespace barbarossa::controlchannel

#endif  // BARBAROSSA_SESSION_STATE_HPP_
