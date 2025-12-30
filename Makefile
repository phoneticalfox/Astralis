SEED0_DIR := src/seed0
BIN := $(SEED0_DIR)/astralis

.PHONY: all seed0 examples clean

all: seed0

seed0:
	$(MAKE) -C $(SEED0_DIR)

examples: seed0
	tools/run_examples.sh

clean:
	$(MAKE) -C $(SEED0_DIR) clean
