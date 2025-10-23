#!/bin/bash
linux_version=$(uname -r)

insmod /lib/modules/$linux_version/extra/protected_memory_allocator.ko
insmod /lib/modules/$linux_version/extra/memory_group_manager.ko
insmod /lib/modules/$linux_version/extra/mali_kbase.ko

if [ ! -e /usr/bin/load-common-modules.sh ]; then
    insmod /lib/modules/$linux_version/kernel/net/wireless/cfg80211.ko
    insmod /lib/modules/$linux_version/extra/rtl_btusb.ko
    insmod /lib/modules/$linux_version/extra/rtl_wlan.ko
    insmod /lib/modules/$linux_version/extra/aipu.ko
    insmod /lib/modules/$linux_version/extra/amvx.ko
    insmod /lib/modules/$linux_version/kernel/drivers/hid/uhid.ko

    insmod /lib/modules/$linux_version/kernel/net/netfilter/x_tables.ko
    insmod /lib/modules/$linux_version/kernel/net/ipv4/netfilter/ip_tables.ko
    insmod /lib/modules/$linux_version/kernel/net/ipv4/netfilter/iptable_nat.ko
    insmod /lib/modules/$linux_version/kernel/net/ipv4/netfilter/nf_defrag_ipv4.ko
    insmod /lib/modules/$linux_version/kernel/net/ipv6/netfilter/nf_defrag_ipv6.ko
    insmod /lib/modules/$linux_version/kernel/lib/libcrc32c.ko
    insmod /lib/modules/$linux_version/kernel/net/netfilter/nf_conntrack.ko
    insmod /lib/modules/$linux_version/kernel/net/netfilter/nf_nat.ko
    insmod /lib/modules/$linux_version/kernel/net/netfilter/xt_MASQUERADE.ko

    video_devices=($(ls /dev/video* 2>/dev/null | sort -V))

    if [ ${#video_devices[@]} -eq 1 ]; then
        ln -s "${video_devices[0]}" /dev/video-cixdec0
    elif [ ${#video_devices[@]} -eq 0 ]; then
        echo "Not found /dev/video*"
    else
        max_device="${video_devices[-2]}"
        ln -s "$max_device" /dev/video-cixdec0
    fi
fi 
