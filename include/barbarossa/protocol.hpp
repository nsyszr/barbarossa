// Copyright (c) 2019 by nsyszr.io.
// Author: Tschokko

#ifndef BARBAROSSA_PROTOCOL_HPP_
#define BARBAROSSA_PROTOCOL_HPP_

#include <string>
#include <string_view>

#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace barbarossa::controlchannel::v1::protocol {

using SessionId = int32_t;
using RequestId = int32_t;
using PublicationId = int32_t;

enum MessageTypes {
  kMessageTypeHello = 1,
  kMessageTypeWelcome = 2,
  kMessageTypeAbort = 3,
  kMessageTypePing = 4,
  kMessageTypePong = 5,
  kMessageTypeError = 9,
  kMessageTypeCall = 10,
  kMessageTypeResult = 11,
  kMessageTypePublish = 20,
  kMessageTypePublished = 21,
};

class BasicMessage {
  json j_;
  friend BasicMessage Parse(const std::string_view& s);

  // Hide the constructor
  BasicMessage() {}
  explicit BasicMessage(const json& j) : j_(j) {}

 public:
  MessageTypes GetMessageType() {
    MessageTypes msg_type = j_[0];
    return msg_type;
  }

  template <typename T>
  T Get() {
    return j_.get<T>();
  }
};

inline BasicMessage Parse(const std::string_view& s) {
  auto j = json::parse(s);
  return BasicMessage{j};
}

////////////////////////////////////////////////////////////////////////////////
// HelloMessage
////////////////////////////////////////////////////////////////////////////////
namespace hellomessage {
// The representation of the websocket hello message (message type 1).
class HelloMessage {
  std::string realm_;
  json details_;

  auto tie() const { return std::tie(realm_, details_); }

  friend void to_json(json& j, const HelloMessage& msg);
  friend void from_json(const json& j, HelloMessage& msg);

 public:
  HelloMessage() : details_(json::object()) {}
  explicit HelloMessage(const std::string& realm)
      : realm_(realm), details_(json::object()) {}
  HelloMessage(const std::string& realm, const json& details)
      : realm_(realm), details_(details) {}
  template <typename T>
  HelloMessage(const std::string& realm, const T& details)
      : realm_(realm), details_(details) {}

  inline bool operator==(const HelloMessage& rhs) const {
    return tie() == rhs.tie();
  }

  // accessors
  auto realm() -> const std::string& { return realm_; }
  auto details() -> const json& { return details_; }
  template <typename T>
  auto details() const {
    return details_.get<T>();
  }

  // mutators
  void set_realm(const std::string& realm) { realm_ = realm; }
  void set_details(const json& details) { details_ = details; }
  template <typename T>
  void set_details(const T& details) {
    details_ = details;
  }
};

inline void to_json(json& j, const HelloMessage& msg) {
  j.push_back(kMessageTypeHello);
  j.push_back(msg.realm_);
  j.push_back(msg.details_);
}

inline void from_json(const json& j, HelloMessage& msg) {
  // j.at("name").get_to(p.name);
  msg.realm_ = j[1];
  msg.details_ = j[2];

  if (msg.details_.is_null()) {
    msg.details_ = json::object();
  }
}

}  // namespace hellomessage

// Returns an instance of hellomessage::HelloMessage
inline hellomessage::HelloMessage HelloMessage(const std::string realm) {
  return hellomessage::HelloMessage(realm);
}

// Returns an instance of hellomessage::HelloMessage
template <typename T>
inline hellomessage::HelloMessage HelloMessage(const std::string realm,
                                               const T& details) {
  return hellomessage::HelloMessage(realm, details);
}

////////////////////////////////////////////////////////////////////////////////
// WelcomeMessage
////////////////////////////////////////////////////////////////////////////////
namespace welcomemessage {

// The representation of the websocket welcome message (message type 2).
class WelcomeMessage {
  SessionId session_id_;
  json details_;

  auto tie() const { return std::tie(session_id_, details_); }

  friend void to_json(json& j, const WelcomeMessage& msg);
  friend void from_json(const json& j, WelcomeMessage& msg);

 public:
  WelcomeMessage() : details_(json::object()) {}
  explicit WelcomeMessage(SessionId session_id)
      : session_id_(session_id), details_(json::object()) {}
  WelcomeMessage(SessionId session_id, const json& details)
      : session_id_(session_id), details_(details) {}
  template <typename T>
  WelcomeMessage(SessionId session_id, const T& details)
      : session_id_(session_id), details_(details) {}

  inline bool operator==(const WelcomeMessage& rhs) const {
    return tie() == rhs.tie();
  }

  // accessors
  auto session_id() -> SessionId { return session_id_; }
  auto details() -> const json& { return details_; }
  template <typename T>
  auto details() const {
    return details_.get<T>();
  }

