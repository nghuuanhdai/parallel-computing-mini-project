include ../config.mk

CXXFLAGS += -I../common/include/ \
            -I../lib/include \
            -I$(DCDBDEPLOYPATH)/include

OBJS =	    ../common/src/sensornavigator.o \
	    ../common/src/globalconfiguration.o \
	    ../common/src/RESTHttpsServer.o \
	    ../common/src/logging.o \
	    GrafanaServer.o \
	    RestAPI.o \
	    Configuration.o

LIBS =      -L../lib \
			-L$(DCDBDEPLOYPATH)/lib/ \
            -lpthread \
            -ldcdb \
            -lcassandra \
            -lboost_system \
            -lboost_chrono \
            -lboost_thread \
            -lboost_log \
            -lboost_regex \
            -lboost_log_setup \
            -lboost_filesystem \
            -lcrypto \
            -lssl

TARGET = grafanaserver

.PHONY : clean install

P = $(shell cd $(DCDBDEPLOYPATH)/lib/ && pwd)
U = $(shell uname)

$(TARGET): $(OBJS)
	@LD_LIBRARY_PATH=$(DCDBDEPLOYPATH)/lib:$$LD_LIBRARY_PATH \
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET)

debug: CXXFLAGS += -DDEBUG
debug: all

clean:
	rm -f $(TARGET)
	rm -f $(OBJS)

install: $(TARGET)
	install $(TARGET) $(DCDBDEPLOYPATH)/bin/
	
../common/src/sensornavigator.o: CXXFLAGS+= -fPIC
