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

REQUEST_INVALID_ENV=0
REQUEST_VERSION=0
REQUEST_INIT=0
REQUEST_DUMP_PRE=0
REQUEST_DUMP=0
REQUEST_DUMP_POST=0
REQUEST_TERM=0
REQUEST_TERM_ERROR=0

env_array="DEVICE_INSTANCE_ID DUMP_INDEX TGID CTX_ID DUMPFAULT_ERROR_TYPE"

usage() {
	echo "Usage: $0 [OPTIONS] COMMANDS"
	echo ""
	echo "  OPTIONS"
	echo "     --bad_version          Returns an invalid plugin version"
	echo "     --invalid_env          Inject invalid environment variables check"
	echo "     --error                Exit with error, always"
	echo "     --error_term           Exit with error on plugin termination (COMMANDS=term)"
	echo ""
	echo "  COMMANDS"
	echo "     version                Version"
	echo "     help                   Print help information"
	echo "     init                   Perform plugin test set up"
	echo "     dump_pre               Perform plugin test dump_pre"
	echo "     dump                   Perform plugin test dump"
	echo "     dump_post              Perform plugin test dump_post"
	echo "     term                   Perform plugin test clean-up"
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

	if [ $REQUEST_INVALID_ENV -eq 1 ]; then
		ret=1
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

	echo "Test: initialized"

	exit 0
}

dump_pre() {
	if [ $REQUEST_DUMP_PRE -eq 0 ]; then
		return
	fi

	echo "Test: dump_pre"

	exit 0
}

dump() {
	if [ $REQUEST_DUMP -eq 0 ]; then
		return
	fi

	echo "Test: dump"

	exit 0
}

dump_post() {
	if [ $REQUEST_DUMP_POST -eq 0 ]; then
		return
	fi

	echo "Test: dump_post"

	exit 0
}

term() {
	if [ $REQUEST_TERM -eq 0 ]; then
		return
	fi

	if [ $REQUEST_TERM_ERROR -eq 1 ]; then
		echo "Test: termination failed"
		exit 1
	fi

	echo "Test: terminated"

	exit 0
}

ARGV=$(getopt --long 'bad_version,invalid_env,error,error_term' 'help,version,init,dump_pre,dump,dump_post,term' "$@")
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
	--bad_version)
		VERSION_MAJOR=100
		VERSION_MINOR=10
		VERSION_PATCH=1
		shift
		;;
	--invalid_env)
		REQUEST_INVALID_ENV=1
		shift
		;;
	--error)
		exit 1
		;;
	--error_term)
		REQUEST_TERM_ERROR=1
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
