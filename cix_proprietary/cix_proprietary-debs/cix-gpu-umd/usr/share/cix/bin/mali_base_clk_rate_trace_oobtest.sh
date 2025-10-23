#!/bin/sh

# -----------------------------------------------------------------------------
# The proprietary software and information contained in this file is
# confidential and may only be used by an authorized person under a valid
# licensing agreement from Arm Limited or its affiliates.
#
# Copyright (C) 2020-2022. Arm Limited or its affiliates. All rights reserved.
#
# This entire notice must be reproduced on all copies of this file and
# copies of this file may only be made by an authorized person under a valid
# licensing agreement from Arm Limited or its affiliates.
# -----------------------------------------------------------------------------

KUTF_MODULE="kutf"
MALI_KUTF_CLK_TRACE_TEST_MODULE="mali_kutf_clk_rate_trace_test_portal"
APP_NAME="mali_base_clk_rate_trace_test"

# Locate the folder with the kernel modules. We cannot make any assumption
# about where kernel modules are, because this script is intended to be run
# on a great variety of platforms.
# We can only assume that both kernel modules will be in the same folder.
bin_dirs="../modules ../bin ../bins ../../bin/mali_tests64 ../../bin/mali_tests32 ../../bin/mali_tests ../../bin ../../bins ."
for d in ${bin_dirs}; do
        if [ -f "${d}/$KUTF_MODULE.ko" ]; then
                module_path="${d}"
                break
        fi
done

if [ -z "${module_path}" ]; then
        echo "Failed to locate kernel modules!"
        exit 1
fi

# Locate the folder with the APP.
for d in ${bin_dirs}; do
        if [ -f "${d}/$APP_NAME" ]; then
                app_path="${d}"
                break
        fi
done

if [ -z "${app_path}" ]; then
        echo "Failed to locate $APP_NAME!"
        exit 1
fi

rmmod $MALI_KUTF_CLK_TRACE_TEST_MODULE
rmmod $KUTF_MODULE

insmod "$module_path/$KUTF_MODULE.ko"
insmod "$module_path/$MALI_KUTF_CLK_TRACE_TEST_MODULE.ko"

# Run the standalone test, its exit status marks success or fail.
# I.e. 0: success; non-zero failure.
${app_path}/${APP_NAME}
