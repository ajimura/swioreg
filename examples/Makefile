# Makefile for sw

CFLAGS += -Wall -I../include/
LDLIBS += ../lib/swio_lib.o

APS  = swio_reset swio_read swio_show swio_done

.c.o:
	$(CC) $(CFLAGS) -c $*.c -o $@

## Aps

aps : $(APS)

#
clean:
	rm -f *.o *~ $(APS)

