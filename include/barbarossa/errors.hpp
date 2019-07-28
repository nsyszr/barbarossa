// Copyright (c) 2019 by nsyszr.io.
// Author: Tschokko

#ifndef BARBAROSSA_ERRORS_HPP_
#define BARBAROSSA_ERRORS_HPP_

#include <stdexcept>
#include <string>


namespace barbarossa::controlchannel {

struct ErrorMessages {
  constexpr static char kTransportConnectionFailed[] =
      "transport failed to connect";
  constexpr static char kTransportAlreadyConnected[] =
      "transport already connected";
  constexpr static char kTransportAlreadyDisconnected[] =
      "transport already disconnected";

  constexpr static char kTransportHandlerAlreadyAttached[] =
      "transport handler already attached";
  constexpr static char kTransportHandlerAlreadyDetached[] =
      "transport handler already detached";

  constexpr static char kSessionAlreadyJoined[] = "session already joined";
  constexpr static char kSessionNotJoined[] = "session not joined";
  constexpr static char kSessionNotAttached[] = "session not attached";
  constexpr static char kSessionTransportAlreadyDetached[] =
      "session transport already detached";
  constexpr static char kSessionTransportAlreadyAttached[] =
      "session transport already attached";

  constexpr static char kInvalidMessageStructure[] =
      "invalid message structure";
  constexpr static char kInvalidMessageType[] = "invalid message type";
};

class NetworkError : public std::runtime_error {
 public:
  explicit NetworkError(const std::string& message)
      : std::runtime_error(message), message_(message) {}

  const char* what() const throw() {
    return std::string("network error: " + message_).c_str();
  }

 private:
  std::string message_;
};

class ProtocolError : public std::runtime_error {
 public:
  explicit ProtocolError(const std::string& message)
      : std::runtime_error(message), message_(message) {}

  const char* what() const throw() {
    return std::string("protocol error: " + message_).c_str();
  }

 private:
  std::string message_;
};

class NoSessionError : public ProtocolError {
 public:
  NoSessionError() : ProtocolError(ErrorMessages::kSessionNotJoined) {}
};

class NoTransportError : public NetworkError {
 public:
  NoTransportError() : NetworkError(ErrorMessages::kSessionNotAttached) {}
};

class TimeoutError : public NetworkError {
 public:
  TimeoutError() : NetworkError("timeout") {}
};

}  // namespace barbarossa::controlchannel

#endif  // BARBAROSSA_ERRORS_HPP_
