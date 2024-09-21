CC=g++
CFLAGS=-Wall -Wpedantic -Wextra

TARGET=lasim
BINDIR = /usr/local/bin

.PHONY: install run watch test clean

$(TARGET): src
	$(CC) $(CFLAGS) src/main.cpp -o $(TARGET)

install:
	sudo install -m 755 $(TARGET) $(BINDIR)

name=hello_world
run: $(TARGET)
	@./$(TARGET) examples/$(name).asm

watch:
	@clear
	@reflex --decoration=none -r 'src/.*|.*\.asm' -s -- zsh -c \
		'clear; sleep 0.2; $(MAKE) --no-print-directory run'

test: $(TARGET)
	@if [ -d 'tests/out' ]; \
		then rm -rf tests/out/*; \
		else mkdir tests/out; \
	fi
	tests/assemble.sh
	tests/branch.sh
	tests/jump.sh
	tests/arith.sh
	tests/memory.sh
	$(CC) $(CFLAGS) tests/test.cpp -o tests/out/test.bin
	tests/test.cpp.sh

clean:
	rm -f ./lasim
	rm -f examples/*.{obj,sym,lc3}
	rm -rf tests/out/*

