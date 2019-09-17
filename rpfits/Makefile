# Default compilers.
CC = gcc
CFLAGS = -Wall -g

# Specify extensions of files to delete when cleaning.
CLEANEXTS = o a

# Target file and install directory.
OUTPUTFILE = libatrpfits.a
INSTALLDIR = ../lib

# Default target.
.PHONY: all
all: $(OUTPUTFILE)

# Build the library.
$(OUTPUTFILE): reader.o
	ar r $@ $^
	ranlib $@

.PHONY: install
install:
	mkdir -p $(INSTALLDIR)
	cp -p $(OUTPUTFILE) $(INSTALLDIR)

.PHONY: clean
clean:
	for file in $(CLEANEXTS); do rm -f *.$$file; done

