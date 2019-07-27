// Copyright (c) 2019 by nsyszr.io.
// Author: Tschokko

#ifndef BARBAROSSA_SESSION_STATE_HPP_
#define BARBAROSSA_SESSION_STATE_HPP_

#include "spdlog/fmt/ostr.h"

namespace barbarossa::controlchannel {

enum class SessionState : int {
  kClosed = 0,
  kEstablishing,
  kEstablished,
  kClosing,
};

// Custom types needs the operator implementation for proper logging (spdlog).
template <typename OSTREAM>
OSTREAM& operator<<(OSTREAM& os, const SessionState& state) {
  switch (state) {
    case SessionState::kClosed:
      return os << "CLOSED";
    case SessionState::kEstablishing:
      return os << "ESTABLISHING";
    case SessionState::kEstablished:
      return os << "ESTABLISHED";
    case SessionState::kClosing:
      return os << "CLOSING";
  }
  return os << "UNKNOWN";
}

}  // namespace barbarossa::controlchannel

#endif  // BARBAROSSA_SESSION_STATE_HPP_
