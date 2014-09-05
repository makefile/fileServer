CC	= gcc 
CFLAGS = -Wall #-g
SOURCES	= my_httpd.c my_httpd.h response.c upload.c httpd_util.c
OBJECTS	= ${SOURCES:.c=.o}

OUT	= myHttpd
#LIBS	= -lungif -L/usr/X11R6/lib 

all: $(OUT)
	@echo Build DONE.

$(OUT): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $(OUT) $(OBJECTS) $(LIBS)

clean:
	rm -f $(OBJECTS)  $(OUT)

distclean: clean
