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

#ifndef WSCPP_WEBSOCKETCLIENTWORKERTLS_H
#define WSCPP_WEBSOCKETCLIENTWORKERTLS_H

#include "GenericWebsocketClientWorker.h"

namespace wscpp
{

class WebsocketClientWorkerTls : public GenericWebsocketClientWorker<websocketpp::config::asio_tls>
{
public:
  WebsocketClientWorkerTls(std::unique_ptr<Parameters> parameters)
      : GenericWebsocketClientWorker((std::move(parameters)))
  {
    using namespace std::placeholders;
    endpoint.set_tls_init_handler(bind(&WebsocketClientWorkerTls::on_tls_init, this, _1));
    start();
  }

  std::shared_ptr<boost::asio::ssl::context> on_tls_init(ConnectionHandle handle)
  {
    auto sslContext =
        std::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12_client);

    if (parameters->tls.ca) {
      for (const std::string& ca : parameters->tls.ca.get()) {
        sslContext->add_certificate_authority(boost::asio::buffer(ca.data(), ca.size()));
      }
    }

    if (!parameters->tls.cert->empty() && !parameters->tls.key->empty()) {
      constexpr auto fileFormat = boost::asio::ssl::context_base::file_format::pem;
      sslContext->use_private_key(
          boost::asio::buffer(parameters->tls.key->data(), parameters->tls.key->size()),
          fileFormat);
      sslContext->use_certificate(
          boost::asio::buffer(parameters->tls.cert->data(), parameters->tls.cert->size()),
          fileFormat);
    }
    if ((parameters->tls.rejectUnauthorized && parameters->tls.rejectUnauthorized.get()) ||
        !parameters->tls.rejectUnauthorized) {
      sslContext->set_verify_mode(boost::asio::ssl::verify_peer |
                                  boost::asio::ssl::verify_fail_if_no_peer_cert);
    } else {
      sslContext->set_verify_mode(boost::asio::ssl::verify_none);
    }

    if (parameters->tls.secureOptions) {
      sslContext->set_options(parameters->tls.secureOptions.get());
    }

    if (parameters->tls.useUnencryptedTls && parameters->tls.useUnencryptedTls.get()) {
      SSL_CTX_set_cipher_list(sslContext->native_handle(), "eNULL");
    }

    return sslContext;
  }
};
} // namespace wscpp

#endif // WSCPP_WEBSOCKETCLIENTWORKERTLS_H
