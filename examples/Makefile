# use CROSS_COMPILE variable to set toolchain (empty by default):
#  >make CROSS_COMPILE=arm-linux-gnueabi-

# build variant: 'debug' or 'release' (by default)
# to change use VARIANT variable:
#  >make VARIANT=debug
#  >make VARIANT=release
variant:=release
ifdef VARIANT
  ifeq '${VARIANT}' 'release'
    variant:=release
  else ifeq '${VARIANT}' 'debug'
    variant:=debug
  else
    $(error '${VARIANT}' is unknown variant, expected: release or debug)
  endif
endif

# platform helper
ifdef PLATFORM
  platform=${PLATFORM}
else
  platform:=$(shell uname -m)
endif
ifndef CROSS_COMPILE
  # try to detect CROSS_COMPILE
  ifeq '${platform}' 'arm'
    CROSS_COMPILE=arm-unknown-linux-gnueabi-
  else ifeq '${platform}' 'bbb'
    CROSS_COMPILE=arm-linux-gnueabihf-
  else ifeq '${platform}' 'RPi2'
    CROSS_COMPILE=arm-linux-gnueabihf-
  else ifeq '${platform}' 'arm-bb'
    CROSS_COMPILE=arm-angstrom-linux-gnueabi-
  endif
endif


#home_path:=${PWD}
#base_path:=$(shell dirname "${home_path}")
home_path:=.
include_dirs:=-I${home_path}/../include -I${home_path}/../externals/include
ext_libs:=-L${home_path}/../externals/lib.${platform} -L${home_path}/../externals/lib

defines+=-DBOOST_SYSTEM_NO_DEPRECATED
# disable SSL for tests
defines+=-DHIVE_DISABLE_SSL

ifeq '${variant}' 'debug'
  defines+=-D_DEBUG
  defines+=-g
else # default
  defines+=-DNDEBUG
  defines+=-O3
endif


CXXFLAGS+=-Wall ${include_dirs} ${defines}
CXXFLAGS+=-fdata-sections -ffunction-sections
LDFLAGS+=${ext_libs}
LDFLAGS+=-Wl,--gc-sections -pthread -lboost_system

examples: simple_dev simple_gw zigbee_gw

simple_dev: ${home_path}/simple_dev.cpp ${home_path}/simple_dev.hpp ${home_path}/basic_app.hpp
	${CROSS_COMPILE}${CXX} -o simple_dev ${home_path}/simple_dev.cpp ${CXXFLAGS} ${LDFLAGS}

simple_gw: ${home_path}/simple_gw.cpp ${home_path}/simple_gw.hpp ${home_path}/basic_app.hpp
	${CROSS_COMPILE}${CXX} -o simple_gw ${home_path}/simple_gw.cpp ${CXXFLAGS} ${LDFLAGS}

zigbee_gw: ${home_path}/zigbee_gw.cpp ${home_path}/zigbee_gw.hpp ${home_path}/basic_app.hpp
	${CROSS_COMPILE}${CXX} -o zigbee_gw ${home_path}/zigbee_gw.cpp ${CXXFLAGS} ${LDFLAGS}

hello:
	echo '#include <stdio.h>\nint main() { printf("Hello!"); return 0;}' | \
	${CROSS_COMPILE}${CXX} -o hello -x c++ - ${CXXFLAGS} ${LDFLAGS}


#########################################################
# clean all the object files and applications
clean:
	@rm -rf *.o
	@rm -f simple_dev simple_gw zigbee_gw
	@rm -f ${PCH_objects}


# PCH header files
PCH_sources+=${home_path}/hive/pch.hpp
PCH_objects:=${PCH_sources:.hpp=.hpp.gch}
PCH: ${PCH_objects}


# rule to build gch files
%.hpp.gch: %.hpp
	@echo "[CXX] $<"
	@${CROSS_COMPILE}${CXX} ${CXXFLAGS} -c $< -o $@
%.h.gch: %.h
	@echo " [CC] $<"
	@${CROSS_COMPILE}${CC} ${CXXFLAGS} -c $< -o $@

# rules to build c/cpp files
%.o: %.cpp
	@echo "[CXX] $<"
	@${CROSS_COMPILE}${CXX} ${CXXFLAGS} -c $< -o $@
%.o: %.c
	@echo " [CC] $<"
	@${CROSS_COMPILE}${CC} ${CXXFLAGS} -c $< -o $@


.PHONY: clean hello examples
