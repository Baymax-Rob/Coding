OBJPATH=obj

all: slac_test_pev

slac_test_pev: slac_test_pev.o
	gcc -o ./obj/slac_pev ./obj/slac_test_pev.o -static -L ../slac/ -L ../libmme/ \
	../slac/pev.a ../libmme/libmme.a -lrt -lmme -lpthread

slac_test_pev.o: | $(OBJPATH)
	gcc -Wall -o ./obj/slac_test_pev.o -c slac_test_pev.c -I ../slac/inc \
    -I ../libmme/inc

$(OBJPATH):
	mkdir $(OBJPATH)

.PHONY: all clean

clean:
	rm -f ./obj/*
	rmdir $(OBJPATH)
