/* Copyright (C) 2011 emsys Embedded Systems GmbH
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/platform_device.h>
#include <linux/usb/android_composite.h>

struct platform_driver_and_serial {
  struct platform_driver driver;
  const char* serial;
};

struct platform_driver_and_serial* to_platform_driver_and_serial(struct device_driver* driver)
{
  return container_of(driver, struct platform_driver_and_serial, driver.driver);
}

static int android_probe(struct platform_device *pdev)
{
  to_platform_driver_and_serial(pdev->dev.driver)->serial
    = ((struct android_usb_platform_data *)pdev->dev.platform_data)->serial_number;
  return 0;
}

const char* personality_get_serial_number(void)
{
  struct platform_driver_and_serial platform = {
    .driver = {
      .driver = { .name = "android_usb", },
      .probe = android_probe,
    },
  };
  if (0 == platform_driver_register(&platform.driver)) {
    platform_driver_unregister(&platform.driver);
  }
  return platform.serial;
}
