CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wformat=2 -Wformat-overflow=2 \
         -Wformat-truncation=2 -Wformat-security -Wnull-dereference -Wstack-protector \
         -Wtrampolines -Walloca -Wvla -Warray-bounds=2 -Wimplicit-fallthrough=3 \
         -Wshift-overflow=2 -Wcast-qual -Wstringop-overflow=4 -Warith-conversion \
         -Wlogical-op -Wduplicated-cond -Wduplicated-branches -Wstrict-prototypes \
         -Wmissing-prototypes -Wundef -Wstrict-overflow=4 -Wswitch-default \
         -Wswitch-enum -Wcast-align=strict -O0 -ggdb3 -Isrc -I/usr/include/sfsexp \
				 -I./external/pl_mpeg -I./external/log/src

LDFLAGS = -lSDL3 -lSDL3_mixer -lsexp

TARGET = qyst
SRC = src/qyst.c
OBJ = $(SRC:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
