# Makefile created by monomake, Lør  7 Jan 2017 14:06:00 CET
# Project: templog

MONO_PATH=$(subst \,/,$(shell monomake path --bare))
include $(MONO_PATH)/predefines.mk

TARGET=templog

include $(MONO_PATH)/mono.mk
