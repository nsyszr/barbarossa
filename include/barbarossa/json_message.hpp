#ifndef JSON_MESSAGE_HPP_
#define JSON_MESSAGE_HPP_

#include "barbarossa/message.hpp"
#include "nlohmann/json.hpp"

using json = nlohmann::json;

class JsonMessage : public Message<json> {
 public:
  Message(std::size_t num_fields);
  Message(MessageFields&& fields);

  JsonMessage& operator=(JsonMessage&& other) override;

  const T& Field(std::size_t index) const override;
  template <typename Type>
  Type Field(std::size_t index) override;

  template <typename Type>
  void SetField(std::size_t index, const Type& type) override;

  std::size_t Size() const override;

  const MessageFields& Fields() const override;
  MessageFields&& Fields() override;
};

#endif  // JSON_MESSAGE_HPP_