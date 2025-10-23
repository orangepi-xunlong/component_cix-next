#!/bin/sh

# -----------------------------------------------------------------------------
# The proprietary software and information contained in this file is
# confidential and may only be used by an authorized person under a valid
# licensing agreement from Arm Limited or its affiliates.
#
# Copyright (C) 2019-2022. Arm Limited or its affiliates. All rights reserved.
#
# This entire notice must be reproduced on all copies of this file and
# copies of this file may only be made by an authorized person under a valid
# licensing agreement from Arm Limited or its affiliates.
# -----------------------------------------------------------------------------

IRQ_LATENCY_PATH="/sys/kernel/debug/kutf_tests/irq/irq_default/irq_latency/0"
KUTF_MODULE="kutf"
MALI_KUTF_IRQ_TEST_MODULE="mali_kutf_irq_test"

# Locate the folder with the kernel modules. We cannot make any assumption
# about where kernel modules are, because this script is intended to be run
# on a great variety of platforms.
# We can only assume that both kernel modules will be in the same folder.
bin_dirs="../bin ../bins ../../bin/mali_tests64 ../../bin/mali_tests32 ../../bin/mali_tests ../../bin ../../bins ."
for d in ${bin_dirs}; do
	if [ -f "${d}/$KUTF_MODULE.ko" ]; then
		exec_path="${d}"
		break
	fi
done

if [ -z "${exec_path}" ]; then
	echo "Failed to locate kernel modules!"
	exit 1
fi

rmmod $MALI_KUTF_IRQ_TEST_MODULE
rmmod $KUTF_MODULE

insmod "$exec_path/$KUTF_MODULE.ko"
insmod "$exec_path/$MALI_KUTF_IRQ_TEST_MODULE.ko"

TEST_RUN=$(cat "$IRQ_LATENCY_PATH/run")

echo -n "echo"
echo -e "Return Message:\n******RunMessageStart******\n\n$TEST_RUN\n\n******RunMessageEnd******\n"

echo "$TEST_RUN" | grep "KUTF_RESULT_PASS"
