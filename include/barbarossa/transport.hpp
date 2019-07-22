#ifndef TRANSPORT_HPP__
#define TRANSPORT_HPP__

namespace barbarossa::controlchannel {

class Transport {
 public:
  virtual void Connect() = 0;

 private:
};

class WebSocketTransport : public Transport {
 public:
  virtual void Connect() override;
};

}  // namespace barbarossa::controlchannel

#include "transport.ipp"
#endif  // TRANSPORT_HPP__