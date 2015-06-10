####### options

TARGET        = a.out

CC            = gcc
CFLAGS        = -g -Wall

INCPATH       = 

LIBS          = 


####### Files

SOURCES       = myshell.c

OBJECTS       = $(SOURCES:.c=.o)


####### Build rules

all: Makefile $(TARGET)

$(TARGET):  $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LIBS)

OUTPUT_OPTION = -o $@

COMPILE.c = $(CC) $(CFLAGS) $(INCPATH) -c

%.o: %.c
	$(COMPILE.c) $(OUTPUT_OPTION) $<


include $(SOURCES:.c=.d)

%.d: %.c
	set -e; rm -f $@; \
	$(CC) -MM $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

clean:
	-rm ./$(TARGET) ./*.o ./*.d
.PHONY: clean
