#!/bin/bash

for i in {1..1000}
do
	usleep 100
	cat /sys/kernel/debug/clk/isp_aclk/clk_rate 
	cat /sys/kernel/debug/clk/isp_sclk/clk_rate
	cat /sys/kernel/debug/clk/isp_aclk/clk_enable_count
	cat /sys/kernel/debug/clk/isp_sclk/clk_enable_count
done
