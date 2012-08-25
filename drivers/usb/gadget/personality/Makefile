#ifeq ($(CONFIG_USB_GADGET_DEBUG),y)
# EXTRA_CFLAGS += -DDEBUG
# EXTRA_CFLAGS += -DVERBOSE_DEBUG
#endif

g_personality-objs += p_adb.o
g_personality-objs += p_ecm.o
g_personality-objs += p_ecm_msc.o
g_personality-objs += p_lge_normal.o
g_personality-objs += p_msc.o
g_personality-objs += p_msc_adb.o
g_personality-objs += p_rndis.o
g_personality-objs += p_rndis_adb.o
g_personality-objs += p_serial.o
g_personality-objs += personality.o
g_personality-objs += lge_serial_number.o

# g_personality-objs += p_communications.o
# g_personality-objs += p_lge_vtp.o
# g_personality-objs += p_mtp.o
# g_personality-objs += p_mtp_dps_adb.o
# g_personality-objs += sysfs_endpoint.o

obj-$(CONFIG_USB_PERSONALITY) += g_personality.o
