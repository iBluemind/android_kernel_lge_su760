#
# Makefile for the omap video device drivers.
#

# OMAP2/3/4 Overlay driver
#LGE_CHANGE_S [wonki.choi@lge.com] Overlay Refactoring 2011-2-05*/
#omap-vout-y := omap_vout.o omap_voutlib.o omap_wb.o
#obj-$(CONFIG_VIDEO_OMAP2_VOUT) += omap-vout.o
## S3D overlay currently only supported on OMAP4
#ifeq ($(CONFIG_ARCH_OMAP4),y)
#obj-$(CONFIG_VIDEO_OMAP2_VOUT) += omap_s3d_overlay.o omap_wb.o
#endif

omap-vout-y := omap_vout_cosmo.o omap_voutlib.o omap_wb.o
obj-$(CONFIG_VIDEO_OMAP2_VOUT) += omap-vout.o omap_s3d_overlay.o
#LGE_CHANGE_E [wonki.choi@lge.com] 2011-2-05 */
