// Copyright (c) 2019 by nsyszr.io.
// Author: Tschokko
//
// Following code is rewritten based on the following Autobahn proven C++ WAMP
// implementation: https://github.com/crossbario/autobahn-cpp
// The control channel protocol is subset and slightly modified WAMP protocol
// optimized for our purporses. Therefore we cannot use a WAMP compliant client
// implementation.

#include <exception>

#include "barbarossa/errors.hpp"
#include "barbarossa/message.hpp"
#include "barbarossa/transport.hpp"
#include "spdlog/spdlog.h"

namespace barbarossa::controlchannel {

template <typename CONFIG>
inline WebSocketTransport<CONFIG>::WebSocketTransport(client_type& endpoint,
                                                      const std::string& uri)
    : endpoint_(endpoint), uri_(uri), open_(false), done_(false) {
  // On websocket is open handler sets a successfull connected promise to tell
  // our Connect method that our connection is established.
  /*endpoint_.set_open_handler([&](websocketpp::connection_hdl) {
    spdlog::debug("transport on open");
    scoped_lock guard(lock_);
    open_ = true;
    connected_.set_value();  // Release successfully the Connect() method
  });*/
  using websocketpp::lib::bind;
  using websocketpp::lib::placeholders::_1;
  endpoint_.set_open_handler(
      bind(&WebSocketTransport<CONFIG>::OnEndpointOpen, this, _1));

  // On websocket is close handler sets our transport state to disconnected.
  endpoint_.set_close_handler([&](websocketpp::connection_hdl) {
    spdlog::debug("transport on close");
    scoped_lock guard(lock_);
    done_ = true;
  });

  // On websocket is fail handler sets an exception to our connected promise to
  // tell our Connect method that the connection failed.
  endpoint_.set_fail_handler([&](websocketpp::connection_hdl) {
    spdlog::debug("transport on fail");
    if (!open_) {
      // Release the Connect() method with a failure
      connected_.set_exception(std::make_exception_ptr(
          NetworkError(ErrorMessages::kTransportConnectionFailed)));
    }
    scoped_lock guard(lock_);
    done_ = true;
  });

  endpoint_.set_tls_init_handler([](websocketpp::connection_hdl) {
    spdlog::debug("endpoint tls init handler");

    context_ptr ctx =
        std::make_shared<asio::ssl::context>(asio::ssl::context::sslv23);

    try {
      ctx->set_options(asio::ssl::context::default_workarounds |
                       asio::ssl::context::no_sslv2 |
                       asio::ssl::context::no_sslv3 |
                       asio::ssl::context::single_dh_use);
    } catch (std::exception& e) {
      std::cout << "Error in context pointer: " << e.what() << std::endl;
    }
    return ctx;
  });

  endpoint_.start_perpetual();
  thread_.reset(new websocketpp::lib::thread(&client_type::run, &endpoint_));
}

template <typename CONFIG>
inline void WebSocketTransport<CONFIG>::Connect() {
  spdlog::debug("transport connecting");
  if (open_) {
    throw std::logic_error(ErrorMessages::kTransportAlreadyConnected);
  }

  websocketpp::lib::error_code ec;
  typename client_type::connection_ptr con = endpoint_.get_connection(uri_, ec);
  if (ec) {
    throw websocketpp::lib::system_error(ec.value(), ec.category(), "connect");
  }

  // Grab the handle for this connection
  hdl_ = con->get_handle();

  // Queue the connection
  endpoint_.connect(con);

  // Wait for connected_ promise
  auto connected_future = connected_.get_future();

  spdlog::debug("transport connected and waiting for on open");
  connected_future.get();  // This future can throw an exception
}

template <typename CONFIG>
inline void WebSocketTransport<CONFIG>::Disconnect() {
  if (open_) {
    throw std::logic_error(ErrorMessages::kTransportAlreadyDisconnected);
  }
  endpoint_.close(hdl_, websocketpp::close::status::normal, "disconnect");
}

template <typename CONFIG>
inline bool WebSocketTransport<CONFIG>::IsConnected() const {
  return open_ && !done_;
}

template <typename CONFIG>
inline void WebSocketTransport<CONFIG>::SendMessage(Message&& message) {}

template <typename CONFIG>
inline void WebSocketTransport<CONFIG>::Attach(
    const std::shared_ptr<TransportHandler>& handler) {
  if (handler_) {
    throw std::logic_error(ErrorMessages::kTransportHandlerAlreadyAttached);
  }

  handler_ = handler;
  handler_->OnAttach(GetSharedPtr());
}

template <typename CONFIG>
inline void WebSocketTransport<CONFIG>::Detach() {
  if (!handler_) {
    throw std::logic_error(ErrorMessages::kTransportHandlerAlreadyDetached);
  }

  // TODO(DGL) Useful reason for detaching a handler???
  handler_->OnDetach("goodbye");
  handler_.reset();
}

template <typename CONFIG>
inline bool WebSocketTransport<CONFIG>::HasHandler() const {
  return handler_ != nullptr;
}

template <typename CONFIG>
inline void WebSocketTransport<CONFIG>::OnEndpointOpen(
    websocketpp::connection_hdl) {
  spdlog::debug("transport on open");
  scoped_lock guard(lock_);
  open_ = true;
  connected_.set_value();
}

}  // namespace barbarossa::controlchannel