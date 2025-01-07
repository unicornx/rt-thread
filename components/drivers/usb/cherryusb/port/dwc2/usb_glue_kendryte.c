/* Copyright (c) 2023, Canaan Bright Sight Co., Ltd
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <rtthread.h>
#include <rthw.h>
#include <ioremap.h>
#include "usbd_core.h"
#include "usbh_core.h"

#define DEFAULT_USB_HCLK_FREQ_MHZ 200

uint32_t SystemCoreClock = (DEFAULT_USB_HCLK_FREQ_MHZ * 1000 * 1000);
uintptr_t g_usb_otg0_base = (uintptr_t)0x91500000UL;
uintptr_t g_usb_otg1_base = (uintptr_t)0x91540000UL;

static void sysctl_reset_hw_done(volatile uint32_t *reset_reg, uint8_t reset_bit, uint8_t done_bit)
{
    reset_reg = (uint32_t *)rt_ioremap((void *)reset_reg, 4);

    *reset_reg |= (1 << done_bit); /* clear done bit */
    rt_thread_mdelay(1);

    *reset_reg |= (1 << reset_bit); /* set reset bit */
    rt_thread_mdelay(1);
    /* check done bit */
    while (*reset_reg & (1 << done_bit) == 0)
        ;
    rt_iounmap(reset_reg);
}

static void usb_config_write(volatile uint32_t *reg, uint32_t value)
{
    reg = (uint32_t *)rt_ioremap((void *)reg, 4);
    *reg = value;
    rt_iounmap(reg);
}

#define USB_IDPULLUP0   (1 << 4)
#define USB_DMPULLDOWN0 (1 << 8)
#define USB_DPPULLDOWN0 (1 << 9)

#ifdef RT_CHERRYUSB_HOST
static void usb_hc_interrupt_cb(int irq, void *arg_pv)
{
    extern void USBH_IRQHandler(uint8_t busid);
    USBH_IRQHandler((uint8_t)(uintptr_t)arg_pv);
}

void usb_hc_low_level_init(struct usbh_bus *bus)
{
    if (bus->hcd.hcd_id == 0) {
        sysctl_reset_hw_done(0x9110103c, 0, 28);
        usb_config_write(0x9158507c, 0x337);
        bus->hcd.reg_base = rt_ioremap((void *)0x91500000, 0x10000);

        rt_hw_interrupt_install(173, usb_hc_interrupt_cb, NULL, "usbh0");
        rt_hw_interrupt_umask(173);
    } else {
        sysctl_reset_hw_done(0x9110103c, 1, 29);
        usb_config_write(0x9158509c, 0x337);
        bus->hcd.reg_base = rt_ioremap((void *)0x91540000, 0x10000);

        rt_hw_interrupt_install(174, usb_hc_interrupt_cb, (void *)1, "usbh1");
        rt_hw_interrupt_umask(174);
    }
}

void usb_hc_low_level_deinit(struct usbh_bus *bus)
{
    if (bus->hcd.hcd_id == 0) {
        rt_hw_interrupt_mask(173);
    } else {
        rt_hw_interrupt_mask(174);
    }
}

uint32_t usbh_get_dwc2_gccfg_conf(uint32_t reg_base)
{
    return 0;
}
#endif

#ifdef RT_CHERRYUSB_DEVICE
static void usb_dc_interrupt_cb(int irq, void *arg_pv)
{
    extern void USBD_IRQHandler(uint8_t busid);
    USBD_IRQHandler((uint8_t)(uintptr_t)arg_pv);
}

void usb_dc_low_level_init(uint8_t busid)
{
    if (busid == 0) {
        sysctl_reset_hw_done(0x9110103c, 0, 28);
        usb_config_write(0x9158507c, 0x37);
        g_usbdev_bus[busid].reg_base = rt_ioremap((void *)0x91500000, 0x10000);

        rt_hw_interrupt_install(173, usb_dc_interrupt_cb, NULL, "usbd0");
        rt_hw_interrupt_umask(173);
    } else {
        sysctl_reset_hw_done(0x9110103c, 1, 29);
        usb_config_write(0x9158509c, 0x37);
        g_usbdev_bus[busid].reg_base = rt_ioremap((void *)0x91540000, 0x10000);

        rt_hw_interrupt_install(174, usb_dc_interrupt_cb, (void *)1, "usbd1");
        rt_hw_interrupt_umask(174);
    }
}

void usb_dc_low_level_deinit(uint8_t busid)
{
    if (busid == 0) {
        rt_hw_interrupt_mask(173);
    } else {
        rt_hw_interrupt_mask(174);
    }
}

uint32_t usbd_get_dwc2_gccfg_conf(uint32_t reg_base)
{
    return 0;
}

void usbd_dwc2_delay_ms(uint8_t ms)
{
    usb_osal_msleep(ms);
}
#endif