include ../../config.mk

CXXFLAGS +=	-I../../common/include/ -I../../lib/include -I$(DCDBDEPLOYPATH)/include
OBJS =		dcdbconfig.o sensoraction.o dbaction.o jobaction.o useraction.o
LIBS =		-L../../lib -L$(DCDBDEPLOYPATH)/lib -ldcdb -lcassandra -luv -lboost_random -lboost_system -lboost_date_time -lboost_regex -lssl -lcrypto -lpthread
# GCC 4.8 is broken
ifeq ($(findstring 4.8, $(shell $(CXX) --version)), 4.8)
SLIBS =         $(DCDBDEPLOYPATH)/lib/libboost_random.a $(DCDBDEPLOYPATH)/lib/libboost_system.a
else
SLIBS =
endif
TARGET =	dcdbconfig

.PHONY : clean install

$(TARGET): $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS) $(SLIBS)

all:	$(TARGET)

clean:
	rm -f $(TARGET)
	rm -f $(OBJS)

install: $(TARGET)
	install $(TARGET) $(DCDBDEPLOYPATH)/bin/
