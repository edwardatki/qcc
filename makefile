.DEFAULT_GOAL := main.bin

run: main.bin
	./main.bin test.c

main.bin: src/*
	gcc src/*.c -o main.bin

clean:
	mkdir -p build
	rm -f build/*
