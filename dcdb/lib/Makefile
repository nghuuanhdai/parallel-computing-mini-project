include ../config.mk

# C++ Compiler Flags (use fPIC for our dynamic library)
CXXFLAGS += -Werror -fPIC -I./include -I./include_internal -I$(DCDBDEPLOYPATH)/include -I../common/include/

# Enable sensor cache
CXXFLAGS += -DUSE_SENSOR_CACHE

# List of object files to build and the derived list of corresponding source files
OBJS = src/connection.o \
       src/sensor.o \
       src/sensordatastore.o \
       src/timestamp.o \
       src/sensorconfig.o \
       src/sensorid.o \
       src/sensorid_num.o \
       src/unitconv.o \
       src/virtualsensor.o \
       src/c_api.o \
       src/sensoroperations.o \
       src/jobdatastore.o \
       src/calievtdatastore.o \
       src/version.o \
       src/libconfig.o

# List of public header files necessary to use this libray
PUBHEADERS = $(shell find include -type f -iname "*.h")

# List of non-public headers
PRIVHEADERS = $(shell find include_internal -type f -iname "*.h")

# External libraries to link against
LIBS = -L$(DCDBDEPLOYPATH)/lib/ -lcassandra -lboost_random -lboost_system -lboost_date_time -lboost_regex -luv -lpthread

# Dynamic library building differs between Linux/BSD and MacOS
OS = $(shell uname)
ifeq ($(OS),Darwin)
	TARGET = libdcdb.dylib
	DLFLAGS = -dynamiclib
else
	TARGET = libdcdb.so
	DLFLAGS = -shared -Wl,-soname,$(TARGET) -Wl,--no-undefined
endif

# Determine the expanded path to which the library will be installed
P =	$(shell cd $(DCDBDEPLOYPATH)/lib/ && pwd) 

# List of Phony Targets
.PHONY : check-target-env all clean install

# Main Library Target
$(TARGET): $(OBJS) .header-check
	@if [ "$(OS)" = "Darwin" ]; then \
		echo "Linking library in Mac OS style: $(TARGET)"; \
		$(CXX) $(CXXFLAGS) $(DLFLAGS) -o $(TARGET) $(OBJS) $(LIBS); \
	else \
		echo "Linking library in Linux style: $(TARGET)"; \
		$(CXX) $(CXXFLAGS) $(DLFLAGS) -o $(TARGET) $(OBJS) $(LIBS); \
	fi

all: $(TARGET) check-target-env

# Alert the user to put the necessary paths into LD_LIBRARY_PATH (or similar on other platforms)
check-target-env:
	@if [ "$(OS)" = "Darwin" ]; then \
	R=`echo $$DYLD_LIBRARY_PATH | grep $P`; \
	if [ "$$R" = "" ]; then \
	printf "\n******************************************\nPlease type the following line before running:\n  export DYLD_LIBRARY_PATH=$$DYLD_LIBRARY_PATH:$P\n******************************************\n"; \
	fi; \
	fi
	@if [ "$(OS)" = "Linux" ]; then \
	R=`echo $$LD_LIBRARY_PATH | grep $P`; \
	if [ "$$R" = "" ]; then \
	printf "\n******************************************\nPlease type the following line before running:\n  export LD_LIBRARY_PATH=$$LD_LIBRARY_PATH:$P\n******************************************\n"; \
	fi; \
	fi
	
# Header check
.header-check: $(PUBHEADERS) $(PRIVHEADERS)
	@touch .header-check
	@touch $(OBJS:.o=.cpp)

# Build the documentation
docs: $(PUBHEADERS) $(PRIVHEADERS) $(SRC)
	@echo "Creating documentation..."
	doxygen

# Clean everything
clean: clean-docs
	rm -f .header-check
	rm -f $(OBJS) $(TARGET)
	
# Clean the documentation
clean-docs:
	rm -rf docs

# Install to deploy path
install: $(TARGET) check-target-env
	mkdir -p $(DCDBDEPLOYPATH)/lib
	mkdir -p $(DCDBDEPLOYPATH)/include/dcdb
	install $(TARGET) $(DCDBDEPLOYPATH)/lib/
	install $(PUBHEADERS) $(DCDBDEPLOYPATH)/include/dcdb/
