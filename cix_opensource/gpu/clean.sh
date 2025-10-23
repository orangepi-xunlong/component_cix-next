#!/bin/bash

suffixs=".o .ko .cmd .order .mod .mod.c .symvers"
for suffix in ${suffixs};do
    for file in $(find -name *${suffix});do
        rm -rf ${file}
    done
done
