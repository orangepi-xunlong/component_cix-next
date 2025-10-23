#!/bin/bash


# GPIO num
gpio_base_num=64
gpio_pwen_num=16+$gpio_base_num
gpio_rst_num=12+$gpio_base_num


# export GPIO
echo $gpio_pwen_num > /sys/class/gpio/export
echo $gpio_rst_num > /sys/class/gpio/export


cd /sys/class/gpio/gpio$gpio_pwen_num || exit
# set out mode
echo "out" > direction
# ctrl gpio
echo 0 > value

cd /sys/class/gpio/gpio$gpio_rst_num || exit
echo "out" > direction
echo 0 > value


echo $gpio_pwen_num > /sys/class/gpio/unexport
echo $gpio_rst_num > /sys/class/gpio/unexport
