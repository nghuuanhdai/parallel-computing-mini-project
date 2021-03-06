CASSANDRA_VERSION = 3.11.5
MOSQUITTO_VERSION = 1.5.5
BOOST_VERSION     = 1.70.0
OPENSSL_VERSION   = 1.1.1c
CPPDRV_VERSION    = 2.15.2
LIBUV_VERSION     = 1.38.1
BACNET-STACK_VERSION = 0.8.6
FREEIPMI_VERSION = 1.6.3
GCRYPT_VERSION   = 1.8.4
GPG-ERROR_VERSION = 1.36
NET-SNMP_VERSION = 5.8
OPENCV_VERSION = 4.1.0
MARIADBCONNECTOR_VERSION = 3.1.3
OPASTACK_VERSION = 10.10.0.0.445
LIBOPA_VERSION = 0.4.0

BOOST_VERSION_U   = $(subst .,_,$(BOOST_VERSION))
OPASTACK_VERSION_H = $(subst .4,-4,$(OPASTACK_VERSION))

DISTFILES = apache-cassandra-$(CASSANDRA_VERSION).tar.gz;http://archive.apache.org/dist/cassandra/$(CASSANDRA_VERSION)/apache-cassandra-$(CASSANDRA_VERSION)-bin.tar.gz \
            mosquitto-$(MOSQUITTO_VERSION).tar.gz;http://mosquitto.org/files/source/mosquitto-$(MOSQUITTO_VERSION).tar.gz \
            boost_$(BOOST_VERSION_U).tar.gz;https://sourceforge.net/projects/boost/files/boost/1.70.0/boost_1_70_0.tar.gz \
            openssl-$(OPENSSL_VERSION).tar.gz;https://www.openssl.org/source/openssl-$(OPENSSL_VERSION).tar.gz \
            libuv-v$(LIBUV_VERSION).tar.gz;https://dist.libuv.org/dist/v$(LIBUV_VERSION)/libuv-v$(LIBUV_VERSION).tar.gz \
            cpp-driver-$(CPPDRV_VERSION).tar.gz;https://github.com/datastax/cpp-driver/archive/$(CPPDRV_VERSION).tar.gz \
            bacnet-stack-$(BACNET-STACK_VERSION).tgz;https://downloads.sourceforge.net/project/bacnet/bacnet-stack/bacnet-stack-$(BACNET-STACK_VERSION)/bacnet-stack-$(BACNET-STACK_VERSION).tgz \
            libgpg-error-$(GPG-ERROR_VERSION).tar.gz;https://gnupg.org/ftp/gcrypt/libgpg-error/libgpg-error-$(GPG-ERROR_VERSION).tar.gz \
            libgcrypt-$(GCRYPT_VERSION).tar.gz;https://www.gnupg.org/ftp/gcrypt/libgcrypt/libgcrypt-$(GCRYPT_VERSION).tar.gz \
            freeipmi-$(FREEIPMI_VERSION).tar.gz;http://ftp.gnu.org/gnu/freeipmi/freeipmi-$(FREEIPMI_VERSION).tar.gz \
            net-snmp-$(NET-SNMP_VERSION).tar.gz;https://sourceforge.net/projects/net-snmp/files/net-snmp/$(NET-SNMP_VERSION)/net-snmp-$(NET-SNMP_VERSION).tar.gz/download \
            opencv-$(OPENCV_VERSION).tar.gz;https://github.com/opencv/opencv/archive/$(OPENCV_VERSION).tar.gz \
            mariadb-connector-c-$(MARIADBCONNECTOR_VERSION)-src.tar.gz;https://downloads.mariadb.com/Connectors/c/connector-c-$(MARIADBCONNECTOR_VERSION)/mariadb-connector-c-$(MARIADBCONNECTOR_VERSION)-src.tar.gz \
            IntelOPA-Basic.SLES124-x86_64.$(OPASTACK_VERSION).tgz;https://downloadmirror.intel.com/29107/eng/IntelOPA-Basic.SLES124-x86_64.$(OPASTACK_VERSION).tgz

