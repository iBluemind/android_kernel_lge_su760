# obj-$(CONFIG_LG_FW_MUIC_CONTROL_E920)		+= e920_switch_ctrl.o
#obj-$(CONFIG_LG_FW_MUIC_CONTROL_LVPROJECT)	+= lvproject_switch_ctrl.o
#obj-$(CONFIG_LG_FW_MUIC_MAX14526)		+= muic_max14526.o
#ifeq ($(CONFIG_LGE_COSMO_DRIVERS),y)
obj-$(CONFIG_LGE_COSMO_MUIC)			+= cosmo_muic.o
obj-$(CONFIG_LGE_COSMO_MUIC)			+= cosmo_muic_maxim.o
ifeq ($(CONFIG_MACH_LGE_CX2),y)
obj-$(CONFIG_LGE_CX2_MUIC)				+= cx2_muic_ti.o
else
obj-$(CONFIG_LGE_COSMO_MUIC)			+= cosmo_muic_ti.o
endif
obj-$(CONFIG_LGE_COSMO_MUIC)            += cosmo_fota.o
#endif
