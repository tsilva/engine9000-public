MEGA_TESTS=test-megasprite test-megamemview test-megapalette test-megastepping test-megasavestate
MEGA_REMAKE=remake-test-megasprite remake-test-megamemview remake-test-megapalette remake-test-megastepping remake-test-megasavestate

MEGA_TEST_ROM=./tests/mega/champa/build/out.bin
MEGA_TEST_ELF=./tests/mega/champa/build/out.elf
MEGA_TEST_SOURCE=./tests/mega/champa
MEGA_TEST_ARGS=--megadrive --source-dir=$(MEGA_TEST_SOURCE) --elf=$(MEGA_TEST_ELF) --rom=$(MEGA_TEST_ROM)
MEGA_TEST_ASSETS=tests/mega/champa/build/out.elf tests/mega/champa/build/out.bin

# makers

make-test-megasprite: all $(MEGA_TEST_ASSETS)
	./e9k-debugger --volume=0  $(MEGA_TEST_ARGS) --make-test tests/results/mega/sprite

make-test-megamemview: all $(MEGA_TEST_ASSETS)
	./e9k-debugger --volume=0 $(MEGA_TEST_ARGS) --make-test tests/results/mega/memview

make-test-megapalette: all $(MEGA_TEST_ASSETS)
	./e9k-debugger --volume=0 $(MEGA_TEST_ARGS) --make-test tests/results/mega/palette

make-test-megastepping: all $(MEGA_TEST_ASSETS)
	./e9k-debugger --volume=0 $(MEGA_TEST_ARGS) --make-test tests/results/mega/stepping

make-test-megasavestate: all $(MEGA_TEST_ASSETS)
	./e9k-debugger --volume=0 $(MEGA_TEST_ARGS) --make-test tests/results/mega/savestate


# remakers

remake-test-megasprite: all $(MEGA_TEST_ASSETS)
	@printf "MEGA SPRITE DEBUG ($@) ..."
	./e9k-debugger --volume=0  $(MEGA_TEST_ARGS) --remake-test tests/results/mega/sprite

remake-test-megamemview: all $(MEGA_TEST_ASSETS)
	@printf "MEGA MEMORY VIEW ($@) ..."
	./e9k-debugger --volume=0  $(MEGA_TEST_ARGS) --remake-test tests/results/mega/memview

remake-test-megapalette: all $(MEGA_TEST_ASSETS)
	@printf "MEGA PALETTE DEBUG ($@) ..."
	./e9k-debugger --volume=0  $(MEGA_TEST_ARGS) --remake-test tests/results/mega/palette

remake-test-megastepping: all $(MEGA_TEST_ASSETS)
	@printf "MEGA STEPPING ($@) ..."
	./e9k-debugger --volume=0  $(MEGA_TEST_ARGS) --remake-test tests/results/mega/stepping

remake-test-megasavestate: all $(MEGA_TEST_ASSETS)
	@printf "MEGA SAVE STATE ($@) ..."
	./e9k-debugger --volume=0  $(MEGA_TEST_ARGS) --remake-test tests/results/mega/savestate


# testers


test-megasprite: all $(MEGA_TEST_ASSETS)
	@printf "MEGA SPRITE DEBUG ($@) ..."
	@./e9k-debugger --volume=0 $(HEADLESS)  $(MEGA_TEST_ARGS) --test tests/results/mega/sprite >> test.log 2>&1
	@echo "PASSED ✅"

test-megamemview: all $(MEGA_TEST_ASSETS)
	@printf "MEGA MEMORY VIEW ($@) ..."
	@./e9k-debugger --volume=0 $(HEADLESS)  $(MEGA_TEST_ARGS) --test tests/results/mega/memview >> test.log 2>&1
	@echo "PASSED ✅"

test-megapalette: all $(MEGA_TEST_ASSETS)
	@printf "MEGA PALETTE DEBUG ($@) ..."
	@./e9k-debugger --volume=0 $(HEADLESS)  $(MEGA_TEST_ARGS) --test tests/results/mega/palette >> test.log 2>&1
	@echo "PASSED ✅"

test-megastepping: all $(MEGA_TEST_ASSETS)
	@printf "MEGA STEPPING ($@) ..."
	@./e9k-debugger --volume=0 $(HEADLESS)  $(MEGA_TEST_ARGS) --test tests/results/mega/stepping  >> test.log 2>&1
	@echo "PASSED ✅"

test-megasavestate: all $(MEGA_TEST_ASSETS)
	@printf "MEGA SAVE STATE ($@) ..."
	@./e9k-debugger --volume=0 $(HEADLESS)  $(MEGA_TEST_ARGS) --test tests/results/mega/savestate >> test.log 2>&1
	@echo "PASSED ✅"

# assets

tests/mega/champa/build/out.elf tests/mega/champa/build/out.bin &:
	make -C ./tests/mega/champa/
