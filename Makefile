CC = wcc
CCFLAGS = -bt=dos -zq -wx -ml -i=lib\pdcurses -oneatx
LINK = wlink
LDFLAGS = op q sys dos
LIBCURSES = lib\pdcurses\dos\pdcurses.lib
CSOL_SRCDIR = src
RM = del

all: csol.exe

clean
	-$(RM) *.obj
	-$(RM) csol.exe

.c: $(CSOL_SRCDIR)

.c.obj: .autodepend
	$(CC) $(CCFLAGS) $<

csol.exe: card.obj game.obj main.obj rc.obj theme.obj ui.obj util.obj
	$(LINK) $(LDFLAGS) n $@ f *.obj l $(LIBCURSES)
