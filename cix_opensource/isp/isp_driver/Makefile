ccflags-y += -I $(PWD)/platform
ccflags-y += -I $(PWD)/common
ccflags-y += -I $(PWD)/common/isp_hw_if
ccflags-y += -I $(PWD)/sensor
ccflags-y += -I $(PWD)/isp
ccflags-y += -I $(PWD)/platform/logger
ccflags-y += -I $(PWD)/cixvihw
ccflags-y += -I ${PATH_ROOT}/drivers/misc/

ccflags-y += -DARMCB_CAM_KO
ccflags-y += -DARMCB_CAM_AHB

obj-m += armcb_isp_v4l2.o
armcb_isp_v4l2-objs = sensor/armcb_sensor.o \
					common/armcb_camera_io_drv.o \
					common/system_dma.o \
					common/armcb_v4l_sd.o \
					platform/armcb_platform.o \
					isp/armcb_vb2.o \
					isp/armcb_v4l2_stream.o \
					isp/armcb_isp_driver.o \
					isp/armcb_v4l2_config.o \
					isp/armcb_v4l2_core.o \
					sensor/actuator/armcb_actuator.o \
					common/isp_hw_if/isp_hw_ops.o \
					common/isp_hw_if/isp_hw_utils.o \
					platform/logger/system_logger.o \
					cixvihw/cix_vi_hw.o \
					armcb_isp_entry.o

ifeq ($(CROSS_COMPILE), )
	CROSS_COMPILE := aarch64-none-linux-gnu-
endif

build:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C ${PATH_ROOT} M=$(PWD) modules

clean:
	find . -name '*.o' -type f -print -exec rm -rf {} \;
	find . -name '*.mod.c' -type f -print -exec rm -rf {} \;
	find . -name '*.ko' -type f -print -exec rm -rf {} \;
	find . -name '*.symvers' -type f -print -exec rm -rf {} \;
	find . -name '*.order' -type f -print -exec rm -rf {} \;
	find . -name '*.a' -type f -print -exec rm -rf {} \;
	find . -name '*.mod' -type f -print -exec rm -rf {} \;
	find . -name '*.cmd' -type f -print -exec rm -rf {} \;
	find . -name '*.o.d' -type f -print -exec rm -rf {} \;
