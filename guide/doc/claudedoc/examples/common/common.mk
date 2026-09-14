# ============================================================
#  common.mk -- shared build rules for the course examples
# ============================================================
#
#  Every example folder has a tiny Makefile that sets a few
#  variables and includes this file. The flow is always:
#
#     1. copy the pristine OS tree into ./build/os
#     2. overlay this example's files on top of the copy
#     3. optionally run this example's patch.py for surgical edits
#     4. build it and boot it in QEMU
#
#  The original project in lytlnybl/ is NEVER modified: everything
#  happens inside this example's own build/ directory.
#
#  Variables an example may set before including this file:
#
#     OVERLAY   files to copy over the OS tree, given as
#               "source:destination" pairs relative to the
#               example folder and to the OS tree respectively.
#               e.g. OVERLAY = kernel_main.c:kernel/kernel_main.c
#
#     PATCHES   optional python scripts run against build/os
#               for edits that insert into existing files.
#
#     DESC      one-line description, printed by `make`.
#
#  Targets:
#     make run     build and boot in QEMU          (the default)
#     make build   build only, no QEMU
#     make shell   boot headless and print the screen as text
#     make diff    show what this example changes vs. the original
#     make clean   delete this example's build directory
# ============================================================

# Locate the OS tree relative to the example folder (examples/<name>/).
OS_SRC   := $(abspath $(dir $(lastword $(MAKEFILE_LIST)))/../../../../lytlnybl)
COMMON   := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))
WORK     := build/os
STAMP    := build/.copied

QEMU     ?= qemu-system-i386
QEMUFLAGS ?= -drive format=raw,file=$(WORK)/build/kernel.img

.PHONY: all run build shell trace diff clean help

all: help

help:
	@echo
	@echo "  $(notdir $(CURDIR))"
ifdef DESC
	@echo "  $(DESC)"
endif
	@echo
	@echo "    make run     build this example and boot it in QEMU"
	@echo "    make build   build only"
	@echo "    make shell   boot headless, print the screen as text"
	@echo "    make trace   boot with interrupt logging, stop at a triple fault"
	@echo "    make diff    show what this example changes"
	@echo "    make clean   remove build/"
	@echo

# ------------------------------------------------------------
# 1+2+3: copy the OS, overlay this example, apply patches
# ------------------------------------------------------------
$(STAMP): $(wildcard *.c) $(wildcard *.h) $(wildcard *.asm) $(wildcard *.py) Makefile
	@echo "==> copying the OS tree into $(WORK)"
	@rm -rf $(WORK)
	@mkdir -p $(WORK)
	@cd $(OS_SRC) && tar --exclude=build --exclude=.git -cf - . | (cd $(CURDIR)/$(WORK) && tar -xf -)
	@$(foreach pair,$(OVERLAY), \
	    src=$(word 1,$(subst :, ,$(pair))); dst=$(word 2,$(subst :, ,$(pair))); \
	    test -f "$$src" || { echo "ERROR: overlay source $$src not found"; exit 1; }; \
	    test -f "$(WORK)/$$dst" || echo "  (new file) $$dst"; \
	    echo "==> overlay $$src -> $$dst"; \
	    mkdir -p "$(WORK)/$$(dirname $$dst)"; \
	    cp "$$src" "$(WORK)/$$dst"; )
	@$(foreach p,$(PATCHES), \
	    echo "==> applying $(p)"; \
	    python3 "$(p)" "$(WORK)" || exit 1; )
	@touch $(STAMP)

# ------------------------------------------------------------
# 4: build
# ------------------------------------------------------------
build: $(STAMP)
	@echo "==> building"
	@$(MAKE) --no-print-directory -C $(WORK) build
	@echo "==> $(WORK)/build/kernel.img ready"

run: build
	@echo "==> booting in QEMU (close the window to stop)"
	$(QEMU) $(QEMUFLAGS)

# Boot with no display and dump the text screen, for checking
# the example works without a graphical session.
shell: build
	@python3 $(COMMON)/screen.py $(WORK)/build/kernel.img $(SCREEN_ARGS)

# Boot with interrupt and reset logging, stopping at a triple fault
# instead of rebooting. This is the technique from Chapter 42.
trace: build
	@rm -f build/trace.log
	-@timeout 30 $(QEMU) $(QEMUFLAGS) -display none -no-reboot \
	    -d int,cpu_reset -D build/trace.log >/dev/null 2>&1
	@echo "==> build/trace.log written ($$(wc -l < build/trace.log) lines)"
	@echo
	@echo "last exceptions before the end:"
	@grep -E '^ *[0-9]+: v=' build/trace.log | tail -12 || true
	@echo
	@grep -c 'Triple fault' build/trace.log >/dev/null 2>&1 \
	    && echo "==> the CPU tripled-faulted (this is what a reboot loop looks like)" \
	    || echo "==> no triple fault"

diff: $(STAMP)
	@$(foreach pair,$(OVERLAY), \
	    src=$(word 1,$(subst :, ,$(pair))); dst=$(word 2,$(subst :, ,$(pair))); \
	    echo "--- original $$dst"; echo "+++ example  $$src"; \
	    diff -u "$(OS_SRC)/$$dst" "$$src" || true; )

clean:
	rm -rf build
