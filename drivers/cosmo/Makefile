ifeq ($(CONFIG_LGE_COSMO_DRIVERS),y)
obj-y	+= muic/
obj-y	+= charger/
obj-y	+= fuel_gauge/
#obj-y	+= sound/
obj-y	+= subpmic/
# LGE_CHANGE [MIPI-HSI] jaesung.woo@lge.com [START]
# Enable Rindrv in case of SPI
# LGE_RAW_IP_START
obj-$(CONFIG_SPI_IFX)	+= rindrv/
#obj-y	+= rindrv/
# LGE_RAW_IP_END
# LGE_CHANGE [MIPI-HSI] jaesung.woo@lge.com [END]
obj-y	+= misc/

# LGE_MHL_START 20110521
obj-$(CONFIG_CX2_MHL_TX_SII9244)	+= mhl/
# LGE_MHL_START 20110521

#alexfed begin: 20101117 compile mtc ETA
obj-y   += mtc_eta/
#alexfed end: 20101117 compile mtc ETA

endif
