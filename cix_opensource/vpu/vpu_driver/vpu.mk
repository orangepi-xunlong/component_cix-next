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

# Description: build VPU OOT kernel module for cix Android product
# Author: Shijie Qin
# Date: 2023-02-27
# Revision: original v1.0
#

KERNEL_OUT := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ
ARCH ?= $(TARGET_KERNEL_ARCH)

VPU_DRIVRE_PATH := $(CIX_VPU_PATH)/driver
VPU_OUT := $(TARGET_OUT_INTERMEDIATES)/VPU_OBJ
# VPU_CONFIG += CONFIG_VIDEO_LINLON_FTRACE=y
VPU_CONFIG += CONFIG_VIDEO_LINLON_PRINT_FILE=y

.PHONY: vpu
vpu: $(VPU_DRIVRE_PATH)
	$(hide) if [ ${clean_build} = 1 ]; then \
		cd $(CIX_VPU_PATH); \
		./clean.sh; \
	fi

	@$(kernel_build_shell_env) \
	$(MAKE) -C $(VPU_DRIVRE_PATH) ARCH=$(ARCH) KDIR=$(KERNEL_OUT) \
		$(CLANG_TO_COMPILE) $(VPU_CONFIG)

	mkdir -p $(VPU_OUT);
	cp $(VPU_DRIVRE_PATH)/amvx.ko $(VPU_OUT);
	cd $(CIX_VPU_PATH); \
	./clean.sh;
