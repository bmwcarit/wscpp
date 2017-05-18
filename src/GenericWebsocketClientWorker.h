/*
 * #%L
 * %%
 * Copyright (C) 2017 BMW Car IT GmbH
 * %%
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * #L%
 */

#ifndef WSCPP_GENERICWEBSOCKETCLIENTWORKER_H
#define WSCPP_GENERICWEBSOCKETCLIENTWORKER_H

#include <cstdint>
#include <memory>
#include <string>
#include <tuple> // for std::ignore

#include <boost/variant.hpp>
#include <nan.h>
#include <uv.h>

#include <concurrentqueue.h>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio.hpp>

#include "IWebsocketClientWorker.h"
#include "Parameters.h"

namespace wscpp
{

namespace events
{
struct Open {
};
struct Close {
  int code;
  std::string reason;
};
struct Error {
  int code;
  std::string reason;
};
} // namespace events

/**
 * This class is modeled after NaN::AsyncWorker and
 * NaN::AsyncProgressWorkerBase.
 * It provides the runtime of the boost::asio::io_service event loop used for
 * websocket
 * communication and handles its interaction with the node event loop.
 */
template <typename Config>
class GenericWebsocketClientWorker : public IWebsocketClientWorker
{
private:
  using Event = boost::variant<events::Open, events::Close, events::Error>;
  using Endpoint = websocketpp::client<Config>;
  using MessagePtr = typename Config::message_type::ptr;

  class EventVisitor : public boost::static_visitor<>
  {
  public:
    EventVisitor(GenericWebsocketClientWorker* worker, v8::Local<v8::Object> context)
        : worker(worker), context(context)
    {
    }

    void operator()(const events::Open& e) const
    {
      worker->parameters->onOpenCallback->Call(context, 0, nullptr);
    }

    void operator()(const events::Close& e) const
    {
      v8::Local<v8::Value> argv[] = {Nan::New<v8::Int32>(e.code),
                                     Nan::New<v8::String>(e.reason).ToLocalChecked()};
      worker->parameters->onCloseCallback->Call(context, 2, argv);
    }

    void operator()(const events::Error& e) const
    {
      v8::Local<v8::Value> argv[] = {Nan::New<v8::Int32>(e.code),
                                     Nan::New<v8::String>(e.reason).ToLocalChecked()};
      worker->parameters->onErrorCallback->Call(context, 2, argv);
    }

  private:
    GenericWebsocketClientWorker* worker;
    v8::Local<v8::Object> context;
  };

protected:
  using ConnectionHandle = websocketpp::connection_hdl;

public:
  GenericWebsocketClientWorker(std::unique_ptr<Parameters> parameters)
      : parameters(std::move(parameters)), endpoint(), work(), messageAsyncHandle(nullptr),
        eventAsyncHandle(nullptr), connectionHandle(), receivedMessageQueue(), eventQueue()
  {
    initLibUv();
    initWebsocketPp();
  }

  void initLibUv()
  {
    work.data = this;

    messageAsyncHandle = new uv_async_t();
    uv_async_init(uv_default_loop(), messageAsyncHandle, messageReceivedCallback);
    messageAsyncHandle->data = this;

    eventAsyncHandle = new uv_async_t();
    uv_async_init(uv_default_loop(), eventAsyncHandle, eventOccuredCallback);
    eventAsyncHandle->data = this;
  }

  ~GenericWebsocketClientWorker() override = default;

  void runEventLoop() { endpoint.run(); }

  static void asyncExecute(uv_work_t* req)
  {
    GenericWebsocketClientWorker* worker = static_cast<GenericWebsocketClientWorker*>(req->data);
    worker->runEventLoop();
  }

  static void asyncExecuteComplete(uv_work_t* req)
  {
    GenericWebsocketClientWorker* worker = static_cast<GenericWebsocketClientWorker*>(req->data);
    worker->handReceivedMessagesToNode();
    worker->handReceivedEventsToNode();
    uv_close(reinterpret_cast<uv_handle_t*>(worker->messageAsyncHandle), asyncCloseMessageHandle);
    uv_close(reinterpret_cast<uv_handle_t*>(worker->eventAsyncHandle), asyncCloseEventHandle);
  }

  void sendTextMessage(const char* data, std::size_t size) override
  {
    // TODO error handling? use overload with error code or let websocketpp throw an
    // exception?
    endpoint.send(connectionHandle, data, size, websocketpp::frame::opcode::text);
  }

  void sendBinaryMessage(const char* data, std::size_t size) override
  {
    endpoint.send(connectionHandle, data, size, websocketpp::frame::opcode::binary);
  }

  void close(std::uint16_t code, const std::string& reason) override
  {
    endpoint.stop_perpetual();
    websocketpp::lib::error_code ec;
    endpoint.close(connectionHandle, code, reason, ec);
    if (ec) {
      const std::string errorMessage = "could not close connection: " + ec.message();
      Nan::ThrowError(errorMessage.c_str());
    }
  }

  /**
   * @brief messageReceived
   * @param connectionHandle
   * @param msg
   */
  void messageReceived(ConnectionHandle connectionHandle, MessagePtr msg)
  {
    receivedMessageQueue.enqueue(std::move(msg));
    signalMessageReceivedToJS();
  }

  /**
   * will be called by node's main thread
   */
  static NAUV_WORK_CB(messageReceivedCallback)
  {
    auto worker = static_cast<GenericWebsocketClientWorker*>(async->data);
    worker->handReceivedMessagesToNode();
  }

