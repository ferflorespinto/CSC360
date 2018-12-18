.phony all:
all:
	gcc diskfunctions.c diskinfo.c -o diskinfo
	gcc diskfunctions.c disklist.c -o disklist
	gcc diskfunctions.c diskget.c -o diskget
	gcc diskfunctions.c diskput.c -o diskput

.PHONY clean:
clean:
	-rm diskinfo
	-rm disklist
	-rm diskget
	-rm diskput
