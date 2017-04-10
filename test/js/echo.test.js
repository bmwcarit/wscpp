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

import test from 'ava';

'use strict';

const WebSocket = require('../..');

const http = require('http');
const https = require('https');

const fs = require('fs');
const crypto = require('crypto');

const getPort = require('get-port');
const WebSocketServer = require('ws').Server;
const cartesianProduct = require('cartesian-product');
import { promisifyListen } from '@sane/promisify-listen';

function initWebsocket(websocketServerUrl, t, options) {
  const client = new WebSocket(websocketServerUrl, options);

  client.onerror = () => {
    t.fail("onerror callback invoked");
  };

  client.onclose = () => {
    t.end();
  };
  return client;
};

function initEchoServer(server) {
  server.on('connection', (ws) => {
    ws.on('message', (message) => {
      ws.send(message);
    });
  });
}

function initNonTlsCommunication(t) {
  return getPort().then((port) => {
    const httpServer = http.createServer();

    return promisifyListen(httpServer).listenAsync(port).then( () => {
      const setup = {};

      setup.server = new WebSocketServer({server: httpServer});
      initEchoServer(setup.server);

      const websocketServerUrl = 'ws://localhost:' + port;
      setup.client = initWebsocket(websocketServerUrl, t);

      return setup;
    });

  });
}

function initTlsCommunication(t) {
  return getPort().then((port) => {

    const serverOptions = {
      cert: fs.readFileSync('test/certificates/generated/server1.cert.pem'),
      key: fs.readFileSync('test/certificates/generated/server1.key.pem'),
      ca: fs.readFileSync('test/certificates/generated/ca1.cert.pem')
    };
    const httpsServer = https.createServer(serverOptions);
    return promisifyListen(httpsServer).listenAsync(port).then( () => {
      const setup = {};
      setup.server = new WebSocketServer({server: httpsServer});
      initEchoServer(setup.server);

      const clientOptions = {
        rejectUnauthorized: true,
        cert: fs.readFileSync('test/certificates/generated/client1.cert.pem'),
        key: fs.readFileSync('test/certificates/generated/client1.key.pem'),
        ca: fs.readFileSync('test/certificates/generated/ca1.cert.pem')
      };

      const websocketServerUrl = 'wss://localhost:' + port;
      setup.client = initWebsocket(websocketServerUrl, t, clientOptions);
      return setup;
    });
  });
}

function runEchoTest(config) {
  const testName = 'message is sent to server and received again; connection: ' + config[0].name +
      ', datatype: ' + typeof config[1];
  test.cb(testName, t => {
    config[0].init(t).then((setup) => {
      setup.client.onopen = () => {
        setup.client.send(config[1]);
      };

      setup.client.onmessage = (msg) => {
        t.deepEqual(msg.data, config[1]);
        setup.client.close();
      }
    });
  });
}

function sequentialNumberArray(count) {
  return Array.from(Array(count).keys());
}
function flattenArray(array) {
  return [].concat.apply([], array)
}

const shortNumberArray = sequentialNumberArray(10);
const longNumberArray = sequentialNumberArray(1000);

const inputList = [shortNumberArray, longNumberArray];
const inputPreprocessorFunctions = [JSON.stringify, Buffer.from];

const preprocessedInput = inputList.map((input) => {
  return inputPreprocessorFunctions.map((fun) => {
    return fun(input);
  });
});
const messages = flattenArray(preprocessedInput);

const nonTlsConfig = {
  name: 'non TLS',
  init: initNonTlsCommunication
};
const tlsConfig = {
  name: 'TLS',
  init: initTlsCommunication
};

const configs = [nonTlsConfig, tlsConfig];
const allTestCases = cartesianProduct([configs, messages]);

allTestCases.forEach(testCase => runEchoTest(testCase));
