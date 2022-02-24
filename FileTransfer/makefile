CC = gcc
CFLAGS = -c -Wall -pthread -lm -lrt 
LDFLAGS = -lpthread -lm -lrt
TARGETS = client server
COMMON_DIR = common
COMMON_OBJS = common.o
CLIENT_OBJS = client.o
SERVER_OBJS = server.o
CLIENT_DIR = file-client
SERVER_DIR = file-server
BIN_DIR = bin

all: $(TARGETS) move

move:
	mkdir -p $(BIN_DIR)
	mv *.o $(TARGETS) $(BIN_DIR)

%.o: $(COMMON_DIR)/%.c
	$(CC) $(CFLAGS) $^ -o $@

server: $(SERVER_OBJS) $(COMMON_OBJS)
	$(CC) $(SERVER_OBJS) $(COMMON_OBJS) -o server $(LDFLAGS)

client: $(CLIENT_OBJS) $(COMMON_OBJS)
	$(CC) $(CLIENT_OBJS) $(COMMON_OBJS) -o client $(LDFLAGS)

%.o: $(CLIENT_DIR)/%.c
	$(CC) $(CFLAGS) $^ -o $@

%.o: $(SERVER_DIR)/%.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf $(BIN_DIR)/*