#!/bin/sh

# copyright:
# ----------------------------------------------------------------------------
# This confidential and proprietary software may be used only as authorized
# by a licensing agreement from ARM Limited.
#      (C) COPYRIGHT 2023-2024 ARM Limited, ALL RIGHTS RESERVED
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

CONFIG_SCRIPT=""
SOURCE_TYPE=""
BUSLOGGER_PATH=""

SINK_ENABLE="/sys/bus/coresight/devices/mali_etr0/enable_sink"
SOURCE_ENABLE=""

DF_FW_INTERNAL_ERROR=8
DF_PROTECTED_MODE_EXIT_TIMEOUT=12
DF_PING_REQUEST_TIMEOUT=14

env_array="DEVICE_INSTANCE_ID DUMP_INDEX TGID CTX_ID DUMPFAULT_ERROR_TYPE"

usage() {
	echo "Usage: $0 [OPTIONS] COMMANDS"
	echo ""
	echo "  Dump Coresight log:"
	echo "    dst: coresight_dump_{nr}_{tgid}_{ctxid}.raw"
	echo ""
	echo "  OPTIONS"
	echo "     --coresight-source-type {ela,etm,itm} Type of the Coresight source to dump"
	echo "     --ela-config-script         Script containing Coresight ELA configuration logic."
	echo "                                 Only required for: --coresight-source-type=ela"
	echo "     --bus-logger-path           Path to where the bus_logger_control binary is"
	echo ""
	echo "  COMMANDS"
	echo "     version                     Version"
	echo "     help                        Print help information"
	echo "     init                        Perform plugin coresight set up"
	echo "     dump_pre                    Perform plugin coresight dump_pre"
	echo "     dump                        Perform plugin coresight dump"
	echo "     dump_post                   Perform plugin coresight dump_post"
	echo "     term                        Perform plugin coresight clean-up"
	echo ""
	echo "  ENV"
	echo "     DEVICE_INSTANCE_ID          Mali device ID"
	echo "     DUMP_INDEX                  Number of plugin dumps calls tracked since the daemon started"
	echo "     TGID                        Associated thread group id"
	echo "     CTX_ID                      Associated context id"
	echo "     DUMPFAULT_ERROR_TYPE        Type of error detected by the daemon"
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

	if [ "$BUSLOGGER_PATH" = "" ] || [ ! -e "$BUSLOGGER_PATH" ]; then
		echo "ERROR: invalid Buslogger argument"
		echo ""

		usage

		exit 1
	fi

	if [ "$SOURCE_TYPE" != "etm" ] && [ "$SOURCE_TYPE" != "itm" ] && [ "$SOURCE_TYPE" != "ela" ]; then
		echo "ERROR: invalid Coresight source type argument"
		echo ""

		usage

		exit 1
	fi

	if [ "$SOURCE_TYPE" = "ela" ]; then
		if [ "$CONFIG_SCRIPT" = "" ] || [ ! -e "$CONFIG_SCRIPT" ]; then
			echo "ERROR: invalid Coresight $SOURCE_TYPE configuration script argument: $CONFIG_SCRIPT"
			echo ""

			usage

			exit 1
		fi
	fi

	if [ ! -e "$SINK_ENABLE" ]; then
		echo "ERROR: missing coresight interface: $SINK_ENABLE"
		echo ""

		usage

		exit 1
	fi

	SOURCE_ENABLE="/sys/bus/coresight/devices/mali-source-$SOURCE_TYPE/enable_source"

	if [ ! -e "$SOURCE_ENABLE" ]; then
		echo "ERROR: missing coresight interface: $SOURCE_ENABLE"
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

	if [ "$SOURCE_TYPE" = "ela" ]; then
		sh "$CONFIG_SCRIPT"
	fi

	echo 1 >"$SINK_ENABLE"
	echo 1 >"$SOURCE_ENABLE"

	exit 0
}

dump_pre() {
	if [ $REQUEST_DUMP_PRE -eq 0 ]; then
		return
	fi

	echo 1 >"$SINK_ENABLE"
	echo 1 >"$SOURCE_ENABLE"

	exit 0
}

dump() {
	if [ $REQUEST_DUMP -eq 0 ]; then
		return
	fi

	echo 0 >"$SOURCE_ENABLE"
	echo 0 >"$SINK_ENABLE"

	dst="coresight_dump_${DUMP_INDEX}_${TGID}_${CTX_ID}.raw"

	if [ "$DUMPFAULT_ERROR_TYPE" = "$DF_FW_INTERNAL_ERROR" ] || [ "$DUMPFAULT_ERROR_TYPE" = "$DF_PROTECTED_MODE_EXIT_TIMEOUT" ] || [ "$DUMPFAULT_ERROR_TYPE" = "$DF_PING_REQUEST_TIMEOUT" ]; then
		echo "INFO: FW is not accessible. Skip Coresight dump."
		echo "Dummy" >"${dst}"
	else
		"$BUSLOGGER_PATH" -p 0,$(($(cat /sys/bus/coresight/devices/mali_etr0/rwp_reg))) -f "${dst}"
	fi

	exit 0
}

dump_post() {
	if [ $REQUEST_DUMP_POST -eq 0 ]; then
		return
	fi

	exit 0
}

term() {
	if [ $REQUEST_TERM -eq 0 ]; then
		return
	fi

	echo 0 >"$SOURCE_ENABLE"
	echo 0 >"$SINK_ENABLE"

	exit 0
}

ARGV=$(getopt --long 'coresight-source-type:,ela-config-script:,bus-logger-path:' 'help,version,init,dump_pre,dump,dump_post,term' "$@")
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
	--ela-config-script)
		CONFIG_SCRIPT=$2
		shift 2
		;;
	--coresight-source-type)
		SOURCE_TYPE=$2
		shift 2
		;;
	--bus-logger-path)
		BUSLOGGER_PATH="$2/bus_logger_control"
		shift 2
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
