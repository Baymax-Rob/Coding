OBJPATH=obj

all: slac_test_evse

slac_test_evse: slac_test_evse.o
	gcc -o ./obj/slac_evse ./obj/slac_test_evse.o -static -L ../slac/ -L ../libmme/ \
	../slac/evse.a ../libmme/libmme.a -lrt -lmme -lpthread

slac_test_evse.o: | $(OBJPATH)
	gcc -Wall -o ./obj/slac_test_evse.o -c slac_test_evse.c -I ../slac/inc \
    -I ../libmme/inc

$(OBJPATH):
		mkdir $(OBJPATH)

.PHONY: all clean

clean:
	rm -f ./obj/*
