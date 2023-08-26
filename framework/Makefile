CC=gcc
INCDIR=../external/include
OBJDIR=../build/objs
CFLAGS=-I$(INCDIR) -Wall
OBJECTS=$(addprefix $(OBJDIR)/, $(addsuffix .o, $(filter-out Makefile,$(wildcard *))))

.PHONY: clean

all: $(OBJECTS)

$(OBJDIR)/%.o: %/*.c %/*.h
	$(CC) -c $(filter %.c, $^) -o $@ $(CFLAGS) 
	
clean: 
	-rm $(OBJECTS)