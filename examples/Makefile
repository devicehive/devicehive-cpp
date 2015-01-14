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
  else ifeq '${platform}' 'arm-bb'
    CROSS_COMPILE=arm-angstrom-linux-gnueabi-
  endif
endif


#home_path:=${PWD}
#base_path:=$(shell dirname "${home_path}")
home_path:=.
include_dirs:=-I${home_path}/../include -I${home_path}/../externals/include
ex_libs:=${home_path}/../externals/lib.${platform}

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


CXXFLAGS+=-Wall -Wno-unused-local-typedefs ${include_dirs} ${defines}
CXXFLAGS+=-fdata-sections -ffunction-sections
LDFLAGS+=-Wl,--gc-sections -pthread -L${ex_libs}
#LIBS+=-lboost_system

examples: simple_dev simple_gw zigbee_gw btle_gw

simple_dev: ${home_path}/simple_dev.cpp
	${CROSS_COMPILE}${CXX} -o simple_dev ${home_path}/simple_dev.cpp ${CXXFLAGS} ${LDFLAGS} \
		${ex_libs}/libboost_system.a

simple_gw: ${home_path}/simple_gw.cpp
	${CROSS_COMPILE}${CXX} -o simple_gw ${home_path}/simple_gw.cpp ${CXXFLAGS} ${LDFLAGS} \
		${ex_libs}/libboost_system.a

zigbee_gw: ${home_path}/zigbee_gw.cpp
	${CROSS_COMPILE}${CXX} -o zigbee_gw ${home_path}/zigbee_gw.cpp ${CXXFLAGS} ${LDFLAGS} \
		${ex_libs}/libboost_system.a

btle_gw: ${home_path}/btle_gw.cpp
	${CROSS_COMPILE}${CXX} -o btle_gw ${home_path}/btle_gw.cpp ${CXXFLAGS} ${LDFLAGS} \
		${ex_libs}/libboost_system.a -lbluetooth

#########################################################
# clean all the object files and applications
clean:
	@rm -rf *.o
	@rm -f simple_dev simple_gw zigbee_gw btle_gw
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


.PHONY: clean examples
