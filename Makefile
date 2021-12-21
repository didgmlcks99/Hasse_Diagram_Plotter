#Makefile
CC = gcc
CFLAG = -W -Wall
TARGET = hasse 
OBJECTS = hasse.c
all = $(TARGET)
$(TARGET) : $(OBJECTS)
	$(CC) $(CFLAG) ezdib.c $^ -o $@ -lm
clean :
	rm hasse
