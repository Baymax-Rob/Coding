## Compiler to use (modify this for cross compile).
CC = $(CROSS_COMPILE)gcc
## Other tools you need to modify for cross compile (static lib only).
AR = $(CROSS_COMPILE)ar
RANLIB = $(CROSS_COMPILE)ranlib

## Uncomment this to build tools using static version of the library.
## Mostly useful for embedded platforms without ldd, or to create
## a local version (non-root).
BUILD_STATIC = y

## Uncomment this to strip binary from symbols. This reduce binary size.
## by a few percent but make debug worse...
BUILD_STRIPPING = n

CFLAGS :=
CFLAGS += $(INC_SRCH_PATH) $(LIB_SRCH_PATH) -g

# Stripping or not ?
ifdef BUILD_STRIPPING
  CFLAGS += -Wl,-s
endif

