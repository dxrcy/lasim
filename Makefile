CC=g++
CFLAGS=-Wall -Wpedantic -Wextra

TARGET=lasim

.PHONY: run watch test

main:
	$(CC) $(CFLAGS) src/main.cpp -o $(TARGET)

name=hello_world
run: main
	@./$(TARGET) examples/$(name).asm

watch:
	@clear
	@reflex --decoration=none -r 'src/.*|.*\.asm' -s -- zsh -c \
		'clear; sleep 0.2; $(MAKE) --no-print-directory run'

test:
	tests/assemble.sh

