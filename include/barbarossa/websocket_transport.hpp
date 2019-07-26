// Copyright (c) 2019 by nsyszr.io.
// Author: Tschokko
//
// Following code is rewritten based on the following Autobahn proven C++ WAMP
// implementation: https://github.com/crossbario/autobahn-cpp
// The control channel protocol is subset and slightly modified WAMP protocol
// optimized for our purporses. Therefore we cannot use a WAMP compliant client
// implementation.

#ifndef BARBAROSSA_WEBSOCKET_TRANSPORT_HPP_
#define BARBAROSSA_WEBSOCKET_TRANSPORT_HPP_

#include <future>
#include <memory>
#include <string>

#include "barbarossa/message.hpp"
#include "barbarossa/transport.hpp"
#include "barbarossa/transport_handler.hpp"
#include "websocketpp/client.hpp"
#define ASIO_STANDALONE
#include "websocketpp/config/asio_no_tls_client.hpp"

namespace barbarossa::controlchannel {

template <typename CONFIG>
class WebSocketTransport
    : public Transport,
      public std::enable_shared_from_this<WebSocketTransport<CONFIG>> {
 public:
  typedef websocketpp::client<CONFIG> client_type;
  typedef websocketpp::lib::lock_guard<websocketpp::lib::mutex> scoped_lock;
  typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context>
      context_ptr;
  typedef websocketpp::config::asio_client::message_type::ptr message_ptr;

  WebSocketTransport(client_type& client, const std::string& uri);

  void Connect() override;
  void Disconnect() override;
  bool IsConnected() const override;
  void SendMessage(Message&& message) override;
  void Attach(const std::shared_ptr<TransportHandler>& handler) override;
  void Detach() override;
  bool HasHandler() const override;

 private:
  std::shared_ptr<WebSocketTransport<CONFIG>> GetSharedPtr() {
    return this->shared_from_this();
  }
  void OnEndpointOpen(websocketpp::connection_hdl);

  client_type& endpoint_;
  std::string uri_;
  bool open_;
  bool done_;

  websocketpp::lib::shared_ptr<websocketpp::lib::thread> thread_;
  websocketpp::lib::mutex lock_;
  websocketpp::connection_hdl hdl_;
  std::shared_ptr<TransportHandler> handler_;
  std::promise<void> connected_;
};

}  // namespace barbarossa::controlchannel

#include "barbarossa/websocket_transport.ipp"
#endif  // BARBAROSSA_WEBSOCKET_TRANSPORT_HPP_
