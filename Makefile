# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
LDFLAGS = -lcurl -lm
STRIP = strip
UNAME_S := $(shell uname)
ifeq ($(UNAME_S),Darwin)
  STRIP_FLAGS = -x
else
  STRIP_FLAGS = -s
endif

# Source and object files
SRCS = youtube_transcript.c cJSON.c
OBJS = $(SRCS:.c=.o)
TARGET = youtube_transcript

# Default target
all: $(TARGET)

# Link the object files to create the executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	-$(STRIP) $(STRIP_FLAGS) $(TARGET)

# Compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets
.PHONY: all clean
