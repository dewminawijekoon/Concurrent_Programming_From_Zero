# Concurrent Programming From Zero — top-level commands.
#
#   make check                   what toolchains does this machine have?
#   make test M=05               run module 05's tests against YOUR code
#   make test M=05 T=ex1_peterson    ...only one exercise
#   make run  M=03 EX=ex2_sum ARGS="8"   build + run one exercise on its own
#   make tsan M=03               the same tests under ThreadSanitizer (data races)
#   make test-all                every module
#   make progress                table of passed exercises per module
#   make verify-answers          test the reference answers (maintainers / CI)
#   make interleave F=tools/interleave/programs/peterson.il   explore a program
#   make clean
#
# M is a module-number prefix: M=05 matches modules/05-mutual-exclusion.

SHELL   := /bin/bash
MODULES := $(sort $(filter-out modules/_TEMPLATE,$(wildcard modules/[0-9]*)))
MOD      = $(firstword $(wildcard modules/$(M)*))
ONLY     = $(if $(T),--only $(T),)

.PHONY: help check test run tsan test-all progress verify-answers verify-answers-repeat \
        interleave interleave-selftest clean

help:
	@sed -n '2,15p' Makefile | sed 's/^# \{0,1\}//'

check:
	@bash setup/check.sh

define need_module
	@if [ -z "$(M)" ] || [ -z "$(MOD)" ]; then echo "Pick a module, e.g. make $(1) M=05"; ls modules | grep -v _TEMPLATE; exit 2; fi
endef

test:
	$(call need_module,test)
	@bash tools/runtests.sh $(ONLY) $(MOD)

run:
	$(call need_module,run)
	@if [ -z "$(EX)" ]; then echo "Pick an exercise, e.g. make run M=$(M) EX=ex1_..."; exit 2; fi
	@bash tools/run.sh $(if $(ANS),--answers,) $(MOD) $(EX) $(ARGS)

tsan:
	$(call need_module,tsan)
	@bash tools/runtests.sh --tsan $(ONLY) $(MOD)

test-all:
	@bash tools/runtests.sh --quiet $(MODULES)

progress:
	@rm -f .progress.tmp
	@bash tools/runtests.sh --quiet --summary .progress.tmp $(MODULES) > /dev/null || true
	@printf "\n%-38s %8s %8s\n" "module" "passed" "skipped"; \
	 while read -r m p t s; do printf "%-38s %4s/%-3s %8s\n" "$$m" "$$p" "$$t" "$$s"; done < .progress.tmp; \
	 rm -f .progress.tmp

verify-answers:
	@bash tools/runtests.sh --answers --quiet $(if $(M),$(MOD),$(MODULES))

# CI: catch flaky tests by running the answers several times.
verify-answers-repeat:
	@for i in $$(seq 1 $${REPEAT:-10}); do echo "### round $$i"; \
	   bash tools/runtests.sh --answers --quiet $(MODULES) || exit 1; done

interleave:
	@python3 tools/interleave/interleave.py $(F) $(ARGS)

interleave-selftest:
	@bash tools/interleave/tests/run.sh

clean:
	@find modules -type d -name '.build*' -prune -exec rm -rf {} +
	@echo cleaned
