CC = wcc
CCFLAGS = -bt=dos -zq -wx -ml -i=..\..\pdcurses -oneatx
LINK = wlink
LDFLAGS = op q sys dos
LIBCURSES = ..\..\pdcurses\dos\pdcurses.lib

all: ..\csol.exe

clean:
	-del *.obj
	-del ..\csol.exe

.c.obj: .autodepend
	$(CC) $(CCFLAGS) $<

..\csol.exe: card.obj game.obj main.obj rc.obj theme.obj ui.obj util.obj
	$(LINK) $(LDFLAGS) n $@ f *.obj l $(LIBCURSES)
