// Copyright (c) 2018 by nsyszr.io.
// Author: dgl

#ifndef PROTOCOL_HPP_
#define PROTOCOL_HPP_

#include <string>
#include <string_view>

#include "nlohmann/json.hpp"

using json = nlohmann::json;

namespace barbarossa::controlchannel::v1::protocol {

enum MessageTypes {
  kMessageTypeInvalid = 0,
  kMessageTypeHello = 1,
  kMessageTypeWelcome = 2,
  kMessageTypeAbort = 3,
  kMessageTypePing = 4,
  kMessageTypePong = 5,
};

class BasicMessage {
  json j_;
  friend BasicMessage Parse(const std::string_view& s);

  // Hide the constructor
  BasicMessage() {}
  BasicMessage(const json& j) : j_(j) {}

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

BasicMessage Parse(const std::string_view& s) {
  auto j = json::parse(s);
  return BasicMessage{j};
}

namespace hellomessage {
// The representation of the websocket hello message (message type 1).
class HelloMessage {
  std::string realm_;
  json details_;

  auto tie() const { return std::tie(realm_, details_); }

  friend void to_json(json& j, const HelloMessage& msg);
  friend void from_json(const json& j, HelloMessage& msg);

 public:
  HelloMessage() {}
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

void to_json(json& j, const HelloMessage& msg) {
  j.push_back(kMessageTypeHello);
  j.push_back(msg.realm_);
  j.push_back(msg.details_);
}

void from_json(const json& j, HelloMessage& msg) {
  // j.at("name").get_to(p.name);
  msg.realm_ = j[1];
  msg.details_ = j[2];

  if (msg.details_.is_null()) {
    msg.details_ = json::object();
  }
}

}  // namespace hellomessage

// Returns an instance of hellomessage::HelloMessage with given realm.
hellomessage::HelloMessage HelloMessage(const std::string realm) {
  return hellomessage::HelloMessage(realm);
}

// Returns an instance of hellomessage::HelloMessage with given realm and
// details.
template <typename T>
hellomessage::HelloMessage HelloMessage(const std::string realm,
                                        const T& details) {
  return hellomessage::HelloMessage(realm, details);
}

namespace welcomemessage {

// The representation of the websocket welcome message (message type 2).
class WelcomeMessage {
  int32_t session_id_;
  json details_;

  auto tie() const { return std::tie(session_id_, details_); }

  friend void to_json(json& j, const WelcomeMessage& msg);
  friend void from_json(const json& j, WelcomeMessage& msg);

 public:
  WelcomeMessage() {}
  explicit WelcomeMessage(int32_t session_id)
      : session_id_(session_id), details_(json::object()) {}
  WelcomeMessage(int32_t session_id, const json& details)
      : session_id_(session_id), details_(details) {}
  template <typename T>
  WelcomeMessage(int32_t session_id, const T& details)
      : session_id_(session_id), details_(details) {}

  inline bool operator==(const WelcomeMessage& rhs) const {
    return tie() == rhs.tie();
  }

  // accessors
  auto session_id() -> int32_t { return session_id_; }
  auto details() -> const json& { return details_; }
  template <typename T>
  auto details() const {
    return details_.get<T>();
  }

  // mutators
  void set_session_id(int32_t session_id) { session_id_ = session_id; }
  void set_details(const json& details) { details_ = details; }
  template <typename T>
  void set_details(const T& details) {
    details_ = details;
  }
};

void to_json(json& j, const WelcomeMessage& msg) {
  j.push_back(kMessageTypeWelcome);
  j.push_back(msg.session_id_);
  j.push_back(msg.details_);
}

void from_json(const json& j, WelcomeMessage& msg) {
  // j.at("name").get_to(p.name);
  msg.session_id_ = j[1];
  msg.details_ = j[2];

  if (msg.details_.is_null()) {
    msg.details_ = json::object();
  }
}

}  // namespace welcomemessage

// Returns an instance of welcomemessage::WelcomeMessage with given session_id.
welcomemessage::WelcomeMessage WelcomeMessage(int32_t session_id) {
  return welcomemessage::WelcomeMessage(session_id);
}

// Returns an instance of welcomemessage::WelcomeMessage with given session_id
// and details.
template <typename T>
welcomemessage::WelcomeMessage WelcomeMessage(int32_t session_id,
                                              const T& details) {
  return welcomemessage::WelcomeMessage(session_id, details);
}

namespace abortmessage {

// The representation of the websocket abort message (message type 3).
class AbortMessage {
  std::string reason_;
  json details_;

  auto tie() const { return std::tie(reason_, details_); }

  friend void to_json(json& j, const AbortMessage& msg);
  friend void from_json(const json& j, AbortMessage& msg);

 public:
  AbortMessage() {}
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

void to_json(json& j, const AbortMessage& msg) {
  j.push_back(kMessageTypeAbort);
  j.push_back(msg.reason_);
  j.push_back(msg.details_);
}

void from_json(const json& j, AbortMessage& msg) {
  // j.at("name").get_to(p.name);
  msg.reason_ = j[1];
  msg.details_ = j[2];

  if (msg.details_.is_null()) {
    msg.details_ = json::object();
  }
}

}  // namespace abortmessage

namespace pingmessage {

// The representation of the websocket ping message (message type 4).
class PingMessage {
  json details_;

  auto tie() const { return std::tie(details_); }

  friend void to_json(json& j, const PingMessage& msg);
  friend void from_json(const json& j, PingMessage& msg);

 public:
  PingMessage() {}
  PingMessage(const json& details) : details_(details) {}
  template <typename T>
  PingMessage(const T& details) : details_(details) {}

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

void to_json(json& j, const PingMessage& msg) {
  j.push_back(kMessageTypePing);
  j.push_back(msg.details_);
}

void from_json(const json& j, PingMessage& msg) {
  // j.at("name").get_to(p.name);
  msg.details_ = j[1];

  if (msg.details_.is_null()) {
    msg.details_ = json::object();
  }
}

}  // namespace pingmessage

// Returns an instance of abortmessage::PingMessage with given reason.
pingmessage::PingMessage PingMessage() { return pingmessage::PingMessage(); }

// Returns an instance of abortmessage::PingMessage with given reason and
// details.
template <typename T>
pingmessage::PingMessage PingMessage(const T& details) {
  return pingmessage::PingMessage(details);
}

namespace pongmessage {

// The representation of the websocket ping message (message type 4).
class PongMessage {
  json details_;

  auto tie() const { return std::tie(details_); }

  friend void to_json(json& j, const PongMessage& msg);
  friend void from_json(const json& j, PongMessage& msg);

 public:
  PongMessage() {}
  PongMessage(const json& details) : details_(details) {}
  template <typename T>
  PongMessage(const T& details) : details_(details) {}

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

void to_json(json& j, const PongMessage& msg) {
  j.push_back(kMessageTypePing);
  j.push_back(msg.details_);
}

void from_json(const json& j, PongMessage& msg) {
  // j.at("name").get_to(p.name);
  msg.details_ = j[1];

  if (msg.details_.is_null()) {
    msg.details_ = json::object();
  }
}

}  // namespace pongmessage

// Returns an instance of abortmessage::PingMessage with given reason.
pongmessage::PongMessage PongMessage() { return pongmessage::PongMessage(); }

// Returns an instance of abortmessage::PingMessage with given reason and
// details.
template <typename T>
pongmessage::PongMessage PongMessage(const T& details) {
  return pongmessage::PongMessage(details);
}

}  // namespace barbarossa::controlchannel::v1::protocol

#endif  // PROTOCOL_HPP_