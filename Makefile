CC=gcc
CFLAGS=-Wall -Werror -g
DEL=rm -f
TARGET = a.out

all: $(TARGET)

minimal: $(TARGET) tidy

fresh: clean $(TARGET)

$(TARGET): main.o script_parser.o script_error.o script_builtins.o
	$(CC) $(CFLAGS) $^ -o $@ 

main.o: main.c script_error.h script_parser.h script_builtins.h script_stddefs.h
	$(CC) -c $(CFLAGS) $< -o $@

script_parser.o: script_parser.c script_parser.h script_error.h
	$(CC) -c $(CFLAGS) $< -o $@

script_error.o: script_error.c script_error.h script_parser.h
	$(CC) -c $(CFLAGS) $< -o $@

script_builtins.o: script_builtins.c script_builtins.h
	$(CC) -c $(CFLAGS) $< -o $@

tidy:
	-$(DEL) *.o

clean:
	-$(DEL) *.o *.out *.log *.exe *.hex *.bin