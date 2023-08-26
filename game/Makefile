CC=gcc
INCDIR=../external/include
OBJDIR=../build/objs
CFLAGS=-I$(INCDIR) -Wall
OBJECTS=$(addprefix $(OBJDIR)/, $(addsuffix .o, $(filter-out Makefile,$(wildcard *))))
FRAME=$(addprefix $(OBJDIR)/, $(addsuffix .o, $(filter-out Makefile,$(notdir $(wildcard ../framework/*)))))
TOOLS=$(addprefix $(OBJDIR)/, $(addsuffix .o, $(filter-out Makefile,$(notdir $(wildcard ../tools/*)))))

.PHONY: clean

all: $(OBJECTS)

$(OBJDIR)/%.o: %/*.c %/*.h $(FRAME) $(TOOLS)
	$(CC) -c $(filter %.c, $^) -o $@ $(CFLAGS) 

clean: 
	-rm $(OBJECTS)