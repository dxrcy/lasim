CC=g++
CFLAGS=-Wall -Wpedantic -Wextra

TARGET=lasim
BINDIR = /usr/local/bin

.PHONY: run watch test clean

main:
	$(CC) $(CFLAGS) src/main.cpp -o $(TARGET)

install:
	sudo install -m 755 $(TARGET) $(BINDIR)

name=hello_world
run: main
	@./$(TARGET) examples/$(name).asm

watch:
	@clear
	@reflex --decoration=none -r 'src/.*|.*\.asm' -s -- zsh -c \
		'clear; sleep 0.2; $(MAKE) --no-print-directory run'

test: main
	tests/assemble.sh

clean:
	rm -f ./lasim
	rm -f examples/*.{obj,sym}

