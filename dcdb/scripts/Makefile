include ../config.mk

.PHONY : clean install all

all:

install:
	@echo "Installing scripts"
	@echo "install SysV init.d script"
	@mkdir -p $(DCDBDEPLOYPATH)/etc/init.d/
	@sed -e 's|%DCDB_PREFIX%|'$(DCDBDEPLOYPATH)'|g' dcdb_init > $(DCDBDEPLOYPATH)/etc/init.d/dcdb
	@chmod +x $(DCDBDEPLOYPATH)/etc/init.d/dcdb

	@echo "install systemd scripts"
	@mkdir -p $(DCDBDEPLOYPATH)/etc/systemd/system
	@sed -e 's|%DCDB_PREFIX%|'$(DCDBDEPLOYPATH)'|g' cassandra.service > $(DCDBDEPLOYPATH)/etc/systemd/system/cassandra.service
	@sed -e 's|%DCDB_PREFIX%|'$(DCDBDEPLOYPATH)'|g' collectagent.service > $(DCDBDEPLOYPATH)/etc/systemd/system/collectagent.service
	@sed -e 's|%DCDB_PREFIX%|'$(DCDBDEPLOYPATH)'|g' pusher.service > $(DCDBDEPLOYPATH)/etc/systemd/system/pusher.service
	
	@echo "install dcdbplot"
	@install -m 755 dcdbplot $(DCDBDEPLOYPATH)/bin/dcdbplot

	@echo "install dcdb.bash"
	@sed -e 's|%DCDB_PREFIX%|'$(DCDBDEPLOYPATH)'|g' dcdb.bash > $(DCDBDEPLOYPATH)/dcdb.bash
