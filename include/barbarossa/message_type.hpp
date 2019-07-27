// Copyright (c) 2019 by nsyszr.io.
// Author: Tschokko
//
// Following code is rewritten based on the following Autobahn proven C++ WAMP
// implementation: https://github.com/crossbario/autobahn-cpp
// The control channel protocol is subset and slightly modified WAMP protocol
// optimized for our purporses. Therefore we cannot use a WAMP compliant client
// implementation.

#ifndef BARBAROSSA_MESSAGE_TYPE_HPP_
#define BARBAROSSA_MESSAGE_TYPE_HPP_

#include <string>

#include "spdlog/fmt/ostr.h"

namespace barbarossa::controlchannel {

enum class MessageType : int {
  kHello = 1,
  kWelcome = 2,
  kAbort = 3,
  kPing = 4,
  kPong = 5,
  kError = 9,
  kCall = 10,
  kResult = 11,
  kPublish = 20,
  kPublished = 21,
};

// Custom types needs the operator implementation for proper logging (spdlog).
template <typename OSTREAM>
OSTREAM& operator<<(OSTREAM& os, const MessageType& state) {
  switch (state) {
    case MessageType::kHello:
      return os << "HELLO";
    case MessageType::kWelcome:
      return os << "WELCOME";
    case MessageType::kAbort:
      return os << "ABORT";
    case MessageType::kPing:
      return os << "PING";
    case MessageType::kPong:
      return os << "PONG";
    case MessageType::kError:
      return os << "ERROR";
    case MessageType::kCall:
      return os << "CALL";
    case MessageType::kResult:
      return os << "RESULT";
    case MessageType::kPublish:
      return os << "PUBLISH";
    case MessageType::kPublished:
      return os << "PUBLISHED";
  }
  return os << "UNKNOWN";
}

}  // namespace barbarossa::controlchannel

#endif  // BARBAROSSA_MESSAGE_TYPE_HPP_
