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

test.cb('connect to non existing server invokes onerror callback', t => {

  const nonExistingServerUrl = 'ws://localhost:12345';
  const client = new WebSocket(nonExistingServerUrl);

  client.onopen = () => {
    t.fail('onopen callback should not be invoked');
  };

  client.onclose = () => {
    t.end();
  };

  client.onerror = (e) => {
    t.false(e.code === undefined);
    t.false(e.reason === undefined);
  };
});
