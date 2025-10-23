#!/bin/sh

# copyright:
# ----------------------------------------------------------------------------
# This confidential and proprietary software may be used only as authorized
# by a licensing agreement from ARM Limited.
#      (C) COPYRIGHT 2023 ARM Limited, ALL RIGHTS RESERVED
# The entire notice above must be reproduced on all authorized copies and
# copies may only be made to the extent permitted by a licensing agreement
# from ARM Limited.
# ----------------------------------------------------------------------------
#

set -e

VERSION_MAJOR=0
VERSION_MINOR=0
VERSION_PATCH=0

REQUEST_VERSION=0
REQUEST_INIT=0
REQUEST_DUMP_PRE=0
REQUEST_DUMP=0
REQUEST_DUMP_POST=0
REQUEST_TERM=0

REQUEST_REG_CORE_DUMP=0

DUMPFAULT_ERROR_WITH_REG_CORE_DUMP=0

DF_FW_INTERNAL_ERROR=8
DF_PROTECTED_MODE_EXIT_TIMEOUT=12
DF_PING_REQUEST_TIMEOUT=14

env_array="DEVICE_INSTANCE_ID DUMP_INDEX TGID CTX_ID DUMPFAULT_ERROR_TYPE"

usage() {
	echo "Usage: $0 [OPTIONS] COMMANDS"
	echo ""
	echo "  Dump fw_traces log:"
	echo "    src: /sys/kernel/debug/mali{id}/fw_core_dump"
	echo "    dst: fw_core_dump_{nr}_{tgid}_{ctxid}.txt"
	echo ""
	echo "  IMPORTANT: The GPU cannot be used after FW register core dump."
	echo "             To prevent side effects such as loss of dumps from other"
	echo "             sources or plugin, fw_core_dump should be part of the last"
	echo "             plugins to execute."
	echo ""
	echo "  OPTIONS"
	echo ""
	echo "     --req_core_dump        Requests FW register core dump to the kernel driver"
	echo ""
	echo "  COMMANDS"
	echo "     version                Version"
	echo "     help                   Print help information"
	echo "     init                   Perform plugin set up"
	echo "     dump_pre               Perform plugin dump_pre"
	echo "     dump                   Perform plugin dump"
	echo "     dump_post              Perform plugin dump_post"
	echo "     term                   Perform plugin clean-up"
	echo ""
	echo "  ENV"
	echo "     DEVICE_INSTANCE_ID     Mali device ID"
	echo "     DUMP_INDEX             Number of plugin dumps calls tracked since the daemon started"
	echo "     TGID                   Associated thread group id"
	echo "     CTX_ID                 Associated context id"
	echo "     DUMPFAULT_ERROR_TYPE   Type of error detected by the daemon"
	echo ""
}

check() {
	count=0

	count=$((count + REQUEST_VERSION))
	count=$((count + REQUEST_INIT))
	count=$((count + REQUEST_DUMP_PRE))
	count=$((count + REQUEST_DUMP))
	count=$((count + REQUEST_DUMP_POST))
	count=$((count + REQUEST_TERM))

	if [ $count -ne 1 ]; then
		echo "ERROR: plugin requires one command argument: $count != 1"
		echo ""
		echo "       REQUEST_VERSION=$REQUEST_VERSION"
		echo "       REQUEST_INIT=$REQUEST_INIT"
		echo "       REQUEST_DUMP_PRE=$REQUEST_DUMP_PRE"
		echo "       REQUEST_DUMP=$REQUEST_DUMP"
		echo "       REQUEST_DUMP_POST=$REQUEST_DUMP_POST"
		echo "       REQUEST_TERM=$REQUEST_TERM"
		echo ""

		usage

		exit 1
	fi
}

environment() {
	ret=0

	for env in ${env_array}; do
		set +e
		printenv "$env" >/dev/null
		env_exists=$?
		set -e

		if [ $env_exists -eq 1 ]; then
			echo "ERROR: missing environment variable: ${env}"
			ret=1
		fi
	done

	if [ "$DUMPFAULT_ERROR_TYPE" = "$DF_FW_INTERNAL_ERROR" ] || [ "$DUMPFAULT_ERROR_TYPE" = "$DF_PROTECTED_MODE_EXIT_TIMEOUT" ] || [ "$DUMPFAULT_ERROR_TYPE" = "$DF_PING_REQUEST_TIMEOUT" ]; then
		DUMPFAULT_ERROR_WITH_REG_CORE_DUMP=0
	elif [ $REQUEST_REG_CORE_DUMP -eq 1 ]; then
		DUMPFAULT_ERROR_WITH_REG_CORE_DUMP=1
	fi

	if [ $ret -eq 1 ]; then
		exit 22
	fi
}

version() {
	if [ $REQUEST_VERSION -eq 0 ]; then
		return
	fi

	echo "$VERSION_MAJOR.$VERSION_MINOR.$VERSION_PATCH"

	exit 0
}

init() {
	if [ $REQUEST_INIT -eq 0 ]; then
		return
	fi

	# Empty

	exit 0
}

dump_pre() {
	if [ $REQUEST_DUMP_PRE -eq 0 ]; then
		return
	fi

	# Empty

	exit 0
}

dump() {
	if [ $REQUEST_DUMP -eq 0 ]; then
		return
	fi

	src="/sys/kernel/debug/mali${DEVICE_INSTANCE_ID}/fw_core_dump"
	dst="fw_core_dump_${DUMP_INDEX}_${TGID}_${CTX_ID}.txt"

	if [ $DUMPFAULT_ERROR_WITH_REG_CORE_DUMP -eq 1 ]; then
		set +e
		# Explicit request for fw_core_dump failed:
		#   Don't treat this as an error, just print an error message.
		#   The firmware may have stopped responding and already made
		#   a core dump available.
		echo "1" >"${src}"
		set -e
	fi

	dd if="${src}" iflag=nonblock 1>"${dst}"

	if [ $DUMPFAULT_ERROR_WITH_REG_CORE_DUMP -eq 1 ]; then
		# Request GPU reset. GPU cannot be used after FW register core dump.
		dst="/sys/kernel/debug/mali${DEVICE_INSTANCE_ID}/reset"
		echo "0" >"${dst}"
	fi

	exit 0
}

dump_post() {
	if [ $REQUEST_DUMP_POST -eq 0 ]; then
		return
	fi

	# Empty

	exit 0
}

term() {
	if [ $REQUEST_TERM -eq 0 ]; then
		return
	fi

	# Empty

	exit 0
}

ARGV=$(getopt --long 'req_core_dump' 'help,version,init,dump_pre,dump,dump_post,term' "$@")
ret=$?

if [ $ret -ne 0 ]; then
	usage
	exit 1
fi

eval set -- "$ARGV"
while true; do
	if [ -z "$1" ]; then
		break
	fi

	case "$1" in
	version)
		REQUEST_VERSION=1
		shift
		;;
	init)
		REQUEST_INIT=1
		shift
		;;
	dump_pre)
		REQUEST_DUMP_PRE=1
		shift
		;;
	dump)
		REQUEST_DUMP=1
		shift
		;;
	dump_post)
		REQUEST_DUMP_POST=1
		shift
		;;
	term)
		REQUEST_TERM=1
		shift
		;;
	--req_core_dump)
		REQUEST_REG_CORE_DUMP=1
		shift
		;;
	? | help)
		usage
		exit 0
		;;
	--)
		shift
		;;
	*)
		usage
		exit 1
		;;
	esac
done

check
version
environment
init
dump_pre
dump
dump_post
term

exit 0
