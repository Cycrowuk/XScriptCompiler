SRC =  	adler32.c compress.c crc32.c deflate.c infback.c inffast.c inflate.c inftrees.c gzio.c trees.c uncompr.c zutil.c

OBJ = $(SRC:.c=.o)

OUT = ../libz.a

# include directories
INCLUDES = -I. -I../include/ -I/usr/local/include

# C++ compiler flags (-g -O2 -Wall)
CCFLAGS = -g

# compiler
CCC = cc

# library paths
LIBS = -L../ -L/usr/local/lib -lm

# compile flags
LDFLAGS = -g

.SUFFIXES: .c

default: dep $(OUT)

.cpp.o:
	$(CCC) $(INCLUDES) $(CCFLAGS) -c $< -o $@

$(OUT): $(OBJ)
	ar rcs $(OUT) $(OBJ)

depend: dep

dep:
	makedepend -- $(CFLAGS) -- $(INCLUDES) $(SRC)

clean:
	rm -f $(OBJ) $(OUT) Makefile.bak 

# DO NOT DELETE

adler32.o: zlib.h zconf.h
compress.o: zlib.h zconf.h
crc32.o: zutil.h zlib.h zconf.h /usr/include/stddef.h
crc32.o: /usr/include/sys/cdefs.h /usr/include/sys/_null.h
crc32.o: /usr/include/sys/_types.h /usr/include/machine/_types.h
crc32.o: /usr/include/string.h /usr/include/strings.h /usr/include/stdlib.h
crc32.o: /usr/include/errno.h /usr/include/limits.h /usr/include/sys/limits.h
crc32.o: /usr/include/machine/_limits.h /usr/include/sys/syslimits.h crc32.h
deflate.o: deflate.h zutil.h zlib.h zconf.h /usr/include/stddef.h
deflate.o: /usr/include/sys/cdefs.h /usr/include/sys/_null.h
deflate.o: /usr/include/sys/_types.h /usr/include/machine/_types.h
deflate.o: /usr/include/string.h /usr/include/strings.h /usr/include/stdlib.h
deflate.o: /usr/include/errno.h
infback.o: zutil.h zlib.h zconf.h /usr/include/stddef.h
infback.o: /usr/include/sys/cdefs.h /usr/include/sys/_null.h
infback.o: /usr/include/sys/_types.h /usr/include/machine/_types.h
infback.o: /usr/include/string.h /usr/include/strings.h /usr/include/stdlib.h
infback.o: /usr/include/errno.h inftrees.h inflate.h inffast.h inffixed.h
inffast.o: zutil.h zlib.h zconf.h /usr/include/stddef.h
inffast.o: /usr/include/sys/cdefs.h /usr/include/sys/_null.h
inffast.o: /usr/include/sys/_types.h /usr/include/machine/_types.h
inffast.o: /usr/include/string.h /usr/include/strings.h /usr/include/stdlib.h
inffast.o: /usr/include/errno.h inftrees.h inflate.h inffast.h
inflate.o: zutil.h zlib.h zconf.h /usr/include/stddef.h
inflate.o: /usr/include/sys/cdefs.h /usr/include/sys/_null.h
inflate.o: /usr/include/sys/_types.h /usr/include/machine/_types.h
inflate.o: /usr/include/string.h /usr/include/strings.h /usr/include/stdlib.h
inflate.o: /usr/include/errno.h inftrees.h inflate.h inffast.h inffixed.h
inftrees.o: zutil.h zlib.h zconf.h /usr/include/stddef.h
inftrees.o: /usr/include/sys/cdefs.h /usr/include/sys/_null.h
inftrees.o: /usr/include/sys/_types.h /usr/include/machine/_types.h
inftrees.o: /usr/include/string.h /usr/include/strings.h
inftrees.o: /usr/include/stdlib.h /usr/include/errno.h inftrees.h
gzio.o: /usr/include/stdio.h /usr/include/sys/cdefs.h
gzio.o: /usr/include/sys/_null.h /usr/include/sys/_types.h
gzio.o: /usr/include/machine/_types.h zutil.h zlib.h zconf.h
gzio.o: /usr/include/stddef.h /usr/include/string.h /usr/include/strings.h
gzio.o: /usr/include/stdlib.h /usr/include/errno.h /usr/include/stdarg.h
trees.o: deflate.h zutil.h zlib.h zconf.h /usr/include/stddef.h
trees.o: /usr/include/sys/cdefs.h /usr/include/sys/_null.h
trees.o: /usr/include/sys/_types.h /usr/include/machine/_types.h
trees.o: /usr/include/string.h /usr/include/strings.h /usr/include/stdlib.h
trees.o: /usr/include/errno.h trees.h
uncompr.o: zlib.h zconf.h
zutil.o: zutil.h zlib.h zconf.h /usr/include/stddef.h
zutil.o: /usr/include/sys/cdefs.h /usr/include/sys/_null.h
zutil.o: /usr/include/sys/_types.h /usr/include/machine/_types.h
zutil.o: /usr/include/string.h /usr/include/strings.h /usr/include/stdlib.h
zutil.o: /usr/include/errno.h
