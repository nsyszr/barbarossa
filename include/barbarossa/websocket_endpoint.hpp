#ifndef WEBSOCKET_ENDPOINT_HPP_
#define WEBSOCKET_ENDPOINT_HPP_

#include <functional>
#include <string>

#include "websocketpp/client.hpp"
// #include "websocketpp/common/memory.hpp"
// #include "websocketpp/common/thread.hpp"
#define ASIO_STANDALONE
#include "websocketpp/config/asio_client.hpp"
// #include "websocketpp/config/asio_no_tls_client.hpp"

namespace barbarossa::controlchannel::v1 {

class WebsocketEndpoint {
 public:
  typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
  typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context>
      context_ptr;
  typedef websocketpp::lib::lock_guard<websocketpp::lib::mutex> scoped_lock;
  typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

  WebsocketEndpoint(const std::string& uri);
  ~WebsocketEndpoint();

  bool Connect();
  bool Send(const std::string& data);
  bool Close();

  // accessors
  // Commented out because this accessors aren't needed.
  /*const std::string& uri() { return uri_; }
  bool initialized() { return initialized_; }
  bool connected() { return connected_; }*/

  // mutators
  void set_on_open_handler(std::function<void()> on_open_handler) {
    on_open_handler_ = on_open_handler;
  }
  void set_on_close_handler(std::function<void()> on_close_handler) {
    on_close_handler_ = on_close_handler;
  }
  void set_on_fail_handler(std::function<void()> on_fail_handler) {
    on_fail_handler_ = on_fail_handler;
  }
  void set_on_message_handler(
      std::function<void(const std::string&)> on_message_handler) {
    on_message_handler_ = on_message_handler;
  }

 private:
  // Keep the ctor order
  std::string uri_;
  bool initialized_;
  bool connected_;

  client endpoint_;
  websocketpp::lib::mutex lock_;
  websocketpp::connection_hdl hdl_;
  websocketpp::lib::shared_ptr<websocketpp::lib::thread> thread_;
  std::function<void()> on_open_handler_;
  std::function<void()> on_close_handler_;
  std::function<void()> on_fail_handler_;
  std::function<void(const std::string&)> on_message_handler_;

  void Init();
  void OnOpen(websocketpp::connection_hdl);
  void OnClose(websocketpp::connection_hdl);
  void OnFail(websocketpp::connection_hdl);
  void OnMessage(websocketpp::connection_hdl, message_ptr msgp);
  context_ptr OnTLSInit(/*const char* hostname, websocketpp::connection_hdl*/);
};

}  // namespace barbarossa::controlchannel::v1

#endif  // WEBSOCKET_ENDPOINT_HPP_