DISTFILES_HASHES = apache-cassandra-3.11.5.tar.gz|9428d8cd8bf8880d6536142e4b837412;\
                   mosquitto-1.5.5.tar.gz|a17dffc6f63b2a4ab2eb5c51139e60e9;\
                   boost_1_70_0.tar.gz|fea771fe8176828fabf9c09242ee8c26;\
                   openssl-1.1.1c.tar.gz|15e21da6efe8aa0e0768ffd8cd37a5f6;\
                   cpp-driver-2.15.2.tar.gz|c33f14436c79fc740879c02917fb9022;\
                   libuv-v1.38.1.tar.gz|44e400ca3974a046b977f41e18c44142;\
                   bacnet-stack-0.8.6.tgz|544ebd42ed959deb2213209b66bbc460;\
                   libgcrypt-1.8.4.tar.gz|433ab0a47b25994fa92b165db185f174;\
                   libgpg-error-1.36.tar.gz|f2283e22959ef71f44d8be3d0987a96b;\
                   freeipmi-1.6.3.tar.gz|b2d97e20db9b81b460ce1b9dad5bf54e;\
                   net-snmp-5.8.tar.gz|63bfc65fbb86cdb616598df1aff6458a; \
                   opencv-4.1.0.tar.gz|b80c59c7e4feee6a00608315e02b0b73; \
                   IntelOPA-Basic.SLES124-x86_64.10.10.0.0.445.tgz|124e16f44ad7ba6dce6b15f4b2d364c3; \
                   mariadb-connector-c-3.1.3-src.tar.gz|384817d60cd890f83c5c9673bfa11a8b;

CASSANDRA_CLUSTER_NAME                 = Datacenter Monitor Database
CASSANDRA_FLUSH_LARGEST_MEMTABLES_AT   = 0.5
CASSANDRA_REDUCE_CACHE_SIZES_AT        = 0.6
CASSANDRA_REDUCE_CACHE_CAPACITY_TO     = 0.4
CASSANDRA_PARTITIONER                  = org.apache.cassandra.dht.ByteOrderedPartitioner
CASSANDRA_COMMITLOG_TOTAL_SPACE_IN_MB  = 256
CASSANDRA_COMMITLOG_SEGMENT_SIZE_IN_MB = 16
CASSANDRA_TOMBSTONE_WARN_THRESHOLD     = 100000
CASSANDRA_TOMBSTONE_FAILURE_THRESHOLD  = 10000000

NETSNMP_PERSISTENT_DIR=$(DCDBDEPLOYPATH)/var/net-snmp

FETCH = wget -c --no-check-certificate -O 
MD5 = $(if $(shell which md5 2>/dev/null),md5,$(if $(shell md5sum --tag Makefile 2&> /dev/null || true),md5sum --tag,openssl md5))
DISTFILESNAMES = $(foreach f,$(DISTFILES),$(shell echo "$(f)" | sed 's/;.*//'))
DISTFILESPATHS_FULL = $(foreach f,$(DISTFILES),$(shell echo "$(f)" | sed 's/.tar.gz;.*//; s/.tgz;.*//; s/.zip;.*//' ))
DISTFILESPATHS = apache-cassandra-$(CASSANDRA_VERSION) mosquitto-$(MOSQUITTO_VERSION) boost_$(BOOST_VERSION_U) openssl-$(OPENSSL_VERSION) libuv-v$(LIBUV_VERSION) cpp-driver-$(CPPDRV_VERSION) opencv-$(OPENCV_VERSION) mariadb-connector-c-$(MARIADBCONNECTOR_VERSION)-src
ifneq (,$(findstring bacnet,$(PLUGINS)))
 	DISTFILESPATHS += bacnet-stack-$(BACNET-STACK_VERSION) 
