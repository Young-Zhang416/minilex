CC = gcc
CFLAGS = -Wall -O2
SRC = src/main.c
TARGET = minilex

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) *.o