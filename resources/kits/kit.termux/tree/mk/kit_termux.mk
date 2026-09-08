# kit.termux build fragment.
#
# GENERATED FILE. kit.termux owns this path and will replace it on update.
#
# Nothing to detect at build time, and that is the point.
#
# Termux:API is not a library — it is a set of command-line programs that talk
# to a companion Android app and print JSON. There is no header to find and
# nothing to link, so the kit is one include path and the standard library.
#
# The dependency is therefore a *runtime* one, and the header reports it
# honestly: sq::termux::available() checks the programs exist, probe() checks
# the app answers. A build that succeeds here can still find the API missing on
# a device, which is the truth of the situation rather than something to paper
# over at compile time.

SQ_KIT_CPPFLAGS += -Isq_kit/include
SQ_KITS_PRESENT += kit.termux

.PHONY: termux-status
termux-status:
	@echo "kit.termux"
	@if command -v termux-battery-status >/dev/null 2>&1; then \
	  echo "  programs       installed"; \
	  printf '  app            '; \
	  if timeout 5 termux-battery-status >/dev/null 2>&1; then \
	    echo "responding"; \
	  else \
	    echo "NOT RESPONDING — install Termux:API from F-Droid"; \
	  fi; \
	else \
	  echo "  programs       NOT FOUND"; \
	  echo "                 pkg install termux-api"; \
	  echo "                 plus the Termux:API app from F-Droid"; \
	fi
	@echo "  note           both are needed; the package alone does nothing"
