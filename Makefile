#Makefile
CC = gcc
CFLAG = -W -Wall
TARGET = hasse 
OBJECTS = hasse.c
all = $(TARGET)
$(TARGET) : $(OBJECTS)
	$(CC) $(CFLAG) $^ -o $@
clean :
	rm hasse
