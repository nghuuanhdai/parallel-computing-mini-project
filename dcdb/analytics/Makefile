include ../config.mk
#include ../common.mk

CXXFLAGS += -DBOOST_NETWORK_ENABLE_HTTPS -I../common/include -I$(DCDBDEPLOYPATH)/include -I$(DCDBDEPLOYPATH)/include/opencv4 -I$(DCDBDEPLOYPATH)/include/mariadb

LIBS = -L../lib -L$(DCDBDEPLOYPATH)/lib/ -ldl -lboost_system -lboost_thread -lboost_log_setup -lboost_log -lboost_regex -lpthread -rdynamic

ifeq ($(OS),Darwin)
	BACNET_PORT = bsd
	LIBEXT = dylib
	LIBFLAGS = -dynamiclib -install_name 
else
	BACNET_PORT = linux
	LIBEXT = so
	LIBFLAGS = -shared -Wl,-soname,
	PLUGINFLAGS = -fPIC
endif
OPERATOR_LIBS = $(foreach p,$(OPERATORS),libdcdboperator_$(p).$(LIBEXT))

all:   $(OPERATOR_LIBS)
debug: CXXFLAGS += -DDEBUG
debug: all

clean:
	rm -f $(OPERATOR_LIBS) $(shell find . -name "*.o")
	rm -f ../common/src/sensornavigator.o

$(OBJS)	: %.o : %.cpp

install_operator: $(OPERATOR_LIBS)
	install $^ $(DCDBDEPLOYPATH)/lib/

install_conf: $(foreach p,$(OPERATORS),config/$(p).conf)
	install -m 644 $^ $(DCDBDEPLOYPATH)/etc/

install: install_operator
	@echo "Done with installation."
	@echo "====================================="
	@echo "To copy the configuration files type:"
	@echo "				     > make install_conf"

operators/%.o: CXXFLAGS+= $(PLUGINFLAGS)
../common/src/sensornavigator.o: CXXFLAGS+= $(PLUGINFLAGS)

libdcdboperator_aggregator.$(LIBEXT): operators/aggregator/AggregatorOperator.o operators/aggregator/AggregatorConfigurator.o ../common/src/sensornavigator.o
	$(CXX) $(LIBFLAGS)$@ -o $@ $^ -L$(DCDBDEPLOYPATH)/lib/ -lboost_log -lboost_system -lboost_regex
	
libdcdboperator_smoothing.$(LIBEXT): operators/smoothing/SmoothingOperator.o operators/smoothing/SmoothingConfigurator.o ../common/src/sensornavigator.o
	$(CXX) $(LIBFLAGS)$@ -o $@ $^ -L$(DCDBDEPLOYPATH)/lib/ -lboost_log -lboost_system -lboost_regex
	
libdcdboperator_regressor.$(LIBEXT): operators/regressor/RegressorOperator.o operators/regressor/RegressorConfigurator.o ../common/src/sensornavigator.o
	$(CXX) $(LIBFLAGS)$@ -o $@ $^ -L$(DCDBDEPLOYPATH)/lib/ -lboost_log -lboost_system -lboost_regex -lopencv_core -lopencv_ml
	
libdcdboperator_classifier.$(LIBEXT): operators/regressor/RegressorOperator.o operators/regressor/ClassifierOperator.o operators/regressor/ClassifierConfigurator.o ../common/src/sensornavigator.o
	$(CXX) $(LIBFLAGS)$@ -o $@ $^ -L$(DCDBDEPLOYPATH)/lib/ -lboost_log -lboost_system -lboost_regex -lopencv_core -lopencv_ml
	
libdcdboperator_clustering.$(LIBEXT): operators/clustering/ClusteringOperator.o operators/clustering/ClusteringConfigurator.o ../common/src/sensornavigator.o
	$(CXX) $(LIBFLAGS)$@ -o $@ $^ -L$(DCDBDEPLOYPATH)/lib/ -lboost_log -lboost_system -lboost_regex -lopencv_core -lopencv_ml
	
libdcdboperator_cssignatures.$(LIBEXT): operators/cssignatures/CSOperator.o operators/cssignatures/CSConfigurator.o ../common/src/sensornavigator.o
	$(CXX) $(LIBFLAGS)$@ -o $@ $^ -L$(DCDBDEPLOYPATH)/lib/ -lboost_log -lboost_system -lboost_regex -lopencv_core
	
libdcdboperator_job_aggregator.$(LIBEXT): operators/aggregator/AggregatorOperator.o operators/aggregator/JobAggregatorOperator.o operators/aggregator/JobAggregatorConfigurator.o ../common/src/sensornavigator.o
	$(CXX) $(LIBFLAGS)$@ -o $@ $^ -L$(DCDBDEPLOYPATH)/lib/ -lboost_log -lboost_system -lboost_regex
	
libdcdboperator_testeroperator.$(LIBEXT): operators/testeroperator/TesterOperator.o operators/testeroperator/TesterOperatorConfigurator.o ../common/src/sensornavigator.o
	$(CXX) $(LIBFLAGS)$@ -o $@ $^ -L$(DCDBDEPLOYPATH)/lib/ -lboost_log -lboost_system -lboost_regex
	
libdcdboperator_filesink.$(LIBEXT): operators/filesink/FilesinkOperator.o operators/filesink/FilesinkConfigurator.o ../common/src/sensornavigator.o
	$(CXX) $(LIBFLAGS)$@ -o $@ $^ -L$(DCDBDEPLOYPATH)/lib/ -lboost_log -lboost_system -lboost_regex

libdcdboperator_smucngperf.$(LIBEXT): operators/smucngperf/SMUCNGPerfOperator.o operators/smucngperf/SMUCNGPerfConfigurator.o operators/smucngperf/SKXPMUMetrics.o ../common/src/sensornavigator.o
	$(CXX) $(LIBFLAGS)$@ -o $@ $^ -L$(DCDBDEPLOYPATH)/lib/ -lboost_log -lboost_system -lboost_regex

libdcdboperator_persystsql.$(LIBEXT): operators/persystsql/PerSystSqlOperator.o operators/persystsql/PerSystSqlConfigurator.o operators/persystsql/MariaDB.o ../common/src/sensornavigator.o
	$(CXX) $(LIBFLAGS)$@ -o $@ $^ -L$(DCDBDEPLOYPATH)/lib/ -L$(DCDBDEPLOYPATH)/lib/mariadb/ -lboost_log -lboost_system -lboost_regex -lmariadb

libdcdboperator_coolingcontrol.$(LIBEXT): operators/coolingcontrol/CoolingControlOperator.o operators/coolingcontrol/CoolingControlConfigurator.o operators/coolingcontrol/SNMPController.o ../dcdbpusher/sensors/snmp/SNMPConnection.o ../common/src/sensornavigator.o
	$(CXX) $(LIBFLAGS)$@ -o $@ $^ -L$(DCDBDEPLOYPATH)/lib/ -lboost_log -lboost_system -lboost_regex -lnetsnmp -lnetsnmpagent

libdcdboperator_healthchecker.$(LIBEXT): operators/healthchecker/HealthCheckerOperator.o operators/healthchecker/HealthCheckerConfigurator.o ../common/src/sensornavigator.o
	$(CXX) $(LIBFLAGS)$@ -o $@ $^ -L$(DCDBDEPLOYPATH)/lib/ -lboost_log -lboost_system -lboost_regex