CC=g++
CFLAGS=-Wall -Wpedantic -Wextra

TARGET=lc3

.PHONY: example run watch

main:
	$(CC) $(CFLAGS) src/main.cpp -o $(TARGET)

example: main
	@lc3as examples/hello_world.asm >/dev/null
	@./lc3 -x examples/hello_world.obj

name=test
run: main
	@lc3as examples/$(name).asm >/dev/null
	@./lc3 -x examples/$(name).obj

watch:
	@clear
	@reflex --decoration=none -r 'src/.*|.*\.asm' -s -- zsh -c \
		'clear; sleep 0.2; $(MAKE) --no-print-directory run'

