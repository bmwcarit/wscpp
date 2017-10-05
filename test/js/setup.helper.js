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

'use strict';

const WebSocket = require('../..');

const http = require('http');
const https = require('https');
const fs = require('fs');
const getPort = require('get-port');
const WebSocketServer = require('ws').Server;
const promisifyListen = require('@sane/promisify-listen').promisifyListen;
const SegfaultHandler = require('segfault-handler');

SegfaultHandler.registerHandler('crash.log');

function initNonTlsCommunication() {
  return getPort().then((port) => {
    const httpServer = http.createServer();
    return promisifyListen(httpServer).listenAsync(port).then(() => {
      const setup = {};
      setup.server = new WebSocketServer({server: httpServer});
      const websocketServerUrl = 'ws://localhost:' + port;
      setup.client = new WebSocket(websocketServerUrl);
      return setup;
    });
  });
}

function readCerts(type, suffix) {
  const pathPrefix = 'test/certificates/generated/';
  const certName = pathPrefix + type + suffix;
  const certs = {
    cert: fs.readFileSync(certName + '.cert.pem'),
    key: fs.readFileSync(certName + '.key.pem'),
    ca: fs.readFileSync(pathPrefix + 'ca' + suffix + '.cert.pem')
  };
  return certs;
}

function initTlsCommunicationEncrypted(serverSuffix, clientSuffix) {
  return getPort().then((port) => {
    const serverOptions = readCerts('server', serverSuffix);
    const httpsServer = https.createServer(serverOptions);

    return promisifyListen(httpsServer).listenAsync(port).then(() => {
      const setup = {};
      setup.server = new WebSocketServer({server: httpsServer});

      const clientOptions = readCerts('client', clientSuffix);
      clientOptions.rejectUnauthorized = true;
      const websocketServerUrl = 'wss://localhost:' + port;
      setup.client = new WebSocket(websocketServerUrl, clientOptions);
      return setup;
    });
  });
}

function initTlsCommunicationUnencrypted(serverSuffix, clientSuffix) {
  return getPort().then((port) => {
    const serverOptions = readCerts('server', serverSuffix);
    serverOptions.ciphers = 'eNULL';
    const httpsServer = https.createServer(serverOptions);

    return promisifyListen(httpsServer).listenAsync(port).then(() => {
      const setup = {};
      setup.server = new WebSocketServer({server: httpsServer});

      const clientOptions = readCerts('client', clientSuffix);
      clientOptions.rejectUnauthorized = true;
      clientOptions.useUnencryptedTls = true;
      const websocketServerUrl = 'wss://localhost:' + port;
      setup.client = new WebSocket(websocketServerUrl, clientOptions);
      return setup;
    });
  });
}

module.exports = {
  initNonTlsCommunication,
  initTlsCommunicationEncrypted,
  initTlsCommunicationUnencrypted
};
