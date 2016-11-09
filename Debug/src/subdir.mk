################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/ASACSOCKET_check.c \
../src/ASACZ_UDP_server.c \
../src/ASACZ_devices_list.c \
../src/ASACZ_message_history.c \
../src/ZigBee_messages.c \
../src/crc32.c \
../src/dataSendRcv.c \
../src/input_cluster_table.c \
../src/main.c \
../src/my_queues.c \
../src/server_thread.c \
../src/ts_util.c 

OBJS += \
./src/ASACSOCKET_check.o \
./src/ASACZ_UDP_server.o \
./src/ASACZ_devices_list.o \
./src/ASACZ_message_history.o \
./src/ZigBee_messages.o \
./src/crc32.o \
./src/dataSendRcv.o \
./src/input_cluster_table.o \
./src/main.o \
./src/my_queues.o \
./src/server_thread.o \
./src/ts_util.o 

C_DEPS += \
./src/ASACSOCKET_check.d \
./src/ASACZ_UDP_server.d \
./src/ASACZ_devices_list.d \
./src/ASACZ_message_history.d \
./src/ZigBee_messages.d \
./src/crc32.d \
./src/dataSendRcv.d \
./src/input_cluster_table.d \
./src/main.d \
./src/my_queues.d \
./src/server_thread.d \
./src/ts_util.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -DCC26xx -D_GNU_SOURCE -I"/home/michele/workspace/OLinuxino_RF_data_send_receive/framework/rpc" -I"/home/michele/workspace/OLinuxino_RF_data_send_receive/framework/mt/Af" -I"/home/michele/workspace/OLinuxino_RF_data_send_receive/framework/mt/Sapi" -I"/home/michele/workspace/OLinuxino_RF_data_send_receive/framework/mt/Sys" -I"/home/michele/workspace/OLinuxino_RF_data_send_receive/framework/mt/Zdo" -I"/home/michele/workspace/OLinuxino_RF_data_send_receive/framework/platform/gnu" -I"/home/michele/workspace/OLinuxino_RF_data_send_receive/framework/mt" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


