curdir 				= $(shell pwd)
nproc 				= $(shell [[ -f /proc/cpuinfo ]] && cat /proc/cpuinfo | grep processor | wc -l)

all: prepare build test

debug: prepare-debug build test

prepare-debug:
	cmake -S $(curdir) -B $(curdir)/build-all/ -DCMAKE_BUILD_TYPE=Debug

prepare:
	cmake -S $(curdir) -B $(curdir)/build-all/ -DCMAKE_BUILD_TYPE=Release


build: prepare
	make -s -C $(curdir)/build-all -j$(nproc)


test: build
	make -C $(curdir)/build-all test

install: test
	make -C $(curdir)/build-all install

clean:
	@rm -rf $(curdir)/build-all/
	@find -name "*.o.*" -delete
	@find -name "*.so.*" -delete


.PHONY:all prepare build test clean install

