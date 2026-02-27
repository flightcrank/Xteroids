# 1. Compiler and Flags
CC = gcc
CFLAGS = -Wall -Wextra -O2
LIBS = -lX11 -lm -lpthread -ldl

# 2. Target Name and Source Files
TARGET = xteroids
SRCS = xteroids.c graphics.c
OBJS = $(SRCS:.c=.o)

# 3. Default Rule (The one that runs when you just type 'make')
all: $(TARGET)

# 4. Linking the final executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LIBS)

# 5. Compiling individual source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 6. Clean rule to remove compiled files
clean:
	rm -f $(TARGET) $(OBJS)

# 7. Phony targets (to prevent conflicts with files named 'all' or 'clean')
.PHONY: all clean
