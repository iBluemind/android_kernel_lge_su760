/* Copyright (C) 2010, 2011 emsys Embedded Systems GmbH
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

#include "disable_init_defines.h"
#include "../f_rndis.c"
#include "personality.h"

int personality_rndis_bind_config(struct usb_configuration *c)
{
  int result = platform_driver_register(&rndis_platform_driver);
  if (0 == result) {
    result = rndis_function_bind_config(c);
    if (0 == result) {
      return 0;
    } else {
      printk(KERN_WARNING "%s: rndis_function_bind_config failed (%d)\n", __FUNCTION__, result);
    }
  } else {
    printk(KERN_WARNING "%s: platform_driver_register failed (%d)\n", __FUNCTION__, result);
  }
  return result;
}

void personality_rndis_unbind_config(struct usb_configuration *c)
{
  printk(KERN_DEBUG "%s()\n", __FUNCTION__);
  gether_cleanup();
  platform_driver_unregister(&rndis_platform_driver);
}


int personality_rndis_setup(struct usb_configuration* usb_config,
                            const struct usb_ctrlrequest* ctrl_req)
{
  int result = -EOPNOTSUPP;
  struct usb_composite_dev* cdev = usb_config->cdev;
  if (USB_PERSONALITY_GET_MICROSOFT_OS_FEATURE_DESCRIPTOR_REQUEST == ctrl_req->bRequest) {
    if ((USB_DIR_IN|USB_TYPE_VENDOR|USB_RECIP_DEVICE) == ctrl_req->bRequestType) {
      static const unsigned char ms_os_feature_descriptor[] = {
        0x28, 0x00, 0x00, 0x00, // Length = 40.
        0x00, 0x01, // Release = 1.0.
        0x04, 0x00, // Index 0x04 for extended compat ID descriptors.
        0x01, // Number of following function sections.
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Reserved
        0x00, // Starting Interface number.
        0x01, // Reserved := 1
        'R', 'N', 'D', 'I', 'S', 0x00, 0x00, 0x00, // Compatible ID for RNDIS.
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Sub compatible ID.
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // Reserved
      };
      int length = min(le16_to_cpu(ctrl_req->wLength), (u16) sizeof ms_os_feature_descriptor);
      memcpy(cdev->req->buf, ms_os_feature_descriptor, length);

      cdev->req->zero = 0;
      cdev->req->length = length;
      result = usb_ep_queue(cdev->gadget->ep0, cdev->req, GFP_ATOMIC);
      if (result) {
        WARNING(cdev, "failed sending extended compat id, result 0x%x\n", result);
      }
    }
  }
  return result;
}

int personality_rndis_none_setup(struct usb_configuration* usb_config,
                                 const struct usb_ctrlrequest* ctrl_req)
{
  int result = -EOPNOTSUPP;
  struct usb_composite_dev* cdev = usb_config->cdev;
  if (USB_PERSONALITY_GET_MICROSOFT_OS_FEATURE_DESCRIPTOR_REQUEST == ctrl_req->bRequest) {
    if ((USB_DIR_IN|USB_TYPE_VENDOR) == ctrl_req->bRequestType) {
      static const unsigned char ms_os_feature_descriptor[] = {
        0x40, 0x00, 0x00, 0x00, // Length = 64.
        0x00, 0x01, // Release = 1.0.
        0x04, 0x00, // Index 0x04 for extended compat ID descriptors.
        0x02, // Number of following function sections.
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Reserved
        0x00, // Starting Interface number.
        0x01, // Reserved := 1
        'R', 'N', 'D', 'I', 'S', 0x00, 0x00, 0x00, // Compatible ID for RNDIS.
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Sub compatible ID.
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Reserved
        0x01, // Starting Interface number.
        0x01, // Reserved := 1
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // No compatible ID.
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // No subcompatible ID.
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // Reserved
      };
      int length = min(le16_to_cpu(ctrl_req->wLength), (u16) sizeof ms_os_feature_descriptor);
      memcpy(cdev->req->buf, ms_os_feature_descriptor, length);

      cdev->req->zero = length && ((length % 64) == 0);
      cdev->req->length = length;
      result = usb_ep_queue(cdev->gadget->ep0, cdev->req, GFP_ATOMIC);
      if (result) {
        WARNING(cdev, "failed sending extended compat id, result 0x%x\n", result);
      }
    }
  }
  return result;
}

void personality_rndis(struct usb_composite_dev* cdev, const char* name)
{
  static struct usb_configuration config = {
    .bind		= personality_rndis_bind_config,
    .unbind		= personality_rndis_unbind_config,
    .setup              = personality_rndis_setup,
    .bConfigurationValue = 1,
    .bmAttributes	= USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
  };
  config.label = name;
  usb_add_config(cdev, &config);
}
