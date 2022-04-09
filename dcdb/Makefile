include config.mk

SUB_DIRS = lib collectagent dcdbpusher analytics grafana tools scripts
PUBHEADERS = dcdbdaemon.h
.PHONY : info all clean cleanall cleandoc distclean deps depsinstall $(SUB_DIRS) doc

all: deps $(foreach s,$(SUB_DIRS),$(s)-build)
include dependencies.mk

info:
	@echo ""
	@echo "DCDB - Data Center Database"
	@echo "Buildsystem description"
	@echo ""
	@echo "Use the DCDBDEPSPATH environment variable to specify a build path for the"
	@echo "dependencies. Currently, the project dependenceis will be built to:"
	@echo "$(DCDBDEPSPATH)"
	@echo ""
	@echo "Use the DCDBDEPLOYPATH environment variable to specifiy the path for deployment"
	@echo "of all tools. Currently, the directory for deployment/staging is:"
	@echo "$(DCDBDEPLOYPATH)"
	@echo ""
	@echo "There should only be a few requirements to build DCDB."
	@echo "On Ubuntu, these are: build-essential, cmake, libssl-dev"
	@echo "To run Cassandra, you should also have a JAVA runtime installed."
	@echo ""
	@echo "To modify the number of parallel build threads, you may set the MAKETHREADS"
	@echo "environment variable. Currently, your system is configured to execute a"
	@echo "parallel build with $(MAKETHREADS) threads."
	@echo ""
	@echo "To start, please type:"
	@echo "     make all"
	@echo ""
	@echo "For ARM cross compilation, please type:"
	@echo "     make ARCH=arm CROSS_COMPILE=<X> all"
	@echo "where <X> is your cross copmlier prefix ( e.g. arm-linux-gnueabihf- )"

clean:
	@$(foreach s,$(SUB_DIRS),echo "Cleaning $(s)..." && make -C $(s) clean && echo;)

cleandeps:
	@$(foreach f,$(DISTFILESPATHS_FULL),echo "Cleaning $(f)..." && rm -rf $(DCDBDEPSPATH)/$(f) && echo;)
	
cleandoc:
	@echo "Cleaning doc/html..."
	@rm -rf doc/html && rm doc/dcdb_documentation.html && rm doc/dox_stderr.txt

cleanall: clean cleandeps cleandoc

distclean: clean
	@echo "Wiping dependencies..."
	@rm -rf $(DCDBDEPSPATH)

mrproper: distclean
	$(eval U := $(shell whoami))
	@if [ "$(U)" = "root" ]; then echo "Sorry, I won't allow you to use mrproper as root."; exit 1; fi
	@echo ""
	@echo "Wiping installation directory..."
	@rm -rf $(DCDBDEPLOYPATH)
	
doc:
	@echo "Generating doxygen HTML documentation..."
	@cd doc/ && doxygen Doxyfile && ln -sf ./html/index.html ./dcdb_documentation.html
	@echo "Generated docs into doc/html"

install: depsinstall $(SUB_DIRS)
	@cd common/include && install $(PUBHEADERS) $(DCDBDEPLOYPATH)/include/dcdb && cd ..
	@echo DONE

deps: $(foreach f,$(DISTFILESPATHS),$(DCDBDEPSPATH)/$(f)/.built)

depsinstall: $(foreach f,$(DISTFILESPATHS),$(DCDBDEPSPATH)/$(f)/.installed)

%-build: deps 
	@echo "Building $*"
	@$(MAKE) -j $(MAKETHREADS) CC=$(CC) CXX=$(CXX) -C $*

$(SUB_DIRS): $(foreach s,$(SUB_DIRS),$(s)-build) 
	@echo "Installing $@"
	@$(MAKE) CC=$(CC) CXX=$(CXX) -C $@ install

$(PROJECTS): deps
	echo ""; \
	echo "Building $@..."; \
	$(MAKE) -j $(MAKETHREADS) CC=$(CC) CXX=$(CXX) -C $@ ; \
	echo "Installing $@..."; \
	$(MAKE) CC=$(CC) CXX=$(CXX) -C $@ install ; \
