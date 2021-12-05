#Makefile
CC = gcc
CFLAG = -W -Wall
TARGET = langdump 
OBJECTS = langdump.c
all = $(TARGET)
$(TARGET) : $(OBJECTS)
	$(CC) $(CFLAG) $^ -o $@
clean :
	rm langdump
