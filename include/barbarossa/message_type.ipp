// Copyright (c) 2019 by nsyszr.io.
// Author: Tschokko
//
// Following code is rewritten based on the following Autobahn proven C++ WAMP
// implementation: https://github.com/crossbario/autobahn-cpp
// The control channel protocol is subset and slightly modified WAMP protocol
// optimized for our purporses. Therefore we cannot use a WAMP compliant client
// implementation.

#include <map>
#include <string>

namespace barbarossa::controlchannel {

inline std::string ToString(MessageType type) {
  static std::map<MessageType, std::string> type_names = {
      {MessageType::kHello, "HELLO"},
      {MessageType::kWelcome, "WELCOME"},
      {MessageType::kAbort, "ABORT"},
      {MessageType::kPing, "PING"},
      {MessageType::kPong, "PONG"},
      {MessageType::kError, "ERROR"},
      {MessageType::kCall, "CALL"},
      {MessageType::kResult, "RESULT"},
      {MessageType::kPublish, "PUBLISH"},
      {MessageType::kPublished, "PUBLISHED"},
  };

  auto result = type_names.find(type);
  if (result == type_names.end()) {
    return std::string("UNKNOWN");
  }

  return result->second;
}

}  // namespace barbarossa::controlchannel