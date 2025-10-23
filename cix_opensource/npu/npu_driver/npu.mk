#  Copyright 2024 Cix Technology Group Co., Ltd.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Description: build NPU OOT kernel module for cix Android product
# Author: Andy Tian
# Date: 2022-11-11
# Revision: original v1.0
#

KERNEL_SRC := $(CIX_KERNEL_PATH)/kernel
KERNEL_OUT := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ

NPU_CROSS_COMPILE := aarch64-linux-gnu-
NPU_SRC_PATH := $(CIX_NPU_PATH)/driver
NPU_OUT := $(TARGET_OUT_INTERMEDIATES)/NPU_OBJ

#NPU configs
COMPASS_DRV_BTENVAR_KMD_VERSION :=5.7.0
BUILD_AIPU_VERSION_KMD :=BUILD_ZHOUYI_V3
BUILD_TARGET_PLATFORM_KMD :=BUILD_PLATFORM_SKY1_ANDROID
BUILD_NPU_DEVFREQ :=y

NPU_KERNEL_CFLAGS = KCFLAGS="${KCFLAGS} -D__ANDROID_COMMON_KERNEL__"

ARCH ?= $(TARGET_KERNEL_ARCH)

NPU_KERNELENVSH := $(NPU_OUT)/kernelenv.sh
.PHONY: $(NPU_KERNELENVSH)
$(NPU_KERNELENVSH):
	mkdir -p $(NPU_OUT)
	echo 'export COMPASS_DRV_BTENVAR_KPATH=$(KERNEL_OUT)' > $(NPU_KERNELENVSH)
	echo 'export CROSS_COMPILE=$(NPU_CROSS_COMPILE)' >> $(NPU_KERNELENVSH)
	echo 'export ARCH=$(ARCH)' >> $(NPU_KERNELENVSH)
	echo 'export COMPASS_DRV_BTENVAR_KMD_VERSION=$(COMPASS_DRV_BTENVAR_KMD_VERSION)' >> $(NPU_KERNELENVSH)
	echo 'export BUILD_AIPU_VERSION_KMD=$(BUILD_AIPU_VERSION_KMD)' >> $(NPU_KERNELENVSH)
	echo 'export BUILD_TARGET_PLATFORM_KMD=$(BUILD_TARGET_PLATFORM_KMD)' >> $(NPU_KERNELENVSH)
	echo 'export BUILD_NPU_DEVFREQ=$(BUILD_NPU_DEVFREQ)' >> $(NPU_KERNELENVSH)

npu: $(NPU_KERNELENVSH) $(NPU_SRC_PATH)
	$(hide) if [ ${clean_build} = 1 ]; then \
		PATH=$$PATH $(MAKE)  -C $(NPU_SRC_PATH) clean; \
	fi

	@ . $(NPU_KERNELENVSH); $(kernel_build_shell_env) \
	cp -f $(NPU_SRC_PATH)/armchina-npu/include/armchina_aipu.h $(KERNEL_SRC)/include/uapi/misc

	@ . $(NPU_KERNELENVSH); $(kernel_build_shell_env) \
	$(MAKE) -C $(NPU_SRC_PATH) ARCH=$(ARCH) \
		$(CLANG_TO_COMPILE) \
		$(NPU_KERNEL_CFLAGS)

	cp $(NPU_SRC_PATH)/aipu.ko $(NPU_OUT);
	@ . $(NPU_KERNELENVSH); $(kernel_build_shell_env) \
	$(MAKE) $(CLANG_TO_COMPILE) -C $(NPU_SRC_PATH) clean;
	rm -f $(KERNEL_SRC)/include/uapi/misc/armchina_aipu.h

