include ../config.mk

CXXFLAGS += -I../common/include/ \
            -I../lib/include \
            -I$(DCDBDEPLOYPATH)/include

OBJS =		../common/src/logging.o \
			../analytics/OperatorManager.o \
			../common/src/sensornavigator.o \
			../common/src/globalconfiguration.o \
			../common/src/RESTHttpsServer.o \
			analyticscontroller.o \
			CARestAPI.o \
			sensorcache.o \
			collectagent.o \
			configuration.o \
			simplemqttserver.o \
			simplemqttserverthread.o \
			simplemqttservermessage.o

LIBS =		-L../lib \
            -L$(DCDBDEPLOYPATH)/lib \
            -ldcdb \
            -ldl \
            -lpthread \
            -lcassandra \
            -luv \
            -lcrypto \
            -lssl \
            -lboost_system \
            -lboost_random \
            -lboost_thread \
            -lboost_filesystem \
            -lboost_date_time \
            -lboost_log_setup \
            -lboost_log \
            -lboost_regex \
            -rdynamic

TARGET =	collectagent

.PHONY : clean install

P =		$(shell cd $(DCDBDEPLOYPATH)/lib/ && pwd)
U =		$(shell uname)

$(TARGET): $(OBJS)
	@LD_LIBRARY_PATH=$(DCDBDEPLOYPATH)/lib:$$LD_LIBRARY_PATH \
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET)

clean:
	rm -f $(TARGET)
	rm -f $(OBJS)

install_conf: config/collectagent.conf
	install -m 644 $^ $(DCDBDEPLOYPATH)/etc/

install: $(TARGET)
	install $(TARGET) $(DCDBDEPLOYPATH)/bin/
	@echo "Done with installation."
	@echo "====================================="
	@echo "To copy the configuration files type:"
	@echo "				     > make install_conf"


../common/src/sensornavigator.o: CXXFLAGS+= -fPIC
