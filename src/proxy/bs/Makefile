# compat makefile for make & nmake
# sxp@xdja.com 2015-05-29 17:39:19

BUILD_DIR=build
BUILD_MAKEFILE=$(BUILD_DIR)/Makefile

all : $(BUILD_MAKEFILE)
	cd $(BUILD_DIR) && $(MAKE)

config : $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake -G "$(G)" ..

install : $(BUILD_MAKEFILE)
	cd $(BUILD_DIR) && $(MAKE) $@

test : $(BUILD_MAKEFILE)
	cd $(BUILD_DIR) && $(MAKE) $@

clean : $(BUILD_MAKEFILE)
	cd $(BUILD_DIR) && $(MAKE) $@

$(BUILD_DIR) :
	mkdir $(BUILD_DIR)

depclean :
	@echo remove $(BUILD_DIR)
	@-rm -rf $(BUILD_DIR) || rmdir /S /Q $(BUILD_DIR)

.PHONY : all config install test clean depclean
