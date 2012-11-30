DeviceHive C++ Framework {#mainpage}
=====================================

This is C++ framework for DeviceHive.
Go to @subpage page_start page to get know how to use DeviceHive.


Overview
--------

DeviceHive provides a C++ library for various devices, including embedded.

It is cross-platform and may be used on Windows, MAC OS X and Linux.
Moreover, using corresponding cross compiler it is possible to use
DeviceHive C++ library on various CPU architectures: x86, ARM, MIPS...

DeviceHive C++ library provides implementation of DeviceHive RESTful interface
and binary protocol. Additionally it includes a lot of @subpage page_hive
(for example HTTP client or JSON parser) which may be used in any C++ project.

DeviceHive C++ library is header-only and very easy to use.
In most cases you just have to include corresponding header file.

DeviceHive C++ library widely uses some best-of-the-art boost libraries: asio, bind etc.


Directory structure
-------------------

DeviceHive C++ framework contains the following directories:
- `docs` - documentation, images and [doxygen](http://doxygen.org) configuration
- `examples` - various examples such as simple device or simple gateway
- `include` - header files:
  - `DeviceHive` - DeviceHive framework itself
  - `hive` - various basic C++ tools such as JSON parser or HTTP client
- `src` - source files (empty for now since framework is header-only)
- `test` - various unit tests
- `tools` - various tools and utilities


Examples
--------

There are several examples of using DeviceHive C++ Framework.
All examples can be found on the @subpage page_examples page.
Please take a look at @ref page_simple_dev first.
