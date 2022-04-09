include ../config.mk

PROJECTS =	dcdbconfig dcdbquery dcdbunitconv dcdbcsvimport dcdbslurmjob

.PHONY : clean install $(PROJECTS)

all:	$(PROJECTS)

$(PROJECTS):
	@$(MAKE) -C $@

clean:
	@$(foreach p,$(PROJECTS),$(MAKE) -C $(p) clean;)

install:
	@$(foreach p,$(PROJECTS),$(MAKE) -C $(p) install;)
