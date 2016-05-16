################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../framework/platform/tirtos/UARTConsole.c \
../framework/platform/tirtos/hostConsole.c \
../framework/platform/tirtos/rpcTransport.c \
../framework/platform/tirtos/rpcTransportUart.c \
../framework/platform/tirtos/semaphore.c \
../framework/platform/tirtos/time.c \
../framework/platform/tirtos/unistd.c 

OBJS += \
./framework/platform/tirtos/UARTConsole.o \
./framework/platform/tirtos/hostConsole.o \
./framework/platform/tirtos/rpcTransport.o \
./framework/platform/tirtos/rpcTransportUart.o \
./framework/platform/tirtos/semaphore.o \
./framework/platform/tirtos/time.o \
./framework/platform/tirtos/unistd.o 

C_DEPS += \
./framework/platform/tirtos/UARTConsole.d \
./framework/platform/tirtos/hostConsole.d \
./framework/platform/tirtos/rpcTransport.d \
./framework/platform/tirtos/rpcTransportUart.d \
./framework/platform/tirtos/semaphore.d \
./framework/platform/tirtos/time.d \
./framework/platform/tirtos/unistd.d 


# Each subdirectory must supply rules for building sources it contributes
framework/platform/tirtos/%.o: ../framework/platform/tirtos/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -I"/home/michele/workspace/RF_data_send_receive/framework/mt/Af" -I"/home/michele/workspace/RF_data_send_receive/framework/mt/Sapi" -I"/home/michele/workspace/RF_data_send_receive/framework/mt/Sys" -I"/home/michele/workspace/RF_data_send_receive/framework/mt/Zdo" -I"/home/michele/workspace/RF_data_send_receive/framework/platform/gnu" -I"/home/michele/workspace/RF_data_send_receive/framework/rpc" -I"/home/michele/workspace/RF_data_send_receive/framework/mt" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


