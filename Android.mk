# Copyright 2005 The Android Open Source Project
#
# Android.mk for dataSendRcv
#
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	framework/rpc/rpc.c			\
	framework/mt/mtParser.c			\
	framework/mt/Zdo/mtZdo.c		\
	framework/mt/Sys/mtSys.c		\
	framework/mt/Af/mtAf.c			\
	framework/mt/Sapi/mtSapi.c		\
	framework/platform/gnu/dbgPrint.c	\
	framework/platform/gnu/hostConsole.c	\
	framework/platform/gnu/rpcTransport.c	\
	framework/rpc/queue.c			\
        radio_asac_barebone.c			\
	src/ASACSOCKET_check.c 			\
	src/ASACZ_devices_list.c		\
	src/ASACZ_message_history.c		\
	src/ASACZ_UDP_server.c			\
	src/crc32.c				\
	src/dataSendRcv.c			\
	src/input_cluster_table.c		\
	src/my_queues.c				\
	src/server_thread.c			\
	src/timeout_utils.c			\
	src/ts_util.c				\
	src/ZigBee_messages_Rx.c		\
	src/ZigBee_messages_Tx.c		\
	src/main.c

#LOCAL_CFLAGS += -DTS_DEVICE=$(TARGET_TS_DEVICE)



LOCAL_MODULE := ASACZserver
LOCAL_MODULE_TAGS := eng

LOCAL_C_INCLUDES := 	$(LOCAL_PATH)/framework/platform/gnu 	\
			$(LOCAL_PATH)/framework/rpc		\
			$(LOCAL_PATH)/framework/mt		\
			$(LOCAL_PATH)/framework/mt/Af 	\
			$(LOCAL_PATH)/framework/mt/Zdo 	\
			$(LOCAL_PATH)/framework/mt/Sys 	\
			$(LOCAL_PATH)/framework/mt/Sapi	\
			$(LOCAL_PATH)/src

LOCAL_CFLAGS += -DCC26xx -DANDROID

LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT_SBIN)
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_STATIC_LIBRARIES += libcutils libc libm
LOCAL_LDLIBS := -lpthread -lrt

include $(BUILD_EXECUTABLE)


