CC=g++
CFLAGS=-Wall -Wpedantic -Wextra

TARGET=lc3

.PHONY: example run watch

main:
	$(CC) $(CFLAGS) src/main.cpp -o $(TARGET)

example: main
	@./lc3 examples/hello_world.asm -o examples/hello_world2.obj

name=test
run: main
	@./lc3 examples/$(name).asm

watch:
	@clear
	@reflex --decoration=none -r 'src/.*|.*\.asm' -s -- zsh -c \
		'clear; sleep 0.2; $(MAKE) --no-print-directory run'