endif
ifneq (,$(findstring ipmi,$(PLUGINS)))
 	DISTFILESPATHS += libgpg-error-$(GPG-ERROR_VERSION) libgcrypt-$(GCRYPT_VERSION) freeipmi-$(FREEIPMI_VERSION)
endif
ifneq (,$(findstring snmp,$(PLUGINS)))
 	DISTFILESPATHS += net-snmp-$(NET-SNMP_VERSION) 
endif
ifneq (,$(findstring opa,$(PLUGINS)))
	ifneq (,$(findstring not installed, $(shell rpm -q opa-libopamgt-devel)))
        	DISTFILESPATHS += IntelOPA-Basic.SLES124-x86_64.$(OPASTACK_VERSION)
	endif
endif


# If cross-compiling for ARM, adjust the build settings
ifeq ("$(ARCH)", "arm")
	OPENSSL_TARGET = "linux-generic32"
else
	OPENSSL_TARGET = $(if $(findstring $(shell uname),Darwin),"darwin64-x86_64-cc","linux-x86_64")
endif

.PRECIOUS: $(DCDBDEPSPATH)/distfiles/% $(DCDBDEPSPATH)/%/.extracted $(DCDBDEPSPATH)/%/.patched
.SECONDEXPANSION:

getarchive = $(filter $1%,$(DISTFILESNAMES))
gethash = $(subst $(1)|,,$(filter $(1)%,$(subst ;, ,$(DISTFILES_HASHES))))

$(DCDBDEPSPATH):
	@mkdir -p $(DCDBDEPSPATH)

$(DCDBDEPSPATH)/distfiles: | $(DCDBDEPSPATH)
	@mkdir -p $(DCDBDEPSPATH)/distfiles

$(DCDBDEPLOYPATH):
	@mkdir -p $(DCDBDEPLOYPATH)/bin
	@mkdir -p $(DCDBDEPLOYPATH)/include
	@mkdir -p $(DCDBDEPLOYPATH)/lib

$(DCDBDEPSPATH)/distfiles/%: | $(DCDBDEPSPATH)/distfiles
	@echo "Check for $(*)"
	$(eval U := $(shell echo "$(DISTFILES)" | sed 's/.*$(*);//' | sed 's/ .*//'))
	@if [ -e $(@) ]; then \
		if [ `$(MD5) $(@)|sed -e 's/.*\ //'` != $(call gethash,$(*)) ]; then \
			rm $(@); \
		fi; \
	fi
	@if [ ! -e $(@) ]; then \
		$(FETCH) $(@) $(U); \
	fi

$(DCDBDEPSPATH)/%/.extracted: | $(DCDBDEPSPATH)/distfiles/$$(call getarchive,%)
	$(eval F := $(notdir $(|)))
	@if [ ! -e $(@) ]; then \
		printf "Verifying checksum: $(F) "; \
		if [ `$(MD5) $(|)|sed -e 's/.*\ //'` = $(call gethash,$(F)) ]; then \
			echo " => OK!"; \
		else \
			echo " => FAIL!"; \
			exit 2; \
		fi; \
		echo Extracting: $(F); \
		if [ "$(suffix $(F))" = "zip" ]; then \
			cd $(DCDBDEPSPATH) && unzip distfiles/$(F); \
		else \
			cd $(DCDBDEPSPATH) && tar xf distfiles/$(F) > /dev/null; \
		fi; \
		if [ $(F) = "IntelOPA-Basic.SLES124-x86_64.$(OPASTACK_VERSION).tgz" ]; then \
			echo "Extracting OPA Library and Headers..."; \
			cd $(DCDBDEPSPATH)/IntelOPA-Basic.SLES124-x86_64.$(OPASTACK_VERSION)/IntelOPA-Tools.SLES124-x86_64.$(OPASTACK_VERSION)/RPMS/x86_64/; \
			rpm2cpio opa-libopamgt-$(OPASTACK_VERSION_H).x86_64.rpm | cpio -idmv; \
			rpm2cpio opa-libopamgt-devel-$(OPASTACK_VERSION_H).x86_64.rpm | cpio -idmv; \
		fi; \
	fi
	@touch $@

