CC = clang
CFLAGS = -Wall -O2 -Iinclude -fobjc-arc -fmodules -miphoneos-version-min=14.0
LDFLAGS = -framework Foundation -lxpc -lSystem -lz -lcrypto -lpthread -ldl
TARGET = xpc_nexus
SRCS = src/main.c src/daemon_utils.c src/interceptor.c src/fuzzer.c src/logger.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
	@echo "Cleaned build artifacts."

install: $(TARGET)
	@echo "Copying to device... Requires SSH/SCP to Jailbroken device"
	@echo "scp $(TARGET) root@IPHONE_IP:/usr/bin/"
	@echo "ssh root@IPHONE_IP chmod +x /usr/bin/$(TARGET)"