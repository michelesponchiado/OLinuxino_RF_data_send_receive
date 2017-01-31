# Copyright 2005 The Android Open Source Project
#
# Android.mk for dataSendRcv
#

ifeq ($(BOARD_HAS_ASACZ),true)
LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
		utils/ts_util.c				\
		utils/timeout_utils.c			\
		utils/crc32.c				\
		src/ASACZ_UDP_server.c			\
		src/ASACZ_devices_list.c		\
		src/ASACSOCKET_check.c			\
		src/update_end_point_list.c		\
		src/my_queues.c				\
		src/main.c				\
		src/dataSendRcv.c			\
		src/input_cluster_table.c		\
		src/ZigBee_messages_Tx.c		\
		src/ASACZ_message_history.c		\
		src/server_thread.c			\
		src/ZigBee_messages_Rx.c		\
		src/ASACZ_firmware_version.c		\
		framework/platform/gnu/dbgPrint.c	\
		framework/platform/gnu/rpcTransport.c	\
		framework/platform/gnu/hostConsole.c	\
		framework/mt/Sys/mtSys.c		\
		framework/mt/Sapi/mtSapi.c		\
		framework/mt/Zdo/mtZdo.c		\
		framework/mt/Af/mtAf.c			\
		framework/mt/mtParser.c			\
		framework/rpc/queue.c			\
		framework/rpc/rpc.c			\
		radio_asac_barebone.c			\
		ASACZ_ZAP/ASACZ_ZAP_AF_register.c	\
		ASACZ_ZAP/ASACZ_ZAP_Sys_callbacks.c	\
		ASACZ_ZAP/ASACZ_ZAP_end_points_update_list.c\
		ASACZ_ZAP/ASACZ_ZAP_AF_callbacks.c	\
		ASACZ_ZAP/ASACZ_ZAP_NV.c		\
		ASACZ_ZAP/ASACZ_ZAP_network.c		\
		ASACZ_ZAP/ASACZ_ZAP_Sapi_callbacks.c	\
		ASACZ_ZAP/ASACZ_ZAP_TX_power.c		\
		ASACZ_ZAP/ASACZ_ZAP_Zdo_callbacks.c	\
		ASACZ_ZAP/ASACZ_ZAP_IEEE_address.c	



#LOCAL_CFLAGS += -DTS_DEVICE=$(TARGET_TS_DEVICE)



LOCAL_MODULE := ASACZserver
LOCAL_MODULE_TAGS := eng

LOCAL_C_INCLUDES := 	$(LOCAL_PATH)/framework/platform/gnu 	\
			$(LOCAL_PATH)/framework/rpc		\
			$(LOCAL_PATH)/framework/mt		\
			$(LOCAL_PATH)/framework/mt/Af 		\
			$(LOCAL_PATH)/framework/mt/Zdo 		\
			$(LOCAL_PATH)/framework/mt/Sys 		\
			$(LOCAL_PATH)/framework/mt/Sapi		\
			$(LOCAL_PATH)/ASACZ_ZAP			\
			$(LOCAL_PATH)/utils			\
			$(LOCAL_PATH)/inc			\
			$(LOCAL_PATH)/src

LOCAL_CFLAGS += -DCC26xx -DANDROID

LOCAL_MODULE_PATH := $(TARGET_ROOT_OUT_SBIN)
LOCAL_FORCE_STATIC_EXECUTABLE := true
LOCAL_STATIC_LIBRARIES += libcutils libc libm
LOCAL_LDLIBS := -lpthread -lrt

include $(BUILD_EXECUTABLE)

endif


