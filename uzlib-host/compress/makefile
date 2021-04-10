target  = compress
objects = tgzip.o
libs    = ../../lib/libtinf.a

COPT    = -Os
CFLAGS  = -Wall -I../../src $(COPT)
LDFLAGS = $(CFLAGS) -s -Wl,-Map,ld.map

.PHONY: all clean

all: $(target)

$(target): $(objects) $(libs)
	$(CC) $(LDFLAGS) -o $@ $^ $(libs)

%.o : %.c
	$(CC) $(CFLAGS) -c $<

clean:
	$(RM) $(objects) $(target)
