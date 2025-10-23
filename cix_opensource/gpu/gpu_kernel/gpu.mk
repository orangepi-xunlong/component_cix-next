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

# Description: build GPU kernel module for cix Android product
# Author: Cunyuan Liu
# Date: 2023-01-30
# Revision: original v1.0
#

KERNEL_SRC := $(CIX_KERNEL_PATH)/kernel
KERNEL_OUT := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ

GPU_CROSS_COMPILE := aarch64-linux-gnu-
GPU_SRC_PATH := $(CIX_GPU_PATH)/drivers/gpu/arm
MGM_SRC_PATH := $(CIX_GPU_PATH)/drivers/base/arm
GPU_OUT := $(TARGET_OUT_INTERMEDIATES)/GPU_OBJ

GPU_KERNEL_CFLAGS = KCFLAGS="${KCFLAGS} -D__ANDROID_COMMON_KERNEL__"

ARCH ?= $(TARGET_KERNEL_ARCH)

# CONFIG_MALI_PLATFORM_NAME and CONFIG_MALI_CSF_SUPPORT is necessary
# for mali-Titan kernel driver
MAKE_CONFIGS := CONFIG_MALI_PLATFORM_NAME="sky1"
MAKE_CONFIGS += CONFIG_MALI_CSF_SUPPORT=y
MAKE_CONFIGS += CONFIG_MALI_MEMORY_GROUP_MANAGER=y
MAKE_CONFIGS += CONFIG_MALI_PROTECTED_MEMORY_ALLOCATOR=y

ifneq ($(CIX_GPU_NO_MALI),)
MAKE_CONFIGS += CONFIG_GPU_HAS_CSF=y
MAKE_CONFIGS += CONFIG_MALI_NO_MALI=y
MAKE_CONFIGS += CONFIG_MALI_REAL_HW=n
endif

GPU_KERNELENVSH := $(GPU_OUT)/kernelenv.sh
.PHONY: $(GPU_KERNELENVSH)
$(GPU_KERNELENVSH):
	mkdir -p $(GPU_OUT)
	echo 'export KDIR=$(KERNEL_OUT)' >> $(GPU_KERNELENVSH)
	echo 'export CROSS_COMPILE=$(GPU_CROSS_COMPILE)' >> $(GPU_KERNELENVSH)
	echo 'export ARCH=$(ARCH)' >> $(GPU_KERNELENVSH)
																						   

gpu: $(GPU_KERNELENVSH) $(GPU_SRC_PATH) $(MGM_SRC_PATH)
	$(hide) if [ ${clean_build} = 1 ]; then \
		PATH=$$PATH $(MAKE)  -C $(GPU_SRC_PATH) clean; \
	fi

	@ . $(GPU_KERNELENVSH); $(kernel_build_shell_env) \
	$(MAKE) -C $(MGM_SRC_PATH) ARCH=$(ARCH) $(MAKE_CONFIGS)\
		$(CLANG_TO_COMPILE) \
		$(GPU_KERNEL_CFLAGS)

	@ . $(GPU_KERNELENVSH); $(kernel_build_shell_env) \
	$(MAKE) -C $(GPU_SRC_PATH) ARCH=$(ARCH) $(MAKE_CONFIGS) \
		$(CLANG_TO_COMPILE) \
		$(GPU_KERNEL_CFLAGS)

	cp $(GPU_SRC_PATH)/midgard/mali_kbase.ko $(GPU_OUT);
	cp $(MGM_SRC_PATH)/memory_group_manager/memory_group_manager.ko $(GPU_OUT);
	cp $(MGM_SRC_PATH)/protected_memory_allocator/protected_memory_allocator.ko $(GPU_OUT);
	@ . $(GPU_KERNELENVSH); $(kernel_build_shell_env) \
	$(MAKE)  -C $(GPU_SRC_PATH) clean;
	@ . $(GPU_KERNELENVSH); $(kernel_build_shell_env) \
	$(MAKE)  -C $(MGM_SRC_PATH) clean;
