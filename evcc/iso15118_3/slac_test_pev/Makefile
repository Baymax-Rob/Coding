OBJPATH=obj
API_SRC=../interfaces/c_api/slac_api.c
API_OBJ=$(OBJPATH)/slac_api.o
SLAC_TEST_OBJ=$(OBJPATH)/slac_test_pev.o

all: slac_test_pev

slac_test_pev: $(SLAC_TEST_OBJ) $(API_OBJ)
	$(CC) -o ./obj/slac_pev $(SLAC_TEST_OBJ) $(API_OBJ) -static -L ../slac/ -L ../libmme/ \
	../slac/pev.a ../libmme/libmme.a -lrt -lmme -lpthread

$(SLAC_TEST_OBJ): | $(OBJPATH)
	$(CC) -Wall -o $(SLAC_TEST_OBJ) -c iso15118_3_slac.c -I . -I ../slac/inc \
    -I ../libmme/inc -I ../interfaces/c_api

$(API_OBJ): $(API_SRC) | $(OBJPATH)
	$(CC) -Wall -o $(API_OBJ) -c $(API_SRC) -I ../slac/inc -I ../libmme/inc -I ../interfaces/c_api

$(OBJPATH):
	mkdir $(OBJPATH)

.PHONY: all clean

clean:
	rm -f ./obj/*
	rmdir $(OBJPATH)
