CPPEXAMPLE_MOD_DIR := $(USERMOD_DIR)

# Add our source files to the respective variables.
SRC_USERMOD += $(CPPEXAMPLE_MOD_DIR)/ALTRAgraphix.c
SRC_USERMOD += $(CPPEXAMPLE_MOD_DIR)/canvas.c
SRC_USERMOD += $(CPPEXAMPLE_MOD_DIR)/fxbuf.c
SRC_USERMOD += $(CPPEXAMPLE_MOD_DIR)/fxbufset.c
SRC_USERMOD += $(CPPEXAMPLE_MOD_DIR)/sprite.c
SRC_USERMOD += $(CPPEXAMPLE_MOD_DIR)/types.c
SRC_USERMOD_CXX += $(CPPEXAMPLE_MOD_DIR)/cpp/fxbuf.cpp
SRC_USERMOD_CXX += $(CPPEXAMPLE_MOD_DIR)/cpp/fxbufset.cpp
SRC_USERMOD_CXX += $(CPPEXAMPLE_MOD_DIR)/cpp/canvas.cpp
SRC_USERMOD_CXX += $(CPPEXAMPLE_MOD_DIR)/cpp/sprite.cpp
#SRC_USERMOD_CXX += $(CPPEXAMPLE_MOD_DIR)/example.cpp

# Add our module directory to the include path.
CFLAGS_USERMOD += -I$(CPPEXAMPLE_MOD_DIR)
CXXFLAGS_USERMOD += -I$(CPPEXAMPLE_MOD_DIR) -std=c++11

# Enable imports of built-in submodules such as altra.types.
CFLAGS_EXTRA += -DMICROPY_MODULE_BUILTIN_SUBPACKAGES=1

# We use C++ features so have to link against the standard library.
LDFLAGS_USERMOD += -lstdc++
