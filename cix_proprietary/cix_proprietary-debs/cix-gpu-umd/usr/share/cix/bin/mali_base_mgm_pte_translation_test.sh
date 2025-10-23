#!/bin/sh

# -----------------------------------------------------------------------------
# The proprietary software and information contained in this file is
# confidential and may only be used by an authorized person under a valid
# licensing agreement from Arm Limited or its affiliates.
#
# Copyright (C) 2022. Arm Limited or its affiliates. All rights reserved.
#
# This entire notice must be reproduced on all copies of this file and
# copies of this file may only be made by an authorized person under a valid
# licensing agreement from Arm Limited or its affiliates.
# -----------------------------------------------------------------------------
PTE_TRANSLATION_PATH_PARTIAL="/sys/kernel/debug/kutf_tests/mgm/mgm_integration/pte_translation"
KUTF_MODULE="kutf"
KBASE_MODULE="mali_kbase"
MALI_KUTF_MGM_INTEGRATION_TEST_MODULE="mali_kutf_mgm_integration_test"
MGM_MODULE_NAME="memory_group_manager"

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

check_insmod() {
	res=$(lsmod | grep "$1")
	if [ -z "$res" ]; then
		echo 0
	else
		echo 1
	fi
}

if [ "$(check_insmod $MGM_MODULE_NAME)" -eq 0 ]; then
	echo "Missing MGM module, re-insmoding kbase..."
	rmmod $MALI_KUTF_MGM_INTEGRATION_TEST_MODULE
	rmmod $KUTF_MODULE
	rmmod $KBASE_MODULE
	insmod "$exec_path/$MGM_MODULE_NAME.ko"
	insmod "$exec_path/$KBASE_MODULE.ko"
fi
if [ "$(check_insmod $MGM_MODULE_NAME)" -eq 1 ]; then
	rmmod $MALI_KUTF_MGM_INTEGRATION_TEST_MODULE
	rmmod $KUTF_MODULE
fi

insmod "$exec_path/$KUTF_MODULE.ko"
insmod "$exec_path/$MALI_KUTF_MGM_INTEGRATION_TEST_MODULE.ko"

group_ids=""
if [ $# -eq 0 ]; then
	echo "No group_id passed will default to testing group_ids 0..15"
	group_ids="$(seq 0 15)"
else
	group_ids="$(echo $@ | tr ' ' '\n' | sort | uniq)"
fi

echo "======================================================="
echo "Running PTE Translation Test"
echo "======================================================="

results=""
for group_id in $group_ids; do
	if [ $group_id -gt 15 ] || [ $group_id -lt 0 ]; then
		echo "Error: group_id=$group_id. Must be in the range 0 <= x < 16."
		exit 1
	fi
	run_path="${PTE_TRANSLATION_PATH_PARTIAL}/${group_id}/run"

	if [ ! -e $run_path ]; then
		echo "Error: path for group_id=$group_id does not exist: $run_path."
		exit 1
	fi

	echo "Testing: group_id=$group_id"

	result="$(cat "$run_path")"
	echo "$result"

	results="$results\n$result"
done
echo "======================================================="
echo "Result Details"
echo "======================================================="
echo "$results" | grep "KUTF_RESULT_PASS"
echo "$results" | grep "KUTF_RESULT_FAIL"

nr_fail="$(echo "$results" | grep "KUTF_RESULT_FAIL" | wc -l)"
if [ "$nr_fail" -gt 0 ]; then
	exit 1
fi
