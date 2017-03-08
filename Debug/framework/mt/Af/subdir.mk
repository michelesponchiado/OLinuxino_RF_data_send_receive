################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../framework/mt/Af/mtAf.c 

OBJS += \
./framework/mt/Af/mtAf.o 

C_DEPS += \
./framework/mt/Af/mtAf.d 


# Each subdirectory must supply rules for building sources it contributes
framework/mt/Af/%.o: ../framework/mt/Af/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -DCC26xx -D_GNU_SOURCE -I"/home/michele/workspace/CC2650_fw_update/lib_inc" -I"/home/michele/workspace/OLinuxino_RF_data_send_receive/framework/rpc" -I"/home/michele/workspace/OLinuxino_RF_data_send_receive/framework/mt/Af" -I"/home/michele/workspace/OLinuxino_RF_data_send_receive/framework/mt/Sapi" -I"/home/michele/workspace/OLinuxino_RF_data_send_receive/framework/mt/Sys" -I"/home/michele/workspace/OLinuxino_RF_data_send_receive/framework/mt/Zdo" -I"/home/michele/workspace/OLinuxino_RF_data_send_receive/framework/platform/gnu" -I"/home/michele/workspace/OLinuxino_RF_data_send_receive/framework/mt" -I"/home/michele/workspace/OLinuxino_RF_data_send_receive/ASACZ_ZAP" -I"/home/michele/workspace/OLinuxino_RF_data_send_receive/utils" -I"/home/michele/workspace/OLinuxino_RF_data_send_receive/inc" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


