.DEFAULT_GOAL := main.bin

# TODO: improve this makefile
examples: main.bin examples/*.c examples/*.h
	cd examples; ../main.bin *.c

main.bin: src/*
	gcc src/*.c -o main.bin

clean:
	mkdir -p build
	rm -f build/*