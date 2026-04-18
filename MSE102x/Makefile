include Makefile.inc

SUBDIRS := libmme \
           slac \
           slac_test_pev \
           slac_test_evse

TOPTARGETS := all clean

$(TOPTARGETS): $(SUBDIRS)
$(SUBDIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)

.PHONY: $(TOPTARGETS) $(SUBDIRS)