  // mutators
  void set_session_id(SessionId session_id) { session_id_ = session_id; }
  void set_details(const json& details) { details_ = details; }
  template <typename T>
  void set_details(const T& details) {
    details_ = details;
  }
};

inline void to_json(json& j, const WelcomeMessage& msg) {
  j.push_back(kMessageTypeWelcome);
  j.push_back(msg.session_id_);
  j.push_back(msg.details_);
}

inline void from_json(const json& j, WelcomeMessage& msg) {
  // j.at("name").get_to(p.name);
  msg.session_id_ = j[1];
  msg.details_ = j[2];

  if (msg.details_.is_null()) {
    msg.details_ = json::object();
  }
}

}  // namespace welcomemessage

// Returns an instance of welcomemessage::WelcomeMessage
inline welcomemessage::WelcomeMessage WelcomeMessage(SessionId session_id) {
  return welcomemessage::WelcomeMessage(session_id);
}

// Returns an instance of welcomemessage::WelcomeMessage
template <typename T>
inline welcomemessage::WelcomeMessage WelcomeMessage(SessionId session_id,
                                                     const T& details) {
  return welcomemessage::WelcomeMessage(session_id, details);
}

////////////////////////////////////////////////////////////////////////////////
// AbortMessage
////////////////////////////////////////////////////////////////////////////////
namespace abortmessage {

// The representation of the websocket abort message (message type 3).
class AbortMessage {
  std::string reason_;
  json details_;

  auto tie() const { return std::tie(reason_, details_); }

  friend void to_json(json& j, const AbortMessage& msg);
  friend void from_json(const json& j, AbortMessage& msg);

 public:
  AbortMessage() : details_(json::object()) {}
  explicit AbortMessage(const std::string& reason)
      : reason_(reason), details_(json::object()) {}
  AbortMessage(const std::string& reason, const json& details)
      : reason_(reason), details_(details) {}
  template <typename T>
  AbortMessage(const std::string& reason, const T& details)
      : reason_(reason), details_(details) {}

  inline bool operator==(const AbortMessage& rhs) const {
    return tie() == rhs.tie();
  }

  // accessors
  auto reason() -> const std::string& { return reason_; }
  auto details() -> const json& { return details_; }
  template <typename T>
  auto details() const {
    return details_.get<T>();
  }

  // mutators
  void set_reason(const std::string& reason) { reason_ = reason; }
  void set_details(const json& details) { details_ = details; }
  template <typename T>
  void set_details(const T& details) {
    details_ = details;
  }
};

inline void to_json(json& j, const AbortMessage& msg) {
  j.push_back(kMessageTypeAbort);
  j.push_back(msg.reason_);
  j.push_back(msg.details_);
}

inline void from_json(const json& j, AbortMessage& msg) {
  // j.at("name").get_to(p.name);
  msg.reason_ = j[1];
  msg.details_ = j[2];

  if (msg.details_.is_null()) {
    msg.details_ = json::object();
  }
}

}  // namespace abortmessage

// Returns an instance of abortmessage::AbortMessage
inline abortmessage::AbortMessage AbortMessage(const std::string& reason) {
  return abortmessage::AbortMessage(reason);
}

// Returns an instance of abortmessage::AbortMessage
template <typename T>
inline abortmessage::AbortMessage AbortMessage(const std::string& reason,
                                               const T& details) {
  return abortmessage::AbortMessage(reason, details);
}

////////////////////////////////////////////////////////////////////////////////
// PingMessage
////////////////////////////////////////////////////////////////////////////////
namespace pingmessage {

// The representation of the websocket ping message (message type 4).
class PingMessage {
  json details_;

  auto tie() const { return std::tie(details_); }

  friend void to_json(json& j, const PingMessage& msg);
  friend void from_json(const json& j, PingMessage& msg);

 public:
  PingMessage() : details_(json::object()) {}
  explicit PingMessage(const json& details) : details_(details) {}
  template <typename T>
  explicit PingMessage(const T& details) : details_(details) {}

  inline bool operator==(const PingMessage& rhs) const {
    return tie() == rhs.tie();
  }

  // accessors
  auto details() -> const json& { return details_; }
  template <typename T>
  auto details() const {
    return details_.get<T>();
  }

  // mutators
  void set_details(const json& details) { details_ = details; }
  template <typename T>
  void set_details(const T& details) {
    details_ = details;
  }
};

inline void to_json(json& j, const PingMessage& msg) {
  j.push_back(kMessageTypePing);
  j.push_back(msg.details_);
}

inline void from_json(const json& j, PingMessage& msg) {
  // j.at("name").get_to(p.name);
  msg.details_ = j[1];

  if (msg.details_.is_null()) {
    msg.details_ = json::object();
  }
}

}  // namespace pingmessage

// Returns an instance of pingmessage::PingMessage
inline pingmessage::PingMessage PingMessage() {
  return pingmessage::PingMessage();
}

// Returns an instance of pingmessage::PingMessage
template <typename T>
inline pingmessage::PingMessage PingMessage(const T& details) {
  return pingmessage::PingMessage(details);
}

