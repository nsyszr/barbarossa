#ifndef MESSAGE_HPP_
#define MESSAGE_HPP_

template <typename T>
class Message {
 public:
  using MessageFields = std::vector<T>;

  virtual ~Message() = default;
  Message(std::size_t num_fields) = 0;
  Message(MessageFields&& fields) = 0;

  Message& operator=(const Message& other) = delete;
  Message& operator=(Message&& other) = 0;

  virtual const T& Field(std::size_t index) const = 0;
  template <typename Type>
  Type Field(std::size_t index);

  template <typename Type>
  void SetField(std::size_t index, const Type& type) = 0;

  std::size_t Size() const = 0;

  const MessageFields& Fields() const = 0;
  MessageFields&& Fields() = 0;
};

#endif  // MESSAGE_HPP_