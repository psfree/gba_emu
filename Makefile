IDIR = ./include
CC=g++
CFLAGS=-g -std=c++11 -I$(IDIR)
ODIR=obj

_DEPS = alu.hpp loguru.hpp cpu.hpp mmu.hpp
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = alu.o loguru.o cpu.o thumb.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)
	
gba: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o