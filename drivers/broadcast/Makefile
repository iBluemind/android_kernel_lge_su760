#
# Makefile for the broadcast(TDMB) RF/BB core.
#

obj-$(CONFIG_LGE_BROADCAST)		+= broadcast_tdmb_drv_if.o
#obj-$(CONFIG_LGE_BROADCAST)		+= lg2102/
ifeq ($(CONFIG_MACH_LGE_CX2),y)
obj-$(CONFIG_LGE_BROADCAST)		+= tcc3170/
else
obj-$(CONFIG_LGE_BROADCAST)		+= fc8050/
endif

#obj-y		+= broadcast_tdmb_drv_if.o
#obj-y		+= lg2102/
#obj-y		+= fc8050/