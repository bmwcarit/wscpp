# wscpp 1.0.0

* Stability improvements

# wscpp 0.2.7

* Updated websocketpp version

# wscpp 0.2.6

* Allow to call close() when connection not established
* Fixed lifecycle issue that could cause a crash during shutdown
* Added option to enable logging of the underlying library
* Verify that cert and key are specified when connecting to TLS server

# wscpp 0.2.5

Added flag `useUnencryptedTls` to optionally set a TLS connection to unencrypted

# wscpp 0.2.4

Updated and cleaned up the project's dependencies.

# wscpp 0.2.3

Bumped version as there was a problem publishing 0.2.2 to npmjs.

# wscpp 0.2.2

Moved `nan` dependency from dev to regular in package.json.

# wscpp 0.2.1

Moved `napa` dependency from dev to regular in package.json.

# wscpp 0.2.0

* Added support for `code` and `reason` parameters for `close()`.
* Added support for specifying `binary` option when sending messages.
* `onclose` callback is invoked with event containing `code` and `reason`.
* `onerror` callback is invoked with event containing `code` and `reason`.
* Made require mechanism of native module part more robust.

# wscpp 0.1.0

This is the initial release of wscpp.
