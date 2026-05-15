Z80_SRCMAP_TEST_DIR ?= tools/z80_srcmap/tests
Z80_SRCMAP_TEST_EXE ?= tools/z80_srcmap/build/z80_srcmap
Z80_SRCMAP_TESTS = test-z80srcmap

.PHONY: test-z80srcmap test-z80srcmap-asxxxx test-z80srcmap-sjasm test-z80srcmap-sjasm-stale clean-z80srcmap-tests

test-z80srcmap: test-z80srcmap-asxxxx test-z80srcmap-sjasm test-z80srcmap-sjasm-stale
	@echo
	@echo "✅✅✅ Z80 SRCMAP TESTS PASSED ✅✅✅"

test-z80srcmap-asxxxx: $(Z80_SRCMAP_TEST_DIR)/asxxxx/build/out.z80srcmap $(Z80_SRCMAP_TEST_DIR)/asxxxx/build/expected.z80srcmap
	@printf "Z80 SRCMAP ASXXXX ($@) ..."
	@diff -u $(Z80_SRCMAP_TEST_DIR)/asxxxx/build/expected.z80srcmap $(Z80_SRCMAP_TEST_DIR)/asxxxx/build/out.z80srcmap
	@echo "PASSED ✅"

test-z80srcmap-sjasm: $(Z80_SRCMAP_TEST_DIR)/sjasm/build/out.z80srcmap $(Z80_SRCMAP_TEST_DIR)/sjasm/build/expected.z80srcmap $(Z80_SRCMAP_TEST_DIR)/sjasm/build/out.noi
	@printf "Z80 SRCMAP SJASM ($@) ..."
	@diff -u $(Z80_SRCMAP_TEST_DIR)/sjasm/build/expected.z80srcmap $(Z80_SRCMAP_TEST_DIR)/sjasm/build/out.z80srcmap
	@diff -u $(Z80_SRCMAP_TEST_DIR)/sjasm/expected.noi $(Z80_SRCMAP_TEST_DIR)/sjasm/build/out.noi
	@echo "PASSED ✅"

test-z80srcmap-sjasm-stale: $(Z80_SRCMAP_TEST_EXE) $(Z80_SRCMAP_TEST_DIR)/sjasm/program.lst $(Z80_SRCMAP_TEST_DIR)/sjasm/src/program.s80 $(Z80_SRCMAP_TEST_DIR)/sjasm/src/macros.i80 $(Z80_SRCMAP_TEST_DIR)/sjasm/build/expected.z80srcmap | $(Z80_SRCMAP_TEST_DIR)/sjasm/build
	@printf "Z80 SRCMAP SJASM STALE NOI ($@) ..."
	@cp $(Z80_SRCMAP_TEST_DIR)/sjasm/program.lst $(Z80_SRCMAP_TEST_DIR)/sjasm/build/program.lst
	@printf "DEF staleLabel 0x7777\nDEF done 0x7777\n" > $(Z80_SRCMAP_TEST_DIR)/sjasm/build/out.noi
	@$(Z80_SRCMAP_TEST_EXE) --listing-format sjasm --build-dir $(Z80_SRCMAP_TEST_DIR)/sjasm/build --source-dir $(Z80_SRCMAP_TEST_DIR)/sjasm/src --out $(Z80_SRCMAP_TEST_DIR)/sjasm/build/out.z80srcmap --out-noi $(Z80_SRCMAP_TEST_DIR)/sjasm/build/out.noi
	@diff -u $(Z80_SRCMAP_TEST_DIR)/sjasm/build/expected.z80srcmap $(Z80_SRCMAP_TEST_DIR)/sjasm/build/out.z80srcmap
	@diff -u $(Z80_SRCMAP_TEST_DIR)/sjasm/expected.noi $(Z80_SRCMAP_TEST_DIR)/sjasm/build/out.noi
	@echo "PASSED ✅"

$(Z80_SRCMAP_TEST_DIR)/asxxxx/build/out.z80srcmap: $(Z80_SRCMAP_TEST_DIR)/asxxxx/program.lst $(Z80_SRCMAP_TEST_DIR)/asxxxx/program.noi $(Z80_SRCMAP_TEST_DIR)/asxxxx/src/program.s $(Z80_SRCMAP_TEST_EXE) | $(Z80_SRCMAP_TEST_DIR)/asxxxx/build
	@cp $(Z80_SRCMAP_TEST_DIR)/asxxxx/program.lst $(Z80_SRCMAP_TEST_DIR)/asxxxx/build/program.lst
	@cp $(Z80_SRCMAP_TEST_DIR)/asxxxx/program.noi $(Z80_SRCMAP_TEST_DIR)/asxxxx/build/program.noi
	@$(Z80_SRCMAP_TEST_EXE) --build-dir $(Z80_SRCMAP_TEST_DIR)/asxxxx/build --source-dir $(Z80_SRCMAP_TEST_DIR)/asxxxx/src --out $@

$(Z80_SRCMAP_TEST_DIR)/asxxxx/build/expected.z80srcmap: $(Z80_SRCMAP_TEST_DIR)/asxxxx/expected.z80srcmap.in | $(Z80_SRCMAP_TEST_DIR)/asxxxx/build
	@sed "s|@TESTDIR@|$(abspath $(Z80_SRCMAP_TEST_DIR)/asxxxx)|g" $< > $@

$(Z80_SRCMAP_TEST_DIR)/sjasm/build/out.z80srcmap $(Z80_SRCMAP_TEST_DIR)/sjasm/build/out.noi &: $(Z80_SRCMAP_TEST_DIR)/sjasm/program.lst $(Z80_SRCMAP_TEST_DIR)/sjasm/src/program.s80 $(Z80_SRCMAP_TEST_DIR)/sjasm/src/macros.i80 $(Z80_SRCMAP_TEST_EXE) | $(Z80_SRCMAP_TEST_DIR)/sjasm/build
	@cp $(Z80_SRCMAP_TEST_DIR)/sjasm/program.lst $(Z80_SRCMAP_TEST_DIR)/sjasm/build/program.lst
	@$(Z80_SRCMAP_TEST_EXE) --listing-format sjasm --build-dir $(Z80_SRCMAP_TEST_DIR)/sjasm/build --source-dir $(Z80_SRCMAP_TEST_DIR)/sjasm/src --out $(Z80_SRCMAP_TEST_DIR)/sjasm/build/out.z80srcmap --out-noi $(Z80_SRCMAP_TEST_DIR)/sjasm/build/out.noi

$(Z80_SRCMAP_TEST_DIR)/sjasm/build/expected.z80srcmap: $(Z80_SRCMAP_TEST_DIR)/sjasm/expected.z80srcmap.in | $(Z80_SRCMAP_TEST_DIR)/sjasm/build
	@sed "s|@TESTDIR@|$(abspath $(Z80_SRCMAP_TEST_DIR)/sjasm)|g" $< > $@

$(Z80_SRCMAP_TEST_DIR)/asxxxx/build $(Z80_SRCMAP_TEST_DIR)/sjasm/build:
	@mkdir -p $@

clean-z80srcmap-tests:
	rm -rf $(Z80_SRCMAP_TEST_DIR)/asxxxx/build $(Z80_SRCMAP_TEST_DIR)/sjasm/build
