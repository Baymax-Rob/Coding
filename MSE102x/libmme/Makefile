BASE := ..
include $(BASE)/Makefile.inc

STAT_LIB=libmme.a
DYN_LIB=libmme.so
OBJPATH=obj
SRCPATH=src
INCPATH=inc

INCLUDES = -I$(INCPATH)
CFLAGS += $(INCLUDES) -Wall

SRCS=$(subst $(SRCPATH)/,,$(wildcard $(SRCPATH)/*.c))
DYN_OBJS=$(addprefix $(OBJPATH)/,$(SRCS:.c=.dyn.o))
STAT_OBJS=$(addprefix $(OBJPATH)/,$(SRCS:.c=.stat.o))
DYN_DEPS=$(patsubst %o,%d,$(DYN_OBJS))
STAT_DEPS=$(patsubst %o,%d,$(STAT_OBJS))
MMTYPES=$(INCPATH)/mmtypes.h

all: $(STAT_LIB) $(DYN_LIB)

$(STAT_LIB): $(STAT_OBJS)
	$(AR) cr $@ $(STAT_OBJS)

$(DYN_LIB): $(DYN_OBJS)
	$(CC) -shared -fPIC -o $@ $(DYN_OBJS)

$(OBJPATH)/%.stat.o: $(SRCPATH)/%.c $(MMTYPES)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -o $@ -c $<

$(OBJPATH)/%.dyn.o: $(SRCPATH)/%.c $(MMTYPES)
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -fPIC -o $@ -c $<

$(DYN_OBJS) $(STAT_OBJS): | $(OBJPATH)

$(OBJPATH):
	mkdir $(OBJPATH)

-include $(STAT_DEPS) $(DYN_DEPS)

.PHONY: all clean

clean:
	rm -f $(DYN_OBJS) $(STAT_OBJS) \
		$(STAT_DEPS) $(DYN_DEPS) \
		$(DYN_LIB) $(STAT_LIB)
	rmdir $(OBJPATH)
