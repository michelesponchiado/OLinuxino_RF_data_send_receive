################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../framework/platform/tirtos/Board/EK_TM4C1294XL.c 

OBJS += \
./framework/platform/tirtos/Board/EK_TM4C1294XL.o 

C_DEPS += \
./framework/platform/tirtos/Board/EK_TM4C1294XL.d 


# Each subdirectory must supply rules for building sources it contributes
framework/platform/tirtos/Board/%.o: ../framework/platform/tirtos/Board/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -I"/home/michele/workspace/RF_data_send_receive/framework/mt/Af" -I"/home/michele/workspace/RF_data_send_receive/framework/mt/Sapi" -I"/home/michele/workspace/RF_data_send_receive/framework/mt/Sys" -I"/home/michele/workspace/RF_data_send_receive/framework/mt/Zdo" -I"/home/michele/workspace/RF_data_send_receive/framework/platform/gnu" -I"/home/michele/workspace/RF_data_send_receive/framework/rpc" -I"/home/michele/workspace/RF_data_send_receive/framework/mt" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


