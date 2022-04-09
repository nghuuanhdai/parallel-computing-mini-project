include ../config.mk

CXXFLAGS += -DBOOST_NETWORK_ENABLE_HTTPS \
           -I../common/include \
           -I$(DCDBDEPLOYPATH)/include

ifneq (,$(wildcard /usr/include/opamgt))
	CXXFLAGS += -I/usr/include/opamgt
endif
           
LIBS = -L$(DCDBDEPLOYPATH)/lib/ \
       -ldl \
       -lmosquitto \
       -lboost_system \
       -lboost_thread \
       -lboost_filesystem \
       -lboost_log_setup \
       -lboost_log \
       -lboost_regex \
       -lpthread \
       -lcrypto \
       -lssl \
       -rdynamic
       
OBJS = dcdbpusher.o \
       Configuration.o \
       MQTTPusher.o \
       RestAPI.o \
       PluginManager.o \
       ../analytics/OperatorManager.o \
       ../common/src/sensornavigator.o \
       ../common/src/globalconfiguration.o \
       ../common/src/logging.o \
       ../common/src/RESTHttpsServer.o

TARGET = dcdbpusher

ifeq ($(OS),Darwin)
	BACNET_PORT = bsd
	LIBEXT = dylib
	LIBFLAGS = -dynamiclib -install_name 
else
	LIBS+= -Wl,-rpath-link=$(DCDBDEPLOYPATH)/lib/
	BACNET_PORT = linux
	LIBEXT = so
	LIBFLAGS = -shared -Wl,-soname,
	PLUGINFLAGS = -fPIC
endif
PLUGIN_LIBS = $(foreach p,$(PLUGINS),libdcdbplugin_$(p).$(LIBEXT))

all:	$(TARGET) $(PLUGIN_LIBS)

$(TARGET): $(foreach f,$(DISTFILESPATHS),$(DCDBDEPSPATH)/$(f)/.installed) $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

debug: CXXFLAGS += -DDEBUG
debug: all

info:
	@echo $(PLUGINS) $(PLUGIN_LIBS)