$(DCDBDEPSPATH)/%/.patched: $(DCDBDEPSPATH)/%/.extracted
	$(eval P := $(realpath patches))
	if [ ! -e $(@) ] && [ -e $(P)/$(*).patch ]; then \
		echo Patching: $(*); \
		cd $(@D) && (patch -N -p1 < $(P)/$(*).patch); \
	fi
	@touch $@

$(DCDBDEPSPATH)/openssl-$(OPENSSL_VERSION)/.built: $(DCDBDEPSPATH)/openssl-$(OPENSSL_VERSION)/.patched
	@echo "Building OpenSSL library..."
	cd $(@D) && ./Configure shared --prefix=$(DCDBDEPLOYPATH) $(OPENSSL_TARGET)
	cd $(@D) && make && touch $(@)

$(DCDBDEPSPATH)/openssl-$(OPENSSL_VERSION)/.installed: $(DCDBDEPSPATH)/openssl-$(OPENSSL_VERSION)/.built | $(DCDBDEPLOYPATH)
	@echo "Installing OpenSSL library..."
	cd $(@D) && make install_sw && touch $(@)

$(DCDBDEPSPATH)/mosquitto-$(MOSQUITTO_VERSION)/.built: $(DCDBDEPSPATH)/mosquitto-$(MOSQUITTO_VERSION)/.patched
	@echo "Building Mosquitto library...";
	mkdir -p $(@D)/build;
	cd $(@D)/build && \
	CC=$(FULL_CC) CXX=$(FULL_CXX) cmake $(CMAKE_CROSS_FLAGS) \
	-DWITH_SRV=no \
	-DWITH_TLS=yes \
	-DWITH_TLS_PSK=yes \
	-DDOCUMENTATION=no \
	-DCMAKE_INSTALL_PREFIX=$(DCDBDEPLOYPATH)/ \
	$(@D) && \
	make -j $(MAKETHREADS) && \
	touch $(@)

$(DCDBDEPSPATH)/mosquitto-$(MOSQUITTO_VERSION)/.installed: $(DCDBDEPSPATH)/mosquitto-$(MOSQUITTO_VERSION)/.built | $(DCDBDEPLOYPATH)
	@echo "Installing Mosquitto library..."
	cd $(@D)/build && make install && touch $(@)

$(DCDBDEPSPATH)/boost_$(BOOST_VERSION_U)/.built: $(DCDBDEPSPATH)/boost_$(BOOST_VERSION_U)/.patched
	@echo "Building Boost..."
	@ if [ "$(ARCH)" = "arm" ]; then \
		echo " using gcc : arm : $(CROSS_COMPILE)g++ ; " > $(@D)/tools/build/src/user-config.jam; \
	fi
	cd $(@D) && ./bootstrap.sh --prefix=$(DCDBDEPLOYPATH) --with-libraries=atomic,chrono,date_time,exception,filesystem,log,program_options,random,regex,system,thread,timer && \
	./b2 stage && touch $(@)
		

$(DCDBDEPSPATH)/boost_$(BOOST_VERSION_U)/.installed: $(DCDBDEPSPATH)/boost_$(BOOST_VERSION_U)/.built
	cd $(@D) && ./b2 -j $(MAKETHREADS) --no-cmake-config install && touch $(@)

$(DCDBDEPSPATH)/libuv-v$(LIBUV_VERSION)/.built: $(DCDBDEPSPATH)/libuv-v$(LIBUV_VERSION)/.patched
	@echo "Building libuv..."
	mkdir -p $(@D)/lib
	cd $(@D)/lib && \
	CC=$(FULL_CC) CXX=$(FULL_CXX) cmake $(CMAKE_CROSS_FLAGS) \
		-DCMAKE_INSTALL_LIBDIR=lib \
		-DCMAKE_INSTALL_PREFIX=$(DCDBDEPLOYPATH)/ \
		.. && \
		make -j $(MAKETHREADS) && touch $(@)

