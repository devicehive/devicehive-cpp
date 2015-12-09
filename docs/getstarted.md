Getting started {#page_start}
=============================

All key DeviceHive elements are presented on the picture below:

![DeviceHive framework elements](@ref preview2.png)

DeviceHive C++ framework is mostly focused on **Device** and **Gateway**
development, **Client** is coming soon.


DeviceHive C++ framework require *boost* library. Although most of the *boost*
libraries are header-only, there are a few *boost* libraries which have to be
built. Building *boost* is quite easy, see corresponding section below.

To use DeviceHive C++ framework on different platforms (*ARM* or *MIPS*)
a toolchain for cross compilation should be prepared, see corresponding
section below. Of course *boost* should be built using this toolchain too.


Building boost
--------------

To use DeviceHive C++ framework the *boost* header files should be available.
At least the following header-only *boost* library are used:
- `boost.asio`
- `boost.bind`
- `boost.smart_ptr`

Moreover, at least the following *boost* libraries should be built:
- `boost.system`

### Builing *boost* on Windows

[Download](http://www.boost.org/users/download/) and unpack *boost* library
package. Then open the visual studio command line tool and cd-ing into unpacked
*boost* folder. To build *boost* run the following commands:

~~~{.bat}
bootstrap.bat
b2.exe --with-system --with-date_time --with-thread
~~~

### Building *boost* on Linux

Some Linux distributives has boost package available.

Ubuntu:

~~~{.sh}
sudo apt-get install libboost-system-dev libboost-thread-dev
~~~

Archlinux:

~~~{.sh}
sudo pacman -S boost make
~~~

If there is no preconfigured *boost* package available it is quite easy
to build boost manually. [Download](http://www.boost.org/users/download/) and
unpack *boost* library package. Open terminal and cd-ing into unpacked *boost*
folder. Then run the following commands:

~~~{.sh}
./bootstrap.sh --with-libraries=system,date_time,thread
./b2
~~~

To build *boost* using custom toolchain `toolset` parameter may be used.
For example, to build *boost* for the RaspberryPi use the following commands:

~~~{.sh}
./bootstrap.sh --with-libraries=system,thread --prefix=/usr/local/raspberrypi
echo "using gcc : bbb : arm-linux-gnueabihf-g++ ;" >> ~/user-config.jam
./b2 toolset=gcc-bbb
~~~

Note, in that case, before build the *boost* you have ensure that the
`~/user-config.jam` configuration file contains the following line:

~~~
using gcc : arm : arm-linux-gnueabihf-g++ ;
~~~

### Usage

Once *boost* libraries have been built it is possible to build DeviceHive C++
examples. By default, the *boost* expected to be located in `externals`
subfolder. The directory structure should look like this:

- `examples` DeviceHive C++ examples
- `include` DeviceHive header files
- `externals` external headers and libraries
  - `include` header files
    - `boost` *boost* header files, copied from the `boost` directory of *boost* package
  - `lib` library files, copied from the `stage` directory of *boost* package



Preparing toolchain
-------------------

Since RaspberryPi and BeagleBone has ARM processor we have to use special
toolchain to build example applications.

Ubuntu has corresponding package:

~~~{.sh}
sudo apt-get install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf
~~~

Also there are a few tools which can help to prepare such a toolchain:
`crossdev`, `crosstool`, `crosstool-NG` etc.

### Installing using crosstool-NG

The following [article](http://www.bootc.net/archives/2012/05/26/how-to-build-a-cross-compiler-for-your-raspberry-pi)
describes how to prepare toolchain for RaspberryPi. The short version is:
    - download and unpack `crosstool-NG` package
    - `./configure && make && sudo make install`
    - `cd ./tools/crosstool-ng/raspberrypi`
    - `ct-ng menuconfig`
    - `ct-ng build`

The `crosstool-NG` package has the following dependencies: `bison` `flex`
`texinfo` `subversion` `libtool` `automake` `libncurses5-dev`. You can install
 all of these dependencies on Ubuntu using the following command:

~~~{.sh}
sudo apt-get install bison flex texinfo subversion libtool automake libncurses5-dev
~~~

### Simple "Hello World!" application

To check cross compilation is working just build the "Hello" example:
~~~{.sh}
make hello CROSS_COMPILE=arm-linux-gnueabihf-
~~~

The following command
~~~{.sh}
file hello
~~~
should print something like:
~~~{.sh}
hello: ELF 32-bit LSB executable, ARM, version 1 (SYSV), dynamically linked (uses shared libs), for GNU/Linux 3.5.0, not stripped
~~~

Now you can copy executable to your RaspberryPi device and check it's working fine:
~~~{.sh}
./hello World
~~~