  /**
   * will be called by node's main thread
   */
  static NAUV_WORK_CB(eventOccuredCallback)
  {
    auto worker = static_cast<GenericWebsocketClientWorker*>(async->data);
    worker->handReceivedEventsToNode();
  }

  inline static void asyncCloseMessageHandle(uv_handle_t* handle)
  {
    auto worker = static_cast<GenericWebsocketClientWorker*>(handle->data);
    delete reinterpret_cast<uv_async_t*>(handle);
    delete worker;
  }

  inline static void asyncCloseEventHandle(uv_handle_t* handle)
  {
    delete reinterpret_cast<uv_async_t*>(handle);
  }

  /**
   * @brief invokes the JS "onmessage" callback for each message which was
   * received via websockets
   * This method may only be called from node's main thread!
   */
  void handReceivedMessagesToNode()
  {
    Nan::HandleScope scope;
    v8::Local<v8::Object> context = Nan::New(parameters->thisContext);
    MessagePtr msg;
    while (receivedMessageQueue.try_dequeue(msg)) {
      const websocketpp::frame::opcode::value opcode = msg->get_opcode();
      if (opcode == websocketpp::frame::opcode::text) {
        v8::Local<v8::Value> argv[] = {Nan::New<v8::String>(msg->get_payload()).ToLocalChecked()};
        parameters->onMessageCallback->Call(context, 1, argv);
      } else if (opcode == websocketpp::frame::opcode::binary) {
        // move payload into a heap allocated string
        // node Buffer wraps around that string's data
        // this string will be deleted when the node Buffer goes out of scope
        auto* payload = new std::string(std::move(msg->get_raw_payload()));
        auto buffer = Nan::NewBuffer(const_cast<char*>(payload->data()), payload->size(),
                                     bufferFreeCallback<std::string>, payload)
                          .ToLocalChecked();

        v8::Local<v8::Value> argv[] = {std::move(buffer)};
        parameters->onMessageCallback->Call(context, 1, argv);
      } else {
        // TODO error handling?
      }
    }
  }

  void handReceivedEventsToNode()
  {
    Nan::HandleScope scope;
    v8::Local<v8::Object> context = Nan::New(parameters->thisContext);
    Event event;
    EventVisitor visitor(this, context);
    while (eventQueue.try_dequeue(event)) {
      boost::apply_visitor(visitor, event);
    }
  }

protected:
  /**
   * @brief start worker asynchronously
   *
   * This will run `asyncExecute` in libuv's thread pool, after that,
   *`asyncExecuteComplete` is called.
   */
  void start()
  {
    websocketpp::lib::error_code ec;
    typename Endpoint::connection_ptr con = endpoint.get_connection(parameters->serverUri, ec);
    if (ec) {
      throw std::runtime_error("could not create connection because: " + ec.message());
    }

    endpoint.connect(con);

    uv_queue_work(uv_default_loop(), &work, asyncExecute,
                  reinterpret_cast<uv_after_work_cb>(asyncExecuteComplete));
  }

  std::unique_ptr<Parameters> parameters;
  Endpoint endpoint;

private:
  void initWebsocketPp()
  {
    endpoint.clear_access_channels(websocketpp::log::alevel::all);
    endpoint.init_asio();

    using namespace std::placeholders;
    endpoint.set_message_handler(
        bind(&GenericWebsocketClientWorker::messageReceived, this, _1, _2));
    endpoint.set_open_handler(bind(&GenericWebsocketClientWorker::connectionOpened, this, _1));
    endpoint.set_close_handler(bind(&GenericWebsocketClientWorker::connectionClosed, this, _1));
    endpoint.set_fail_handler(bind(&GenericWebsocketClientWorker::connectionFailed, this, _1));
  }

  void connectionOpened(ConnectionHandle connectionHandle)
  {
    this->connectionHandle = std::move(connectionHandle);
    signalEventOccurred(events::Open());
  }

  void connectionClosed(ConnectionHandle connectionHandle)
  {
    auto connection = endpoint.get_con_from_hdl(connectionHandle);
    signalEventOccurred(
        events::Close{connection->get_remote_close_code(), connection->get_remote_close_reason()});
  }

  void connectionFailed(ConnectionHandle connectionHandle)
  {
    auto connection = endpoint.get_con_from_hdl(connectionHandle);
    auto errorCode = connection->get_ec();
    signalEventOccurred(events::Error{errorCode.value(), errorCode.message()});
  }

  /**
   * @brief notify node to call `messageReceivedCallback`
   */
  void signalMessageReceivedToJS() const { uv_async_send(messageAsyncHandle); }

  /**
   * @brief notify node to call `eventOccuredCallback`
   */
  void signalEventOccurred(Event event)
  {
    eventQueue.enqueue(std::move(event));
    uv_async_send(eventAsyncHandle);
  }

  template <typename T>
  static void bufferFreeCallback(char* data, void* hint)
  {
    delete reinterpret_cast<T*>(hint);
  }

  uv_work_t work;
  uv_async_t* messageAsyncHandle;
  uv_async_t* eventAsyncHandle;

  ConnectionHandle connectionHandle;

  moodycamel::ConcurrentQueue<MessagePtr> receivedMessageQueue;
  moodycamel::ConcurrentQueue<Event> eventQueue;
};

} // namespace wscpp

#endif // WSCPP_GENERICWEBSOCKETCLIENTWORKER_H