$(DCDBDEPSPATH)/libuv-v$(LIBUV_VERSION)/.installed: $(DCDBDEPSPATH)/libuv-v$(LIBUV_VERSION)/.built | $(DCDBDEPLOYPATH)
	@echo "Installing libuv..."
	cd $(@D)/lib && make install && touch $(@)

$(DCDBDEPSPATH)/cpp-driver-$(CPPDRV_VERSION)/.built: $(DCDBDEPSPATH)/cpp-driver-$(CPPDRV_VERSION)/.patched $(DCDBDEPSPATH)/openssl-$(OPENSSL_VERSION)/.built $(DCDBDEPSPATH)/libuv-v$(LIBUV_VERSION)/.built
	@echo "Building cpp-driver..."
	mkdir -p $(@D)/build
	cd $(@D)/build && \
	CC=$(FULL_CC) CXX=$(FULL_CXX) cmake $(CMAKE_CROSS_FLAGS) \
		-DOPENSSL_ROOT_DIR=$(DCDBDEPSPATH)/openssl-$(OPENSSL_VERSION) \
		-DLIBUV_ROOT_DIR=$(DCDBDEPSPATH)/libuv-v$(LIBUV_VERSION) \
		-DCASS_BUILD_EXAMPLES=NO \
		-DCMAKE_INSTALL_LIBDIR=lib \
		-DCMAKE_INSTALL_PREFIX=$(DCDBDEPLOYPATH)/ \
		-DCMAKE_EXE_LINKER_FLAGS="-L$(DCDBDEPSPATH)/boost_$(BOOST_VERSION_U)/stage/lib -lboost_random" \
		-DCMAKE_SHARED_LINKER_FLAGS="-L$(DCDBDEPSPATH)/boost_$(BOOST_VERSION_U)/stage/lib -lboost_random" \
		.. && \
		make -j $(MAKETHREADS) && touch $(@)

$(DCDBDEPSPATH)/cpp-driver-$(CPPDRV_VERSION)/.installed: $(DCDBDEPSPATH)/cpp-driver-$(CPPDRV_VERSION)/.built | $(DCDBDEPLOYPATH)
	@echo "Installing cpp-driver..."
	cd $(@D)/build && make install && touch $(@)
	
$(DCDBDEPSPATH)/apache-cassandra-$(CASSANDRA_VERSION)/.built: $(DCDBDEPSPATH)/apache-cassandra-$(CASSANDRA_VERSION)/.patched
	@touch $(DCDBDEPSPATH)/apache-cassandra-$(CASSANDRA_VERSION)/.built

