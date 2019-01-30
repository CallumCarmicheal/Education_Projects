DEPS = rdwrn.h
OBJ = rdwrn.o

CC=cc
CFLAGS=-std=gnu11 -I. -Wall -g -O0 -pthread -lncurses -ggdb3

.PHONY: all
all: makedirs client server

.PHONY: makedirs
makedirs:
	@mkdir -p obj ||:
	@mkdir -p bin ||:

%.o: %.c $(DEPS)
	$(CC) -c -o obj/$@ $< $(CFLAGS)

.PHONY: client
client: $(OBJ)
	@echo "\033[96m"
	@echo "==========================="
	@echo "== Building - client ... =="
	@echo "==========================="
	@echo "\ Read documentation.docx /"
	@echo "\033[0m"
	$(CC) client.c -o bin/$@ obj/$^ $(CFLAGS)

.PHONY: server
server: $(OBJ)
	@echo "\033[96m"
	@echo "==========================="
	@echo "== Building - server ... =="
	@echo "==========================="
	@echo "\ Read documentation.docx /"
	@echo "\033[0m"
	$(CC) server.c -o bin/$@ obj/$^ $(CFLAGS)

.PHONY: clean
clean:
	@echo "\033[96m"
	@echo "======================="
	@echo "== Cleaning files... =="
	@echo "======================="
	@echo "\033[0m"
	
	$(RM) -rf obj
	$(RM) bin/client
	$(RM) bin/server
