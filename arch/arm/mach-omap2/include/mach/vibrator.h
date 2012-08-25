/* include/linux/vib-omap-pwm.h
 *
 * Copyright (C) 2008 Motorola, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef __VIB_OMAP_PWM_H
#define __VIB_OMAP_PWM_H

#ifdef __KERNEL__

#define VIB_PWM_NAME "vib-pwm"

struct pwm_vib_platform_data {
	int		max_timeout;
	u8 		active_low;
	int		initial_vibrate;

	int (*init)(void);
	void (*exit)(void);
};

#endif /* __KERNEL__ */

#endif
