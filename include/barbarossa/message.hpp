#ifndef MESSAGE_HPP_
#define MESSAGE_HPP_

#include <cstdint>
#include <vector>

#include "nlohmann/json.hpp"

namespace barbarossa::controlchannel {

using json = nlohmann::json;

class Message {
 public:
  using MessageFields = std::vector<json>;

  explicit Message(std::size_t num_fields);
  explicit Message(MessageFields&& fields);

  Message& operator=(const Message& other) = delete;
  Message& operator=(Message&& other);

  const json& Field(std::size_t index) const;
  template <typename Type>
  Type Field(std::size_t index);

  template <typename Type>
  void SetField(std::size_t index, const Type& type);

  std::size_t Size() const;

  const MessageFields& Fields() const;
  MessageFields&& Fields();

 private:
  MessageFields fields_;
};

}  // namespace barbarossa::controlchannel

#include "barbarossa/message.ipp"
#endif  // MESSAGE_HPP_