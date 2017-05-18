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

import test from 'ava';

const cartesianProduct = require('cartesian-product');
const Setup = require('./setup.helper.js');

function runEchoTest(config) {
  const setup = config[0];
  const message = config[1];

  const testName = 'message is sent to server and received again; connection: ' + setup.name +
      ', datatype: ' + typeof message;
  test.cb(testName, t => {
    setup.init().then((setup) => {

      setup.server.on('connection', (ws) => {
        ws.on('message', (message) => {
          ws.send(message);
        });
      });

      setup.client.onerror = () => {
        t.fail('onerror callback invoked');
      };
      setup.client.onclose = () => {
        t.end();
      };

      setup.client.onopen = () => {
        setup.client.send(config[1]);
      };

      setup.client.onmessage = (msg) => {
        t.deepEqual(msg.data, message);
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

function initTlsCommunication() {
  const serverSuffix = '1';
  const clientSuffix = '1';
  return Setup.initTlsCommunication(serverSuffix, clientSuffix);
}

const nonTlsConfig = {
  name: 'non TLS',
  init: Setup.initNonTlsCommunication
};
const tlsConfig = {
  name: 'TLS',
  init: initTlsCommunication
};

const configs = [nonTlsConfig, tlsConfig];
const allTestCases = cartesianProduct([configs, messages]);

allTestCases.forEach(testCase => runEchoTest(testCase));
