.PHONY: info all clean pusher

info:
	@echo "Makefile to build RPM packages from this DCDB installation"
	@echo "Targets: "
	@echo "info:    Print this info message"
	@echo "all:     Build all packages"
	@echo "pusher:  Build pusher- and required libs-package"

pusher:
	rpmbuild --define "_topdir `pwd`" -bb SPECS/dcdb-pusher-0.3.spec
	rpmbuild --define "_topdir `pwd`" -bb SPECS/dcdb-pusher-libs-0.3.spec

#Deprecated by dcdb-pusher-deps package.
#May get useful again if the build system gets overhauled and only dependencies
#that require actual patching are shipped with DCDB.
#mosquitto:
#	cp ../../deps/distfiles/mosquitto-1.5.5.tar.gz SOURCES/
#	cp ../patches/mosquitto-1.5.5.patch SOURCES/
#	rpmbuild --define "_topdir `pwd`" -ba SPECS/libmosquitto-dcdb-1.5.5.spec

all: pusher

clean:
	rm -rf BUILD/* BUILDROOT/* RPMS/* SOURCES/* SRPMS/*
