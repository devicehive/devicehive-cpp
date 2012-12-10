DeviceHive C++ framework TODO list
==================================


Basic tools:
- JSON module:
  - support for UTF-8 encoding
  - formal tests
- HTTP module:
  - support for HTTPS connections, checks cerficiates
  - support for HTTP 1.1, mutiple request per connection
- LOG module:
  - support for logging level filter (limit) for some targets
  - support for message filters
  - support for text configurations
  - support for custom formats
- robust binary streams: support for exceptions
- documentation and examples for all modules

DeviceHive framework:
- support for C++ client framework
- protocol generator based on formal JSON description
- script to build externals (boost, openssl)
