#!/usr/bin/env python
# -----------------------------------------------------------------------------
# The proprietary software and information contained in this file is
# confidential and may only be used by an authorized person under a valid
# licensing agreement from Arm Limited or its affiliates.
#
# Copyright (C) 2016-2024. Arm Limited or its affiliates. All rights reserved.
#
# This entire notice must be reproduced on all copies of this file and
# copies of this file may only be made by an authorized person under a valid
# licensing agreement from Arm Limited or its affiliates.
# -----------------------------------------------------------------------------
#
# Convert modifier in input .ll file. Used to be able to write modifiers in text.
from __future__ import print_function

import sys
import re
import subprocess as sp

def convert_common(match):
    args = match.group(1)
    args = args.replace(",", " ")
    return args


def convert_tex_mod(match):
    return sp.check_output("MaliModConv to-texmod {:s}".format(convert_common(match)), shell=True).rstrip().decode('ascii')

def convert_var_mod(match):
    return sp.check_output("MaliModConv to-varmod {:s}".format(convert_common(match)), shell=True).rstrip().decode('ascii')

def convert_flat_var_mod(match):
    return sp.check_output("MaliModConv to-flatvarmod {:s}".format(convert_common(match)), shell=True).rstrip().decode('ascii')

def convert_buf_var_mod(match):
    return sp.check_output("MaliModConv to-bufvarmod {:s}".format(convert_common(match)), shell=True).rstrip().decode('ascii')

def convert_collective_op(match):
    return sp.check_output("MaliModConv to-collective-op {:s}".format(convert_common(match)), shell=True).rstrip().decode('ascii')

def convert_clper_mod(match):
    return sp.check_output("MaliModConv to-clper-mod {:s}".format(convert_common(match)), shell=True).rstrip().decode('ascii')

def convert_wmask_mod(match):
    return sp.check_output("MaliModConv to-wmask-mod {:s}".format(convert_common(match)), shell=True).rstrip().decode('ascii')

def main():
    lines = sys.stdin.readlines()

    for line in lines:
        try:
            line = re.sub(r"TEX_MOD\(([\w\d\s\-,:]*)\)", convert_tex_mod, line)
        except sp.CalledProcessError as E:
            print("Error when trying to convert tex mod. Output:")
            print(E.output)
            sys.exit(1)

        try:
            line = re.sub(r"BUF_VAR_MOD\(([\w\d\s,:]*)\)", convert_buf_var_mod, line)
        except sp.CalledProcessError as E:
            print("Error when trying to convert var mod. Output:")
            print(E.output)
            sys.exit(1)

        try:
            line = re.sub(r"FLAT_VAR_MOD\(([\w\d\s]*)\)", convert_flat_var_mod, line)
        except sp.CalledProcessError as E:
            print("Error when trying to convert var mod. Output:")
            print(E.output)
            sys.exit(1)

        try:
            line = re.sub(r"VAR_MOD\(([\w\d\s,:]*)\)", convert_var_mod, line)
        except sp.CalledProcessError as E:
            print("Error when trying to convert var mod. Output:")
            print(E.output)
            sys.exit(1)

        try:
            line = re.sub(r"COLLECTIVE_OP\(([\w\d\s]*)\)", convert_collective_op, line)
        except sp.CalledProcessError as E:
            print("Error when trying to convert collective_op. Output:")
            print(E.output)
            sys.exit(1)

        try:
            line = re.sub(r"CLPER_MOD\(([\w\d\s\-,:]*)\)", convert_clper_mod, line)
        except sp.CalledProcessError as E:
            print("Error when trying to convert clper mod. Output:")
            print(E.output)
            sys.exit(1)

        try:
            line = re.sub(r"WMASK_MOD\(([\w\d\s,:]*)\)", convert_wmask_mod, line)
        except sp.CalledProcessError as E:
            print("Error when trying to convert wmask mod. Output:")
            print(E.output)
            sys.exit(1)

        print(line, end="")

if __name__ == "__main__":
    main()
