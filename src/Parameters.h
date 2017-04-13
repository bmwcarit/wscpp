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

#ifndef WSCPP_PARAMETERS_H
#define WSCPP_PARAMETERS_H

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <websocketpp/uri.hpp>

#include "marshalling.h"

namespace wscpp
{

struct Parameters {

  struct TLS {
    // Verify the server certificate?
    boost::optional<bool> rejectUnauthorized;

    // Trusted certificates
    boost::optional<std::vector<std::string>> ca;

    // The certificate key (PEM encoded string)
    boost::optional<std::string> cert;

    // The private key (PEM encoded string)
    boost::optional<std::string> key;

    // Value is a numeric bitmask of the SSL_OP_* options (see
    // https://nodejs.org/api/crypto.html#crypto_openssl_options).
    boost::optional<std::int64_t> secureOptions;
  };

  Parameters(Nan::NAN_METHOD_ARGS_TYPE info)
  {
    Nan::HandleScope handleScope;
    if (info.Length() >= 2) {
      v8::String::Utf8Value serverUriStr(info[0]->ToString());
      serverUri = std::make_shared<websocketpp::uri>(*serverUriStr);
      if (!serverUri->get_valid()) {
        throw std::invalid_argument("websocket URI is not valid");
      }

      v8::Local<v8::Object> context = info[1].As<v8::Object>();
      thisContext.Reset(context);

      using marshalling::convertFromV8;
      using marshalling::getOptionalMemberValue;

      convertFromV8(getOptionalMemberValue(context, "onOpenCallback"), onOpenCallback);
      convertFromV8(getOptionalMemberValue(context, "onMessageCallback"), onMessageCallback);
      convertFromV8(getOptionalMemberValue(context, "onCloseCallback"), onCloseCallback);
      convertFromV8(getOptionalMemberValue(context, "onErrorCallback"), onErrorCallback);

      if (info.Length() == 3) {
        if (info[2]->IsObject()) {
          v8::Local<v8::Object> options = info[2].As<v8::Object>();
          convertFromV8(getOptionalMemberValue(options, "rejectUnauthorized"),
                        tls.rejectUnauthorized);
          convertFromV8(getOptionalMemberValue(options, "cert"), tls.cert);
          convertFromV8(getOptionalMemberValue(options, "key"), tls.key);
          convertFromV8(getOptionalMemberValue(options, "ca"), tls.ca);
          convertFromV8(getOptionalMemberValue(options, "secureOptions"), tls.secureOptions);
          if (tls.rejectUnauthorized && tls.rejectUnauthorized.get() && !tls.ca) {
            throw std::invalid_argument("need CA for server certificate verification");
          }
          if (tls.cert.is_initialized() != tls.key.is_initialized()) {
            throw std::invalid_argument("both cert and key are required");
          }
        } else {
          throw std::invalid_argument("options must be an object");
        }
      }
    } else {
      throw std::invalid_argument("Invalid number of arguments");
    }
  }

  std::shared_ptr<websocketpp::uri> serverUri;
  Nan::Persistent<v8::Object> thisContext;
  std::unique_ptr<Nan::Callback> onOpenCallback;
  std::unique_ptr<Nan::Callback> onMessageCallback;
  std::unique_ptr<Nan::Callback> onCloseCallback;
  std::unique_ptr<Nan::Callback> onErrorCallback;

  TLS tls;
};
} // namespace wscpp

#endif // WSCPP_PARAMETERS_H