clean:
	rm -f $(PLUGIN_LIBS) $(TARGET) $(OBJS) $(foreach p,$(PLUGINS),sensors/$(p)/*.o)

$(OBJS)	: %.o : %.cpp

install_lib: $(PLUGIN_LIBS)
	install $^ $(DCDBDEPLOYPATH)/lib/

install_conf: $(foreach p,dcdbpusher $(PLUGINS),config/$(p).conf)
	install -m 644 $^ $(DCDBDEPLOYPATH)/etc/

install: $(TARGET) install_lib
	install $(TARGET) $(DCDBDEPLOYPATH)/bin/
	@echo "Done with installation."
	@echo "====================================="
	@echo "To copy the configuration files type:"
	@echo "				     > make install_conf"

BACNET_SRC:= $(shell find $(DCDBDEPSPATH) -name "bacnet-stack-*" -type d)
sensors/%.o: CXXFLAGS+= $(PLUGINFLAGS)
sensors/bacnet/%.o: CXXFLAGS+= -I$(BACNET_SRC)/include -I$(BACNET_SRC)/ports/$(BACNET_PORT)
../common/src/sensornavigator.o: CXXFLAGS+= $(PLUGINFLAGS)

libdcdbplugin_sysfs.$(LIBEXT): sensors/sysfs/SysfsSensorGroup.o sensors/sysfs/SysfsConfigurator.o
	$(CXX) $(LIBFLAGS)$@ -o $@ $^ -L$(DCDBDEPLOYPATH)/lib/ -lboost_log -lboost_system -lboost_regex

libdcdbplugin_perfevent.$(LIBEXT): sensors/perfevent/PerfSensorGroup.o sensors/perfevent/PerfeventConfigurator.o
	$(CXX) $(LIBFLAGS)$@ -o $@ $^ -L$(DCDBDEPLOYPATH)/lib/ -lboost_log -lboost_system

libdcdbplugin_ipmi.$(LIBEXT): sensors/ipmi/IPMISensorGroup.o sensors/ipmi/IPMIHost.o sensors/ipmi/IPMIConfigurator.o sensors/ipmi/LenovoXCC.o
	$(CXX) $(LIBFLAGS)$@ -o $@ $^ -L$(DCDBDEPLOYPATH)/lib/ -lboost_log -lboost_system -lboost_regex -lfreeipmi

libdcdbplugin_pdu.$(LIBEXT): sensors/pdu/PDUSensorGroup.o sensors/pdu/PDUUnit.o sensors/pdu/PDUConfigurator.o
	$(CXX) $(LIBFLAGS)$@ -o $@ $^ -L$(DCDBDEPLOYPATH)/lib/ -lcrypto -lssl -lboost_log -lboost_system

libdcdbplugin_bacnet.$(LIBEXT): sensors/bacnet/BACnetSensorGroup.o sensors/bacnet/BACnetClient.o sensors/bacnet/BACnetConfigurator.o
	$(CXX) $(LIBFLAGS)$@ -o $@ $^ -L$(DCDBDEPLOYPATH)/lib/ -lboost_log -lboost_system -lbacnet

libdcdbplugin_snmp.$(LIBEXT): sensors/snmp/SNMPSensorGroup.o sensors/snmp/SNMPConnection.o sensors/snmp/SNMPConfigurator.o
	$(CXX) $(LIBFLAGS)$@ -o $@ $^ -L$(DCDBDEPLOYPATH)/lib/ -lboost_log -lboost_system -lnetsnmp -lnetsnmpagent

libdcdbplugin_procfs.$(LIBEXT): sensors/procfs/ProcfsSensorGroup.o sensors/procfs/ProcfsParser.o sensors/procfs/ProcfsConfigurator.o
	$(CXX) $(LIBFLAGS)$@ -o $@ $^ -L$(DCDBDEPLOYPATH)/lib/ -lboost_log -lboost_system -lboost_regex

libdcdbplugin_tester.$(LIBEXT): sensors/tester/TesterSensorGroup.o sensors/tester/TesterConfigurator.o
	$(CXX) $(LIBFLAGS)$@ -o $@ $^ -L$(DCDBDEPLOYPATH)/lib/ -lboost_log -lboost_system

libdcdbplugin_gpfsmon.$(LIBEXT): sensors/gpfsmon/GpfsmonSensorGroup.o sensors/gpfsmon/GpfsmonConfigurator.o
	$(CXX) $(LIBFLAGS)$@ -o $@ $^ -L$(DCDBDEPLOYPATH)/lib/ -lboost_log -lboost_system

libdcdbplugin_opa.$(LIBEXT): sensors/opa/OpaSensorGroup.o sensors/opa/OpaConfigurator.o
	$(CXX) $(LIBFLAGS)$@ -o $@ $^ -L$(DCDBDEPLOYPATH)/lib/ -lboost_log -lboost_system -lopamgt -lssl

libdcdbplugin_msr.$(LIBEXT): sensors/msr/MSRSensorGroup.o sensors/msr/MSRConfigurator.o
	$(CXX) $(LIBFLAGS)$@ -o $@ $^ -L$(DCDBDEPLOYPATH)/lib/ -lboost_log -lboost_system

libdcdbplugin_caliper.$(LIBEXT): sensors/caliper/CaliperSensorGroup.o sensors/caliper/CaliperConfigurator.o
	$(CXX) $(LIBFLAGS)$@ -o $@ $^ -L$(DCDBDEPLOYPATH)/lib/ -lboost_log -lboost_system
# nvcc compiler and extra flags to enable the linking and creating of .so file to work: 
 libdcdbplugin_nvml.$(LIBEXT): sensors/nvml/nvmlSensorGroup.o sensors/nvml/nvmlConfigurator.o
	$(NVCC) -shared --compiler-options '-fPIC'  -o $@ $^ -L$(DCDBDEPLOYPATH)/lib/ -lboost_log -lboost_system -lnvidia-ml

libdcdbplugin_rest.$(LIBEXT): sensors/rest/RESTSensorGroup.o sensors/rest/RESTUnit.o sensors/rest/RESTConfigurator.o
	$(CXX) $(LIBFLAGS)$@ -o $@ $^ -L$(DCDBDEPLOYPATH)/lib/ -lcrypto -lssl -lboost_regex -lboost_log -lboost_system


