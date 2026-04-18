include Makefile.inc

.PHONY: all clean libmme slac slac_test_pev app

APP_BIN=main
APP_OBJ=main.o

# Serial order so `make -j` does not start slac before libmme.a exists.
all: app

libmme:
	$(MAKE) -C iso15118_3/libmme all

slac: libmme
	$(MAKE) -C iso15118_3/slac all

slac_test_pev: slac
	$(MAKE) -C iso15118_3/slac_test_pev all

$(APP_OBJ): main.c
	$(CC) -Wall -o $(APP_OBJ) -c main.c -I iso15118_3/slac_test_pev

app: slac
	$(CC) -Wall -o $(APP_BIN) main.c iso15118_3/slac_test_pev/iso15118_3_slac.c iso15118_3/interfaces/c_api/slac_api.c \
	-I iso15118_3/slac_test_pev -I iso15118_3/slac/inc -I iso15118_3/libmme/inc -I iso15118_3/interfaces/c_api \
	-static -L iso15118_3/slac -L iso15118_3/libmme iso15118_3/slac/pev.a iso15118_3/libmme/libmme.a -lrt -lmme -lpthread

clean:
	rm -f $(APP_BIN) $(APP_OBJ)
	$(MAKE) -C iso15118_3/slac_test_pev clean
	$(MAKE) -C iso15118_3/slac clean
	$(MAKE) -C iso15118_3/libmme clean
