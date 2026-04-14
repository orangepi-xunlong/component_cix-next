# SPDX-License-Identifier: GPL-2.0

MODULE_NAME := amvx
SRC_DIR := .
EXTRA_CFLAGS += -I$(PWD)/driver -I$(PWD)/driver/if -I$(PWD)/driver/dev -I$(PWD)/driver/if/v4l2 -I$(PWD)/driver/external

# Add objects for if module.
if-y := $(SRC_DIR)/driver/if/mvx_if.o \
	$(SRC_DIR)/driver/if/mvx_buffer.o \
	$(SRC_DIR)/driver/if/mvx_firmware_cache.o \
	$(SRC_DIR)/driver/if/mvx_firmware.o \
	$(SRC_DIR)/driver/if/mvx_firmware_v2.o \
	$(SRC_DIR)/driver/if/mvx_firmware_v3.o \
	$(SRC_DIR)/driver/if/mvx_mmu.o \
	$(SRC_DIR)/driver/if/mvx_secure.o \
	$(SRC_DIR)/driver/if/mvx_session.o

# Add external interface.
if-y += $(SRC_DIR)/driver/if/v4l2/mvx_ext_v4l2.o \
	$(SRC_DIR)/driver/if/v4l2/mvx_v4l2_buffer.o \
	$(SRC_DIR)/driver/if/v4l2/mvx_v4l2_session.o \
	$(SRC_DIR)/driver/if/v4l2/mvx_v4l2_vidioc.o \
	$(SRC_DIR)/driver/if/v4l2/mvx_v4l2_fops.o \
	$(SRC_DIR)/driver/if/v4l2/mvx_v4l2_ctrls.o

# Add objects for dev module.
dev-y := $(SRC_DIR)/driver/dev/mvx_dev.o \
	 $(SRC_DIR)/driver/dev/mvx_hwreg.o \
	 $(SRC_DIR)/driver/dev/mvx_hwreg_v500.o \
	 $(SRC_DIR)/driver/dev/mvx_hwreg_v550.o \
	 $(SRC_DIR)/driver/dev/mvx_hwreg_v61.o \
	 $(SRC_DIR)/driver/dev/mvx_hwreg_v52_v76.o \
	 $(SRC_DIR)/driver/dev/mvx_lsid.o \
	 $(SRC_DIR)/driver/dev/mvx_scheduler.o \
	 $(SRC_DIR)/driver/mvx_pm_runtime.o

OBJS := $(SRC_DIR)/driver/mvx_driver.o \
	  $(SRC_DIR)/driver/mvx_seq.o \
	  $(SRC_DIR)/driver/mvx_log.o \
	  $(SRC_DIR)/driver/mvx_log_group.o \
	  $(if-y) $(dev-y)

ifneq ($(KERNELRELEASE),)
	obj-m := $(MODULE_NAME).o
	$(MODULE_NAME)-objs :=  $(OBJS)
else
	COMPASS_DRV_BTENVAR_KPATH ?= /lib/modules/`uname -r`/build
	PWD :=$(shell pwd)

all:
	$(MAKE) -C $(COMPASS_DRV_BTENVAR_KPATH) M=$(PWD) modules
clean:
	$(MAKE) -C $(COMPASS_DRV_BTENVAR_KPATH) M=$(PWD) clean
endif
