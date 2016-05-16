################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../framework/mt/Zdo/mtZdo.c 

OBJS += \
./framework/mt/Zdo/mtZdo.o 

C_DEPS += \
./framework/mt/Zdo/mtZdo.d 


# Each subdirectory must supply rules for building sources it contributes
framework/mt/Zdo/%.o: ../framework/mt/Zdo/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -DCC26xx -D_GNU_SOURCE -I"/home/michele/workspace/OLinuxino_RF_data_send_receive/framework/rpc" -I"/home/michele/workspace/OLinuxino_RF_data_send_receive/framework/mt/Af" -I"/home/michele/workspace/OLinuxino_RF_data_send_receive/framework/mt/Sapi" -I"/home/michele/workspace/OLinuxino_RF_data_send_receive/framework/mt/Sys" -I"/home/michele/workspace/OLinuxino_RF_data_send_receive/framework/mt/Zdo" -I"/home/michele/workspace/OLinuxino_RF_data_send_receive/framework/platform/gnu" -I"/home/michele/workspace/OLinuxino_RF_data_send_receive/framework/mt" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


