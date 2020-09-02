ifeq ($(RTE_SDK),)
$(error "Please define RTE_SDK environment variable")
endif

# Default target, can be overriden by command line or environment
RTE_TARGET ?= x86_64-native-linuxapp-gcc

include $(RTE_SDK)/mk/rte.vars.mk

# binary name
APP = server

# all source are stored in SRCS-y
SRCS-y = txn_queue.c server.c think_queue.c

CFLAGS += -O3 # -g for gdb debugging
EXTRA_CFLAGS= -I$(S)/include/
#CFLAGS += $(WERROR_FLAGS)

include $(RTE_SDK)/mk/rte.extapp.mk
