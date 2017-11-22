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
import sinon from 'sinon'

const WebSocket = require('../..');

const logCallback = sinon.spy();

test.cb('log callback is not called by default', t => {
  t.plan(1);
  const nonExistingServerUrl = 'ws://localhost:12345';
  const client = new WebSocket(nonExistingServerUrl);
  client.onlog = logCallback;
  client.onerror = () => {
    t.false(logCallback.called);
    t.end();
  };
});

test.cb('log callback is called if logging is enabled', t => {
  t.plan(1);
  const nonExistingServerUrl = 'ws://localhost:12345';
  const client = new WebSocket(nonExistingServerUrl, {loggingEnabled: true});
  client.onlog = logCallback;
  client.onerror = () => {
    t.true(logCallback.called);
    t.end();
  };
});
