LIB = libcli.so

CC = c++
DEBUG = -g
#OPTIM = -O3
CPPFLAGS += $(DEBUG) $(OPTIM) -Wall -fPIC
LDFLAGS += -shared -Wl,-soname,$(LIB).$(MAJOR).$(MINOR)
LIBPATH += -L.
LIBS = -lcrypt

OBJS = filter.o debug.o libcli.o


all: $(LIB) clitest
# clitest

$(LIB): $(OBJS)
	$(CC) -o $(LIB) $^ $(LDFLAGS) $(LIBS)

%.o: %.cxx %.hxx
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -o $@ -c $<

libcli.o: command.hxx common.hxx libcli.cxx libcli.hxx 

clitest.o:	clitest.cxx
	$(CC) $(CPPFLAGS) $(CFLAGS) -fPIC -o $@ -c $<

clitest: clitest.o $(OBJS)
	$(CC) $(CPPFLAGS) $(CFLAGS) -o $@ $< $(OBJS) $(LIBS)

clean:
	-rm -f *.o $(LIB)* clitest

