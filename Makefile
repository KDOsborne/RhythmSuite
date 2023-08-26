DIRS=framework tools game
BUILDDIR=build
BINDIR=$(BUILDDIR)/bin
OBJDIR=$(BUILDDIR)/objs
LIBDIR=external/libs
INCDIR=external/include
EXENAME=$(BINDIR)/rhythmsuite.exe

CSOURCEFILE=main.c
CC=gcc
CFLAGS=-I$(INCDIR) -L$(LIBDIR) -Wall -mwindows

LIBS=-lWs2_32 -lComdlg32 -lopengl32 -lgdi32 -lOle32 -lImm32 -lbass -lbass_fx -l:libsodium.a
OBJECTS=$(wildcard $(OBJDIR)/*.o)

.PHONY: rhythmsuite clean $(DIRS)

all: $(DIRS) rhythmsuite

rhythmsuite: $(CSOURCEFILE)
	$(CC) $(CSOURCEFILE) $(OBJECTS) -o $(EXENAME) $(CFLAGS) $(LIBS)

$(DIRS):
	+$(MAKE) -C $@

clean:
	-make -C framework clean 
	-make -C tools clean 
	-make -C game clean
	-rm $(EXENAME)