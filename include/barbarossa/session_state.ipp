namespace barbarossa::controlchannel {

#include <map>
#include <string>

inline std::string ToString(SessionState state) {
  static std::map<SessionState, std::string> state_names = {
      {SessionState::kClosed, "HELLO"},
      {SessionState::kEstablishing, "WELCOME"},
      {SessionState::kEstablished, "ABORT"},
      {SessionState::kClosing, "PING"}};

  auto result = state_names.find(state);
  if (result == state_names.end()) {
    return std::string("UNKNOWN");
  }

  return result->second;
}

}