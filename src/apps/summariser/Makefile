# Location of include files.
INCDIR = ../..

# Default compilers.
CC = gcc
G77 = gfortran
CFLAGS = -Wall -g -I$(INCDIR)

# Specify extensions of files to delete when cleaning.
CLEANEXTS = o

# Target file and install directory.
OUTPUTFILE = summariser
INSTALLDIR = ../../bin

# Libraries we need.
LIBDIR = ../../lib
LIBS = -latrpfits -lrpfits

# Default target.
.PHONY: all
all: $(OUTPUTFILE)

# Build the output file.
$(OUTPUTFILE): summariser.o
	$(G77) -o $@ $^ -L$(LIBDIR) $(LIBS)

.PHONY: install
install:
	mkdir -p $(INSTALLDIR)
	cp -p $(OUTPUTFILE) $(INSTALLDIR)

.PHONY: clean
clean:
	for file in $(CLEANEXTS); do rm -f *.$$file; done
	rm -f $(OUTPUTFILE)
