CC=g++
CFLAGS=-Wall -Wpedantic -Wextra

TARGET=lc3sim

main:
	$(CC) $(CFLAGS) src/main.cpp -o $(TARGET)

run: main
	@lc3as example/example.asm >/dev/null
	@./lc3sim example/example.obj

watch:
	@clear
	@$(MAKE) --no-print-directory run
	@reflex --decoration=none -r 'src/.*|.*\.asm' -s -- zsh -c \
		'clear; sleep 0.2; $(MAKE) --no-print-directory run'

