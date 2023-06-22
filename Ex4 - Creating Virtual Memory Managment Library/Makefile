CC=g++
CXX=g++
AR=ar
RANLIB=ranlib

LIBSRC=VirtualMemory.cpp
LIBHDR=VirtualMemory.h MemoryConstants.h
LIBOBJ=$(LIBSRC:.cpp=.o)

INCS=-I.
CFLAGS = -Wall -std=c++11 -pthread -g $(INCS)
CXXFLAGS = -Wall -std=c++11 -pthread -g $(INCS)

VMLIB = libVirtualMemory.a

TAR=tar
TARFLAGS=-cvf
TARNAME=ex4.tar
TARSRCS=VirtualMemory.cpp README Makefile

TARGETS = $(VMLIB) $(TARNAME)

all: $(TARGETS)

$(VMLIB): $(LIBOBJ)
	$(AR) rcs $@ $^
	$(RANLIB) $@

clean:
	$(RM) $(TARGETS) $(LIBOBJ)

depend:
	makedepend -- $(CFLAGS) -- $(LIBSRC)

$(TARNAME): $(TARSRCS)
	$(TAR) $(TARFLAGS) $@ $^

.PHONY: all clean depend