$(DCDBDEPSPATH)/apache-cassandra-$(CASSANDRA_VERSION)/.installed: $(DCDBDEPSPATH)/apache-cassandra-$(CASSANDRA_VERSION)/.built | $(DCDBDEPLOYPATH)
	@echo ""
	@echo "Staging and configuring Cassandra..."
	@rsync -rlptD $(@D)/ $(DCDBDEPLOYPATH)/cassandra
	@$(eval P := $(shell echo "$(DCDBDEPLOYPATH)" | sed 's/\//\\\//g'))
	@sed -i.original -e 's/\/var\/lib\/cassandra/$(P)\/var\/lib\/cassandra/g' $(DCDBDEPLOYPATH)/cassandra/conf/cassandra.yaml
	@sed -i -e 's/.*cluster_name:.*/cluster_name:\ '\''$(CASSANDRA_CLUSTER_NAME)'\''/' $(DCDBDEPLOYPATH)/cassandra/conf/cassandra.yaml
	@sed -i -e 's/.*flush_largest_memtables_at:.*/flush_largest_memtables_at:\ $(CASSANDRA_FLUSH_LARGEST_MEMTABLES_AT)/' $(DCDBDEPLOYPATH)/cassandra/conf/cassandra.yaml
	@sed -i -e 's/.*reduce_cache_sizes_at:.*/reduce_cache_sizes_at:\ $(CASSANDRA_REDUCE_CACHE_SIZES_AT)/' $(DCDBDEPLOYPATH)/cassandra/conf/cassandra.yaml
	@sed -i -e 's/.*reduce_cache_capacity_to:.*/reduce_cache_capacity_to:\ $(CASSANDRA_REDUCE_CACHE_CAPACITY_TO)/' $(DCDBDEPLOYPATH)/cassandra/conf/cassandra.yaml
	@sed -i -e 's/.*partitioner:.*/partitioner:\ $(CASSANDRA_PARTITIONER)/' $(DCDBDEPLOYPATH)/cassandra/conf/cassandra.yaml
	@sed -i -e 's/.*commitlog_total_space_in_mb:.*/commitlog_total_space_in_mb: $(CASSANDRA_COMMITLOG_TOTAL_SPACE_IN_MB)/' $(DCDBDEPLOYPATH)/cassandra/conf/cassandra.yaml
	@sed -i -e 's/.*commitlog_segment_size_in_mb:.*/commitlog_segment_size_in_mb: $(CASSANDRA_COMMITLOG_SEGMENT_SIZE_IN_MB)/' $(DCDBDEPLOYPATH)/cassandra/conf/cassandra.yaml
	@sed -i -e 's/.*tombstone_warn_threshold:.*/tombstone_warn_threshold: $(CASSANDRA_TOMBSTONE_WARN_THRESHOLD)/' $(DCDBDEPLOYPATH)/cassandra/conf/cassandra.yaml
	@sed -i -e 's/.*tombstone_failure_threshold:.*/tombstone_failure_threshold: $(CASSANDRA_TOMBSTONE_FAILURE_THRESHOLD)/' $(DCDBDEPLOYPATH)/cassandra/conf/cassandra.yaml
	@touch $@

$(DCDBDEPSPATH)/bacnet-stack-$(BACNET-STACK_VERSION)/.built: $(DCDBDEPSPATH)/bacnet-stack-$(BACNET-STACK_VERSION)/.patched
	@echo ""
	@echo "Building BACNet-Stack..."
	$(eval BACNET_PORT:= $(if $(filter $(OS),Darwin),bsd,linux))
	cd $(@D) && BACNET_PORT=$(BACNET_PORT) MAKE_DEFINE=-fpic make -j $(MAKETHREADS) library
	touch $(@)

