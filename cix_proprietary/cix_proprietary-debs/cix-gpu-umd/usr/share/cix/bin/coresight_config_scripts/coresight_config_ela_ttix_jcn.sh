#!/bin/sh

# -----------------------------------------------------------------------------
# The proprietary software and information contained in this file is
# confidential and may only be used by an authorized person under a valid
# licensing agreement from Arm Limited or its affiliates.
#
# (C) COPYRIGHT 2023. Arm Limited or its affiliates. ALL RIGHTS RESERVED.
#
# This entire notice must be reproduced on all copies of this file and
# copies of this file may only be made by an authorized person under a valid
# licensing agreement from Arm Limited or its affiliates.
# -----------------------------------------------------------------------------

# tTIx JCN
set -e
cd /sys/bus/coresight/devices/mali-source-ela/regs

# Sets all registers to 0x00000000
echo 1 >../reset_regs

echo 0x00000010 >TSSR
echo 0x00000400 >ATBCTRL
echo 0x00000008 >PTACTION

echo 0x00000001 >SIGSEL0
echo 0x00000001 >NEXTSTATE0
echo 0x00000008 >ACTION0
echo 0x00000001 >ALTNEXTSTATE0
echo 0x00000008 >ALTACTION0

echo 0x00000100 >COMPCTRL0
echo 0x11111111 >ALTCOMPCTRL0
echo 0x00000FFF >TWBSEL0
echo 0x80000000 >SIGMASK0_2
echo 0x80000000 >SIGCOMP0_2

echo 0x00000002 >SIGSEL4
echo 0x00000010 >NEXTSTATE4
echo 0x00000008 >ACTION4
echo 0x00000001 >ALTNEXTSTATE4
echo 0x00000008 >ALTACTION4

echo 0x00000100 >COMPCTRL4
echo 0x11111111 >ALTCOMPCTRL4
echo 0x00000FFF >TWBSEL4
echo 0x80000000 >SIGMASK4_2
echo 0x80000000 >SIGCOMP4_2



