#ifndef BARBAROSSA_WEBSOCKET_TRANSPORT_HPP_
#define BARBAROSSA_WEBSOCKET_TRANSPORT_HPP_

#include <future>
#include <string>

#include "barbarossa/message.hpp"
#include "barbarossa/transport.hpp"
#include "websocketpp/client.hpp"
#define ASIO_STANDALONE
#include "websocketpp/config/asio_no_tls_client.hpp"

namespace barbarossa::controlchannel {

class WebSocketTransport : public Transport {
 public:
  typedef websocketpp::client<websocketpp::config::asio_client> client;
  typedef websocketpp::lib::lock_guard<websocketpp::lib::mutex> scoped_lock;
  typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

  explicit WebSocketTransport(const std::string& uri);

  void Connect() override;
  void Disconnect() override;
  bool IsConnected() const override;
  void SendMessage(Message&& message) override;
  bool HasHandler() const override;

 private:
  std::string uri_;
  bool open_;
  bool done_;
  client endpoint_;
  websocketpp::lib::mutex lock_;
  websocketpp::connection_hdl hdl_;
  // websocketpp::lib::shared_ptr<websocketpp::lib::thread> thread_;
  std::promise<void> connected_;
};

}  // namespace barbarossa::controlchannel

#include "barbarossa/websocket_transport.ipp"
#endif  // BARBAROSSA_WEBSOCKET_TRANSPORT_HPP_
