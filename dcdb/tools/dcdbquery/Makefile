include ../../config.mk

CXXFLAGS +=	-I../../common/include/ -I../../lib/include -I$(DCDBDEPLOYPATH)/include
OBJS =		dcdbquery.o query.o
LIBS =		-L../../lib -L$(DCDBDEPLOYPATH)/lib -ldcdb -lcassandra -luv -lboost_random -lboost_system -lboost_date_time -lboost_regex -lssl -lcrypto -lpthread
TARGET =	dcdbquery

.PHONY : clean install

$(TARGET): $(OBJS)
	$(CXX) -o $(TARGET) $(OBJS) $(LIBS)

all:	$(TARGET)

clean:
	rm -f $(TARGET)
	rm -f $(OBJS)

install: $(TARGET)
	install $(TARGET) $(DCDBDEPLOYPATH)/bin/