////////////////////////////////////////////////////////////////////////////////
// PingMessage
////////////////////////////////////////////////////////////////////////////////
namespace pongmessage {

// The representation of the websocket pong message (message type 5).
class PongMessage {
  json details_;

  auto tie() const { return std::tie(details_); }

  friend void to_json(json& j, const PongMessage& msg);
  friend void from_json(const json& j, PongMessage& msg);

 public:
  PongMessage() : details_(json::object()) {}
  explicit PongMessage(const json& details) : details_(details) {}
  template <typename T>
  explicit PongMessage(const T& details) : details_(details) {}

  inline bool operator==(const PongMessage& rhs) const {
    return tie() == rhs.tie();
  }

  // accessors
  auto details() -> const json& { return details_; }
  template <typename T>
  auto details() const {
    return details_.get<T>();
  }

  // mutators
  void set_details(const json& details) { details_ = details; }
  template <typename T>
  void set_details(const T& details) {
    details_ = details;
  }
};

inline void to_json(json& j, const PongMessage& msg) {
  j.push_back(kMessageTypePing);
  j.push_back(msg.details_);
}

inline void from_json(const json& j, PongMessage& msg) {
  // j.at("name").get_to(p.name);
  msg.details_ = j[1];

  if (msg.details_.is_null()) {
    msg.details_ = json::object();
  }
}

}  // namespace pongmessage

// Returns an instance of pongmessage::PongMessage
inline pongmessage::PongMessage PongMessage() {
  return pongmessage::PongMessage();
}

// Returns an instance of pongmessage::PongMessage
template <typename T>
inline pongmessage::PongMessage PongMessage(const T& details) {
  return pongmessage::PongMessage(details);
}

////////////////////////////////////////////////////////////////////////////////
// ErrorMessage
////////////////////////////////////////////////////////////////////////////////
namespace errormessage {

// The representation of the websocket error message (message type 9).
class ErrorMessage {
  MessageTypes request_type_;
  RequestId request_id_;
  std::string error_;
  json details_;

  auto tie() const {
    return std::tie(request_type_, request_id_, error_, details_);
  }

  friend void to_json(json& j, const ErrorMessage& msg);
  friend void from_json(const json& j, ErrorMessage& msg);

 public:
  ErrorMessage() : details_(json::object()) {}
  ErrorMessage(MessageTypes request_type, RequestId request_id,
               const std::string& error)
      : request_type_(request_type),
        request_id_(request_id),
        error_(error),
        details_(json::object()) {}
  ErrorMessage(MessageTypes request_type, RequestId request_id,
               const std::string& error, const json& details)
      : request_type_(request_type),
        request_id_(request_id),
        error_(error),
        details_(details) {}
  template <typename T>
  ErrorMessage(MessageTypes request_type, RequestId request_id,
               const std::string& error, const T& details)
      : request_type_(request_type),
        request_id_(request_id),
        error_(error),
        details_(details) {}

  inline bool operator==(const ErrorMessage& rhs) const {
    return tie() == rhs.tie();
  }

  // accessors
  auto request_type() -> MessageTypes { return request_type_; }
  auto request_id() -> RequestId { return request_id_; }
  auto error() -> const std::string& { return error_; }
  auto details() -> const json& { return details_; }
  template <typename T>
  auto details() const {
    return details_.get<T>();
  }

  // mutators
  void set_request_type(MessageTypes request_type) {
    request_type_ = request_type;
  }
  void set_request_id(RequestId request_id) { request_id_ = request_id; }
  void set_error(const std::string& error) { error_ = error; }
  void set_details(const json& details) { details_ = details; }
  template <typename T>
  void set_details(const T& details) {
    details_ = details;
  }
};

inline void to_json(json& j, const ErrorMessage& msg) {
  j.push_back(kMessageTypeError);
  j.push_back(msg.request_type_);
  j.push_back(msg.request_id_);
  j.push_back(msg.error_);
  j.push_back(msg.details_);
}

inline void from_json(const json& j, ErrorMessage& msg) {
  // j.at("name").get_to(p.name);
  msg.request_type_ = j[1];
  msg.request_id_ = j[2];
  msg.error_ = j[3];
  msg.details_ = j[4];

  if (msg.details_.is_null()) {
    msg.details_ = json::object();
  }
}

}  // namespace errormessage

// Returns an instance of errormessage::ErrorMessage
inline errormessage::ErrorMessage ErrorMessage(MessageTypes request_type,
                                               RequestId request_id,
                                               const std::string& error) {
  return errormessage::ErrorMessage(request_type, request_id, error);
}

// Returns an instance of errormessage::ErrorMessage
template <typename T>
inline errormessage::ErrorMessage ErrorMessage(MessageTypes request_type,
                                               RequestId request_id,
                                               const std::string& error,
                                               const T& details) {
  return errormessage::ErrorMessage(request_type, request_id, error, details);
}

}  // namespace barbarossa::controlchannel::v1::protocol

#endif  // BARBAROSSA_PROTOCOL_HPP_
