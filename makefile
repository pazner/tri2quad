-include Make.user

ifndef MFEM_DIR
$(error MFEM_DIR is not set. Please create a Make.user file to set MFEM_DIR)
endif

MFEM_BUILD_DIR = $(MFEM_DIR)
CONFIG_MK = $(MFEM_BUILD_DIR)/config/config.mk

-include $(CONFIG_MK)

# Overall structure
BUILD_DIR=build
DEPENDS=$(BUILD_DIR)/.depends
SOURCES=
OBJECTS=$(SOURCES:%.cpp=$(BUILD_DIR)/%.o)
APPS=tri2quad
APP_SRC=$(APPS:%=%.cpp)

CXXFLAGS=-g $(MFEM_CXXFLAGS)

# Use compiler configuration from MFEM
LFLGAS=$(MFEM_LIBS)
INCFLAGS=$(MFEM_INCFLAGS)

.PHONY: all clean style deps
all: $(APPS)

# Build the executable
$(APPS):%: $(BUILD_DIR)/%.o $(OBJECTS) $(MFEM_LIB_FILE)
	$(MFEM_CXX) $(MFEM_LINK_FLAGS) $< $(OBJECTS) $(LFLGAS) -o $@

$(BUILD_DIR)/%.o: makefile | $(BUILD_DIR)
	$(MFEM_CXX) -c $(CXXFLAGS) $(INCFLAGS) -o $@ $*.cpp

# Use the compiler to determine dependencies on header files
# Some sed magic in the next target to properly prefix the object files with path
BUILD_DEPS=$(MFEM_CXX) $(CXXFLAGS) $(MFEM_INCFLAGS) -M $(SOURCES) $(APP_SRC) \
		| sed -E 's\#^(.*\.o: *)(.*/)?(.*\.cpp)\#build/\2\1\2\3\#' > $(DEPENDS)

$(DEPENDS): | $(BUILD_DIR)
	$(BUILD_DEPS)

deps:
	$(BUILD_DEPS)

# Rebuild dependencies unless doing "make clean" or "make style"
ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),style)
-include $(DEPENDS)
endif
endif

# Ensure build directory exists
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(EXE) $(BUILD_DIR)

FORMAT_FILES = $(wildcard *.?pp)
ASTYLE = astyle --options=$(MFEM_DIR)/config/mfem.astylerc
style:
	@if ! $(ASTYLE) $(FORMAT_FILES) | grep Formatted; then\
	   echo "No source files were changed.";\
	fi
