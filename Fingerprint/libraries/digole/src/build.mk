# This file is a makefile included from the top level makefile which
# defines the sources built for the target.

# Define the prefix to this directory. 
# Note: The name must be unique within this build and should be
#       based on the root of the project
TARGET_DIGOLE_PATH = libraries/digole
TARGET_DIGOLE_SRC_PATH = $(TARGET_DIGOLE_PATH)/src

# Add include paths.
INCLUDE_DIRS += $(TARGET_DIGOLE_PATH)/inc

# C source files included in this build.
CSRC +=

# C++ source files included in this build.
CPPSRC += $(TARGET_DIGOLE_SRC_PATH)/DigoleSerialDisp.cpp
#CPPSRC += $(TARGET_DIGOLE_SRC_PATH)/function_pulseIn.cpp

# ASM source files included in this build.
ASRC +=