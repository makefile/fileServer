CC	= gcc 
CFLAGS = -Wall 
SOURCES	= my_httpd.c my_httpd.h response.c httpd_util.c
OBJECTS	= ${SOURCES:.c=.o}

OUT	= myHttpd
#LIBS	= -lungif -L/usr/X11R6/lib -ljpeg -lpng

all: $(OUT)
	@echo Build DONE.

$(OUT): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $(OUT) $(OBJECTS) $(LIBS)

clean:
	rm -f $(OBJECTS)  $(OUT)

distclean: clean
