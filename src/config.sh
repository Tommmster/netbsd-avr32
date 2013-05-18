#! /bin/sh
#
# Quick & dirty hack to create and link the inc directory (which is of
# the same nature as well).
#
# This script will be gone forever once we get proper support for build.sh.

if test -e ./sys/inc; then :; else
	mkdir ./sys/inc;
fi

if test -e ./sys/inc/avr32; then :; else
	cd ./sys/inc && ln -s ../arch/avr32/include avr32 && cd ../..;
fi

if test -e ./sys/inc/machine; then :; else
	cd ./sys/inc && ln -s ../arch/avr32/include machine && cd ../..;
fi
