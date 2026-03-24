# --- Settings ---
CC = gcc
CFLAGS = -Wall -Wextra -O2
LIBS = -lX11 -lm -lpthread -ldl
TARGET = xteroids
SRCS = xteroids.c graphics.c
ZIG_TARGET = x86_64-linux-gnu.2.17
RELEASE_NAME = $(TARGET)-linux-x86_64

# --- Rules ---
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LIBS)

# 1. Compile for glibc 2.17 using Zig
release: clean
	zig cc -target $(ZIG_TARGET) $(CFLAGS) -O3 $(SRCS) -o $(RELEASE_NAME) $(LIBS) \
	-I/usr/include -L/usr/lib64 -Wno-macro-redefined -s

# 2. Package binary + assets into a single tarball
dist: release
	@echo "Packaging for distribution..."
	tar -czvf $(RELEASE_NAME).tar.gz $(RELEASE_NAME) assets/
	@echo "------------------------------------------------"
	@echo "SUCCESS: Created $(RELEASE_NAME).tar.gz"
	@echo "------------------------------------------------"

clean:
	rm -f $(TARGET) $(RELEASE_NAME) $(RELEASE_NAME).tar.gz *.o

.PHONY: all clean release dist

