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

std::string ToString(MessageType type);

}  // namespace barbarossa::controlchannel

#include "barbarossa/message_type.ipp"
#endif  // BARBAROSSA_MESSAGE_TYPE_HPP_
