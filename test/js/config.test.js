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
const WebSocket = require('../..');

function runInvalidTlsConfigTest(options) {
  test('connect to wss server with invalid TLS configuration throws', t => {
    t.throws(() => {
      const wssServer = 'wss://localhost:12345';
      const client = new WebSocket(wssServer, options);
    }, Error);
  });
}

const missingCert = {
  key: 'key'
};

const missingKey = {
  cert: 'cert'
};

const missingCertAndKey = {};

const missingCa = {
  key: 'key',
  cert: 'cert',
  rejectUnauthorized: true
}

const invalidTlsConfigs = [missingCert, missingKey, missingCertAndKey, missingCa];
invalidTlsConfigs.forEach(config => runInvalidTlsConfigTest(config));