$(DCDBDEPSPATH)/bacnet-stack-$(BACNET-STACK_VERSION)/.installed: $(DCDBDEPSPATH)/bacnet-stack-$(BACNET-STACK_VERSION)/.built | $(DCDBDEPLOYPATH)
	@echo "Installing BACNet-Stack..."
	mkdir   $(DCDBDEPLOYPATH)/include/bacnet && \
	install $(DCDBDEPSPATH)/bacnet-stack-$(BACNET-STACK_VERSION)/include/* /$(DCDBDEPLOYPATH)/include/bacnet && \
	install $(DCDBDEPSPATH)/bacnet-stack-$(BACNET-STACK_VERSION)/lib/libbacnet.a /$(DCDBDEPLOYPATH)/lib/ && touch $(@)

$(DCDBDEPSPATH)/libgpg-error-$(GPG-ERROR_VERSION)/.built: $(DCDBDEPSPATH)/libgpg-error-$(GPG-ERROR_VERSION)/.patched
	@echo ""
	@echo "Building gpg-error library..."
	cd $(@D) && ./configure --prefix=$(DCDBDEPLOYPATH)
	cd $(@D) && make -j $(MAKETHREADS) && touch $(@)

$(DCDBDEPSPATH)/libgpg-error-$(GPG-ERROR_VERSION)/.installed: $(DCDBDEPSPATH)/libgpg-error-$(GPG-ERROR_VERSION)/.built
	@echo "Installing gpg-error library..."
	cd $(@D) && make install && touch $(@)

$(DCDBDEPSPATH)/libgcrypt-$(GCRYPT_VERSION)/.built: $(DCDBDEPSPATH)/libgcrypt-$(GCRYPT_VERSION)/.patched
	@echo ""
	@echo "Building gcrypt library..."
	cd $(@D) && CFLAGS="-I$(DCDBDEPSPATH)/libgpg-error-$(GPG-ERROR_VERSION)/src" LDFLAGS="-L$(DCDBDEPSPATH)/libgpg-error-$(GPG-ERROR_VERSION)/src/.libs" PATH="$(DCDBDEPSPATH)/libgpg-error-$(GPG-ERROR_VERSION)/src:$(PATH)" ./configure --prefix=$(DCDBDEPLOYPATH)
	cd $(@D) && make -j $(MAKETHREADS) && touch $(@)

$(DCDBDEPSPATH)/libgcrypt-$(GCRYPT_VERSION)/.installed: $(DCDBDEPSPATH)/libgcrypt-$(GCRYPT_VERSION)/.built
	@echo "Installing gcrypt library..."
	cd $(@D) && make install && touch $(@)

$(DCDBDEPSPATH)/freeipmi-$(FREEIPMI_VERSION)/.built: $(DCDBDEPSPATH)/freeipmi-$(FREEIPMI_VERSION)/.patched
	@echo ""
	@echo "Building FreeIPMI library..."
	cd $(@D) && CFLAGS="-I$(DCDBDEPSPATH)/libgcrypt-$(GCRYPT_VERSION)/src -I$(DCDBDEPSPATH)/libgpg-error-$(GPG-ERROR_VERSION)/src" LDFLAGS=-L$(DCDBDEPSPATH)/libgcrypt-$(GCRYPT_VERSION)/src/.libs ./configure --prefix=$(DCDBDEPLOYPATH) --without-argp
	cd $(@D) && make -j $(MAKETHREADS) && touch $(@)

$(DCDBDEPSPATH)/freeipmi-$(FREEIPMI_VERSION)/.installed: $(DCDBDEPSPATH)/freeipmi-$(FREEIPMI_VERSION)/.built | $(DCDBDEPLOYPATH)
	@echo "Installing FreeIPMI library..."
	cd $(@D) && make install && touch $(@)

$(DCDBDEPSPATH)/net-snmp-$(NET-SNMP_VERSION)/.built: $(DCDBDEPSPATH)/net-snmp-$(NET-SNMP_VERSION)/.patched
	@echo ""
	@echo "Building net-SNMP library..."
	cd $(@D) && ./configure --prefix=$(DCDBDEPLOYPATH) --with-default-snmp-version=3 --with-defaults --with-logfile=none --with-persistent-directory=$(NETSNMP_PERSISTENT_DIR) --disable-embedded-perl --disable-perl-cc-checks --without-perl-modules --disable-agent --disable-applications --disable-manuals --disable-scripts --disable-mibs
	cd $(@D) && make -j $(MAKETHREADS) && touch $(@)

$(DCDBDEPSPATH)/net-snmp-$(NET-SNMP_VERSION)/.installed: $(DCDBDEPSPATH)/net-snmp-$(NET-SNMP_VERSION)/.built | $(DCDBDEPLOYPATH)
	@echo ""
	@echo "Installing net-SNMP library..."
	cd $(@D) && make install && touch $(@)
	
$(DCDBDEPSPATH)/opencv-$(OPENCV_VERSION)/.built: $(DCDBDEPSPATH)/opencv-$(OPENCV_VERSION)/.patched
	@echo ""
	@echo "Building OpenCV library..."
	cd $(@D) && mkdir build && cd build && cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$(DCDBDEPLOYPATH) -DCMAKE_INSTALL_LIBDIR=lib -DINSTALL_C_EXAMPLES=OFF -DBUILD_EXAMPLES=OFF -DBUILD_LIST=core,dnn,features2d,flann,gapi,ml ..
	cd $(@D) && cd build && make -j $(MAKETHREADS) && touch $(@)

$(DCDBDEPSPATH)/opencv-$(OPENCV_VERSION)/.installed: $(DCDBDEPSPATH)/opencv-$(OPENCV_VERSION)/.built | $(DCDBDEPLOYPATH)
	@echo ""
	@echo "Installing OpenCV library..."
	cd $(@D) && cd build && make install && touch $(@)

$(DCDBDEPSPATH)/IntelOPA-Basic.SLES124-x86_64.$(OPASTACK_VERSION)/.built: $(DCDBDEPSPATH)/IntelOPA-Basic.SLES124-x86_64.$(OPASTACK_VERSION)/.patched
	@touch $(@)

$(DCDBDEPSPATH)/IntelOPA-Basic.SLES124-x86_64.$(OPASTACK_VERSION)/.installed: $(DCDBDEPSPATH)/IntelOPA-Basic.SLES124-x86_64.$(OPASTACK_VERSION)/.built | $(DCDBDEPLOYPATH)
	@echo ""
	@echo "Installing Intel OPA library..."
	cp -a $(DCDBDEPSPATH)/IntelOPA-Basic.SLES124-x86_64.$(OPASTACK_VERSION)/IntelOPA-Tools.SLES124-x86_64.$(OPASTACK_VERSION)/RPMS/x86_64/usr/include/opamgt $(DCDBDEPLOYPATH)/include/ && \
	mv $(DCDBDEPLOYPATH)/include/opamgt/iba $(DCDBDEPLOYPATH)/include/ && \
	cp -a $(DCDBDEPSPATH)/IntelOPA-Basic.SLES124-x86_64.$(OPASTACK_VERSION)/IntelOPA-Tools.SLES124-x86_64.$(OPASTACK_VERSION)/RPMS/x86_64/usr/lib/* $(DCDBDEPLOYPATH)/lib/ && \
	ln -sr $(DCDBDEPLOYPATH)/lib/libopamgt.so.$(LIBOPA_VERSION) $(DCDBDEPLOYPATH)/lib/libopamgt.so.0 && \
	touch $(@)

$(DCDBDEPSPATH)/mariadb-connector-c-$(MARIADBCONNECTOR_VERSION)-src/.built: $(DCDBDEPSPATH)/mariadb-connector-c-$(MARIADBCONNECTOR_VERSION)-src/.patched
	@echo ""
	@echo "Building MariaDB Connector...";
	mkdir -p $(@D)/build;
	cd $(@D)/build && \
	CC=$(FULL_CC) CXX=$(FULL_CXX) cmake $(CMAKE_CROSS_FLAGS) \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_INSTALL_PREFIX=$(DCDBDEPLOYPATH)/ \
	-DINSTALL_LIBDIR=lib \
	-DINSTALL_PLUGINDIR=lib \
	-DCMAKE_PREFIX_PATH=$(DCDBDEPSPATH)/openssl-$(OPENSSL_VERSION) \
	-DCMAKE_C_FLAGS="-L$(DCDBDEPSPATH)/openssl-$(OPENSSL_VERSION) -lcrypto -lssl" \
	$(@D) && \
	make -j $(MAKETHREADS) && \
	touch $(@)

$(DCDBDEPSPATH)/mariadb-connector-c-$(MARIADBCONNECTOR_VERSION)-src/.installed: $(DCDBDEPSPATH)/mariadb-connector-c-$(MARIADBCONNECTOR_VERSION)-src/.built | $(DCDBDEPLOYPATH)
	@echo "Installing Maria DB Connector library..."
	cd $(@D)/build && make install && touch $(@)

