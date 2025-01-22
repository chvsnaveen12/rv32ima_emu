all:
	@gcc *.c -w -O3 -o emu

run: all
	@./emu