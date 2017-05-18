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

#include <memory>

#include <nan.h>

#include "IWebsocketClientWorker.h"
#include "WebsocketClientWorker.h"
#include "WebsocketClientWorkerTls.h"
#include "marshalling.h"

namespace wscpp
{
class WebsocketClientWrapper : public Nan::ObjectWrap
{
public:
  static NAN_MODULE_INIT(Init)
  {
    v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("WebsocketClientWorker").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(2);

    SetPrototypeMethod(tpl, "send", send);
    SetPrototypeMethod(tpl, "close", close);

    constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());
    Nan::Set(target, Nan::New("WebsocketClientWorker").ToLocalChecked(),
             Nan::GetFunction(tpl).ToLocalChecked());
  }

private:
  WebsocketClientWrapper(IWebsocketClientWorker* worker) : worker(worker) {}

  ~WebsocketClientWrapper() override = default;

  static NAN_METHOD(New)
  {
    if (info.Length() != 3) {
      return Nan::ThrowError("Invalid number of arguments");
    }

    if (info.IsConstructCall()) {
      try {
        auto parameters = std::make_unique<Parameters>(info);
        IWebsocketClientWorker* worker = nullptr;
        if (parameters->serverUri->get_secure()) {
          worker = new WebsocketClientWorkerTls(std::move(parameters));
        } else {
          worker = new WebsocketClientWorker(std::move(parameters));
        }

        WebsocketClientWrapper* wrapper = new WebsocketClientWrapper(worker);

        wrapper->Wrap(info.This());
        info.GetReturnValue().Set(info.This());

      } catch (const std::exception& e) {
        Nan::ThrowError(e.what());
        info.GetReturnValue().SetUndefined();
        return;
      }

    } else {
      const int argc = 3;
      v8::Local<v8::Value> argv[argc] = {info[0], info[1], info[2]};
      v8::Local<v8::Function> cons = Nan::New(constructor());
      info.GetReturnValue().Set(Nan::NewInstance(cons, argc, argv).ToLocalChecked());
    }
  }

  static NAN_METHOD(send)
  {
    Nan::HandleScope scope;
    if (info.Length() != 2) {
      return Nan::ThrowError("Invalid number of arguments");
    }
    const bool isBinary = info[1]->BooleanValue();
    v8::Local<v8::Object> buffer = info[0]->ToObject();
    const char* data = node::Buffer::Data(buffer);
    const std::size_t size = node::Buffer::Length(buffer);

    auto wrapper = Nan::ObjectWrap::Unwrap<WebsocketClientWrapper>(info.Holder());

    if (isBinary) {
      wrapper->worker->sendBinaryMessage(data, size);
    } else {
      wrapper->worker->sendTextMessage(data, size);
    }
  }

  static NAN_METHOD(close)
  {
    if (info.Length() != 2) {
      return Nan::ThrowError("Invalid number of arguments");
    }
    std::uint16_t code = 1000;
    std::string reason;

    using marshalling::convertFromV8;
    using marshalling::maybeToOptionalValue;

    convertFromV8(maybeToOptionalValue(info[0]), code);
    convertFromV8(maybeToOptionalValue(info[1]), reason);

    auto wrapper = Nan::ObjectWrap::Unwrap<WebsocketClientWrapper>(info.Holder());
    wrapper->worker->close(code, reason);
  }

  static inline Nan::Persistent<v8::Function>& constructor()
  {
    static Nan::Persistent<v8::Function> constructorFunctino;
    return constructorFunctino;
  }

  IWebsocketClientWorker* worker;
};

} // namespace wscpp

NODE_MODULE(websocketpp_client, wscpp::WebsocketClientWrapper::Init)
