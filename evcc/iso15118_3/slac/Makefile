BASE_DIR := ../..
include $(BASE_DIR)/Makefile.inc

PEV_A_BIN=pev.a
OBJPATH=obj
SRCPATH=src

LIBMME_DIR = ../libmme
LIBMME_SO_BIN = $(LIBMME_DIR)/libmme.so
LIBMME_A_BIN = $(LIBMME_DIR)/libmme.a

RESPONSE_FILE=extra_flags

INCLUDES = -Iinc -I$(BASE_DIR)/libmme/inc
CFLAGS += $(INCLUDES) -Wall
LDFLAGS += -static -L../libmme -lmme -lrt -lpthread

EXTRA_CFLAGS= $(INCLUDES) -MMD -Wall


SRCS=$(subst $(SRCPATH)/,,$(wildcard $(SRCPATH)/*.c))
OBJS=$(addprefix $(OBJPATH)/,$(SRCS:.c=.o))
DEPS=$(patsubst %o,%d,$(OBJS))


all: $(PEV_A_BIN)

$(PEV_A_BIN): $(OBJS) $(LIBMME_A_BIN)
	$(AR) cr $@ $(OBJPATH)/pev.o $(OBJPATH)/pev_slac.o \
$(OBJPATH)/nodeListOperation.o $(OBJPATH)/timerOperation.o \
$(OBJPATH)/mstar_vs.o

$(OBJPATH)/%.o: $(SRCPATH)/%.c
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJS): | $(OBJPATH)

$(OBJPATH):
	mkdir $(OBJPATH)

-include $(DEPS)

.PHONY: all clean

clean:
	rm -f $(OBJS) $(DEPS) $(BIN)
	rmdir $(OBJPATH)
