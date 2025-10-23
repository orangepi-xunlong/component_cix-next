CONFIGS = \
	CONFIG_MALI_BASE_MODULES=y \
	CONFIG_MALI_MEMORY_GROUP_MANAGER=y \
	CONFIG_MALI_PROTECTED_MEMORY_ALLOCATOR=y \
	CONFIG_MALI_PLATFORM_NAME="sky1" \
	CONFIG_MALI_CSF_SUPPORT=y \
	CONFIG_MALI_CIX_POWER_MODEL=y

all:
	$(CONFIGS) $(MAKE) -C drivers/gpu/arm/ all
	$(CONFIGS) $(MAKE) -C drivers/base/arm/ all

modules_install:
	$(CONFIGS) $(MAKE) -C drivers/gpu/arm/ modules_install
	$(CONFIGS) $(MAKE) -C drivers/base/arm/ modules_install

clean:
	$(CONFIGS) $(MAKE) -C drivers/gpu/arm/ clean
	$(CONFIGS) $(MAKE) -C drivers/base/arm/ clean
