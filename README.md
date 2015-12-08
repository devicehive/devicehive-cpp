DeviceHive C++ framework
========================

[DeviceHive]: http://devicehive.com "DeviceHive framework"
[DataArt]: http://dataart.com "DataArt"

This is C++ framework for [DeviceHive].
For more information please see corresponding documentation.


About DeviceHive
----------------

[DeviceHive] turns any connected device into the part of Internet of Things.
It provides the communication layer, control software and multi-platform
libraries to bootstrap development of smart energy, home automation, remote
sensing, telemetry, remote control and monitoring software and much more.
Connect embedded Linux using Python or C++ libraries and JSON protocol or
connect AVR, Microchip devices using lightweight C libraries and BINARY
protocol. Develop client applications using HTML5/JavaScript, iOS and Android
libraries. For solutions involving gateways, there is also gateway middleware
that allows to interface with devices connected to it. Leave communications
to [DeviceHive] and focus on actual product and innovation.


Building
------------------

### Obtaining source code

```
git clone https://github.com/devicehive/devicehive-cpp.git
```


### Pre-requisites
First of all you need to install required libraries and tools to build device hive cpp library.
Depending on your preferences you need to install c++ compiler toolset.

Ubuntu:
```
sudo apt-get install libboost-all-dev make
```

Archlinux:
```
sudo pacman -S boost make
```


Now you are ready to go to examples directory and issue make command.
This would build all the required library source codes and create example binary files.

DeviceHive license
------------------

[DeviceHive] is developed by [DataArt] Apps and distributed under Open Source
[MIT license](http://en.wikipedia.org/wiki/MIT_License). This basically means
you can do whatever you want with the software as long as the copyright notice
is included. This also means you don't have to contribute the end product or
modified sources back to Open Source, but if you feel like sharing, you are
highly encouraged to do so!

Copyright (C) 2012 DataArt Apps

All Rights Reserved
