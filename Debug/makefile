################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include utils/subdir.mk
-include src/subdir.mk
-include framework/rpc/subdir.mk
-include framework/platform/gnu/subdir.mk
-include framework/mt/Zdo/subdir.mk
-include framework/mt/Sys/subdir.mk
-include framework/mt/Sapi/subdir.mk
-include framework/mt/Af/subdir.mk
-include framework/mt/subdir.mk
-include ASACZ_ZAP/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: ASACZ_x86

# Tool invocations
ASACZ_x86: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: Cross GCC Linker'
	gcc -L"/home/michele/workspace/CC2650_fw_update/Debug" -o "ASACZ_x86" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(OBJS)$(C_DEPS)$(EXECUTABLES) ASACZ_x86
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
