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
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

namespace barbarossa::controlchannel {

using json = nlohmann::json;

template <typename CONFIG>
inline WebSocketTransport<CONFIG>::WebSocketTransport(client_t& client,
                                                      const std::string& uri)
    : client_(client), uri_(uri), open_(false), done_(false) {
  // Moved all websocketpp initialization code to a seperate method.
  StartClient();
}

template <typename CONFIG>
inline WebSocketTransport<CONFIG>::~WebSocketTransport() {
  StopClient("disconnect");
}

template <typename CONFIG>
inline void WebSocketTransport<CONFIG>::Connect() {
  spdlog::debug("transport connecting");
  if (open_) {
    throw std::logic_error(ErrorMessages::kTransportAlreadyConnected);
  }

  websocketpp::lib::error_code ec;
  // typename client_t::connection_ptr connection_ptr_t;
  auto conn = client_.get_connection(uri_, ec);
  if (ec) {
    throw websocketpp::lib::system_error(ec.value(), ec.category(), "connect");
  }

  // Grab the handle for this connection
  hdl_ = conn->get_handle();

  // Queue the connection
  client_.connect(conn);

  // Wait for connected_ promise
  auto connected_future = connected_.get_future();

  spdlog::debug("transport connected and waiting for websocketpp on open");
  connected_future.get();  // This future can throw an exception
}

template <typename CONFIG>
inline void WebSocketTransport<CONFIG>::Disconnect() {
  if (!open_) {
    throw std::logic_error(ErrorMessages::kTransportAlreadyDisconnected);
  }

  StopClient("disconnect");  // TODO(DGL) This is regular shutdown.
}

template <typename CONFIG>
inline bool WebSocketTransport<CONFIG>::IsConnected() const {
  return open_ && !done_;
}

template <typename CONFIG>
inline void WebSocketTransport<CONFIG>::SendMessage(Message&& message) {
  // json j = message;
  auto j = json::array();
  for (std::size_t i = 0; i < message.Size(); i++) {
    j.push_back(message.Field(i));
  }
  spdlog::debug("sending message: {}", j.dump());

  websocketpp::lib::error_code ec;
  client_.send(hdl_, j.dump(), websocketpp::frame::opcode::text, ec);
  if (ec) {
    spdlog::error("websocket_transport: error sending message: {}",
                  ec.message());
    throw NetworkError(ec.message());
  }
}

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

// StartClient is called by the ctor. It inits websocketpp, attaches the
// callbacks and starts the thread.
template <typename CONFIG>
inline void WebSocketTransport<CONFIG>::StartClient() {
  client_.clear_access_channels(websocketpp::log::alevel::all);
  client_.set_access_channels(websocketpp::log::alevel::connect);
  client_.set_access_channels(websocketpp::log::alevel::disconnect);
  client_.set_access_channels(websocketpp::log::alevel::app);

  // On websocket is open handler sets a successfull connected promise to tell
  // our Connect method that our connection is established.
  client_.set_open_handler([&](websocketpp::connection_hdl) {
    spdlog::debug("transport on open");
    scoped_lock_t guard(lock_);
    open_ = true;
    connected_.set_value();  // Release successfully the Connect() method
  });

  // On websocket is close handler sets our transport state to disconnected.
  client_.set_close_handler([&](websocketpp::connection_hdl) {
    spdlog::debug("transport on close");
    scoped_lock_t guard(lock_);
    done_ = true;
  });

  // On websocket is fail handler sets an exception to our connected promise to
  // tell our Connect method that the connection failed.
  client_.set_fail_handler([&](websocketpp::connection_hdl) {
    spdlog::debug("transport on fail");
    if (!open_) {
      // Release the Connect() method with a failure
      connected_.set_exception(std::make_exception_ptr(
          NetworkError(ErrorMessages::kTransportConnectionFailed)));
    }
    scoped_lock_t guard(lock_);
    done_ = true;
  });

  client_.set_message_handler(
      [&](websocketpp::connection_hdl, message_ptr_t msg) {
        if (msg->get_opcode() != websocketpp::frame::opcode::text) {
          spdlog::error("unsupported binary data");
          // TODO(DGL) What should we do now? Raise ProtocolError?
          return;
        }
        RecvMessage(msg->get_payload());
      });

  client_.start_perpetual();
  thread_.reset(new websocketpp::lib::thread(&client_t::run, &client_));
}

template <typename CONFIG>
void WebSocketTransport<CONFIG>::StopClient(
    const std::string& reason) noexcept {
  // Leave if we closed already, e.g. with Disconnect()
  if (done_) {
    return;
  }

  // In StartEndpoint we executed start_perpetual and now we're doing the
  // opposit.
  client_.stop_perpetual();

  // Close the websocket connection
  client_.close(hdl_, websocketpp::close::status::normal, reason);

  // We're waiting until the websocketpp thread finishes it's work. Sample code
  // runs the join in the destructor. I don't like that because you don't know
  // why and where your thread is blocking.
  thread_->join();

  open_ = false;
}

template <typename CONFIG>
void WebSocketTransport<CONFIG>::RecvMessage(const std::string& payload) {
  if (!handler_) {
    spdlog::error("no transport handler attached");
    // TODO(DGL) What should we do now? Raise an error?
    return;
  }

  spdlog::debug("got message: {}", payload);

  // Parse the message.
  // TODO(DGL) We should create a Codec class
  auto j = json::parse(payload);
  if (!j.is_array()) {
    spdlog::error("protocol error: invalid message received.");
    // TODO(DGL) We should raise an protocol error?
    return;
  }

  Message message(j.size());
  std::size_t i = 0;
  for (auto& field : j) {
    message.SetField(i++, field);
  }

  handler_->OnMessage(std::move(message));
}

}  // namespace barbarossa::controlchannel