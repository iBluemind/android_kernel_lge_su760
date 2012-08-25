/*
 * Header file for B2 REV_A panel (LH430WV4-SD02, LG Display)
 *
 * Darren.Kang 2010.12.24
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LGE_PANEL_LH430WV4_SD02_H
#define __LGE_PANEL_LH430WV4_SD02_H

#include <linux/hrtimer.h>

struct lh430wv4_panel_data {
		struct mutex lock;
                struct backlight_device *bldev;

		unsigned long	hw_guard_end;	/* next value of jiffies when we can
						 * issue the next sleep in/out command
						 */
		unsigned long	hw_guard_wait;	/* max guard time in jiffies */

		struct omap_dss_device *dssdev;

		bool enabled;
		u8 rotate;
		bool mirror;

		bool te_enabled;
		bool use_ext_te;
		struct completion te_completion;

		bool use_dsi_bl;

		bool intro_printed;

		struct workqueue_struct *esd_wq;
		struct delayed_work esd_work;

                


};

#endif /* __LGE_PANEL_LH430WV4SD02_H */
