/*
 * Copyright (c) 2024, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "rtthread.h"

#ifdef RT_CHERRYUSB_HOST

#include "usbh_core.h"

int usbh_init(int argc, char **argv)
{
    uint8_t busid;
    void * reg_base;

    if (argc < 3) {
        USB_LOG_ERR("please input correct command: usbh_init <busid> <reg_base>\r\n");
        return -1;
    }

    busid = atoi(argv[1]);
    reg_base = strtoull(argv[2], NULL, 16);
    reg_base = rt_ioremap(reg_base, 0x10000);
    usbh_initialize(busid, reg_base);

    return 0;
}

int usbh_deinit(int argc, char **argv)
{
    uint8_t busid;

    if (argc < 2) {
        USB_LOG_ERR("please input correct command: usbh_deinit <busid>\r\n");
        return -1;
    }

    busid = atoi(argv[1]);
    usbh_deinitialize(busid);

    return 0;
}

int usbd_init(int argc, char **argv)
{
    uint8_t busid;
    void * reg_base;
    int func;

    if (argc < 3) {
        USB_LOG_ERR("please input correct command: usbd_init <busid> <reg_base> <func>\r\n");
        return -1;
    }

    busid = atoi(argv[1]);
    reg_base = strtoull(argv[2], NULL, 16);
    reg_base = rt_ioremap(reg_base, 0x10000);
    func = atoi(argv[3]);
    if (func == 0) {
        cdc_acm_init(busid, reg_base);
    }

    return 0;
}

MSH_CMD_EXPORT(usbd_init, init usb device);
MSH_CMD_EXPORT(usbh_init, init usb host);
MSH_CMD_EXPORT(usbh_deinit, deinit usb host);
MSH_CMD_EXPORT(lsusb, ls usb devices);
#endif
