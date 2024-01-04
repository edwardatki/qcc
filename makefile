.DEFAULT_GOAL := qcc

#Build
qcc: src/*
	gcc src/*.c -o qcc

# Test all
test: clean $(addprefix  test_, $(basename $(notdir $(wildcard tests/*.c))))
	cd tools; ./test_summary.sh

clean:
	rm -r tests/build
	rm -r tests/results

# Compiler test
tests/build/%.asm: tests/%.c qcc
	mkdir -p tests/build
	cd tests; ../qcc $(notdir $<) -o build/$(notdir $(basename $<)).asm

# Assemble test
tests/build/%.bin: tests/build/%.asm
	customasm $< tools/architecture.asm -f binary -o tests/build/$(notdir $(basename $<)).bin

# Run test
test_%: tests/build/%.bin
	mkdir -p tests/results
	rm -f tests/results/$(notdir $(basename $<)).pass
	touch tests/results/$(notdir $(basename $<)).fail
	./tools/emulator tests/build/$(notdir $(basename $<)).bin -n && rm -f tests/results/$(notdir $(basename $<)).fail && touch tests/results/$(notdir $(basename $<)).pass || true
