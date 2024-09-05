CC=g++
CFLAGS=-Wall -Wpedantic -Wextra

run: lc3sim
	@lc3as example/example.asm >/dev/null
	@./lc3sim example/example.asm

watch:
	@clear
	@$(MAKE) --no-print-directory run
	@reflex --decoration=none -r '.*\.(cpp|h)|.*\.asm' -- zsh -c \
		'clear; sleep 0.2; $(MAKE) --no-print-directory run'

lc3sim: lc3sim.cpp
	$(CC) $(CFLAGS) -o lc3sim lc3sim.cpp

