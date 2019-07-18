#ifndef WEBSOCKET_ENDPOINT_HPP_
#define WEBSOCKET_ENDPOINT_HPP_


#include "barbarossa/control_channel.hpp"
#include "websocketpp/client.hpp"
// #include "websocketpp/common/memory.hpp"
// #include "websocketpp/common/thread.hpp"
#define ASIO_STANDALONE
#include "websocketpp/config/asio_no_tls_client.hpp"

namespace barbarossa::controlchannel::v1 {

class WebsocketEndpoint {
 public:
  typedef websocketpp::client<websocketpp::config::asio_client> client;
  typedef websocketpp::lib::lock_guard<websocketpp::lib::mutex> scoped_lock;
  typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

  WebsocketEndpoint(ControlChannel& control_channel);
  ~WebsocketEndpoint();

  bool Connect(const std::string& uri);
  bool SendMessage(const std::string& data);
  bool Close();

  // accessors
  bool initialized() { return initialized_; }

 private:
  bool initialized_;
  bool connected_;
  ControlChannel& control_channel_;
  client endpoint_;
  websocketpp::lib::mutex lock_;
  websocketpp::connection_hdl hdl_;
  websocketpp::lib::shared_ptr<websocketpp::lib::thread> thread_;

  void Init();
  void OnOpen(websocketpp::connection_hdl);
  void OnClose(websocketpp::connection_hdl);
  void OnFail(websocketpp::connection_hdl);
  void OnMessage(websocketpp::connection_hdl hdl, message_ptr msg);
};

}  // namespace barbarossa::controlchannel::v1

#endif  // WEBSOCKET_ENDPOINT_HPP_