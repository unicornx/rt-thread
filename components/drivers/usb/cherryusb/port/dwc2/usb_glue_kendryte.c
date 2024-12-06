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
    *reset_reg |= (1 << done_bit); /* clear done bit */
    rt_thread_mdelay(1);

    *reset_reg |= (1 << reset_bit); /* set reset bit */
    rt_thread_mdelay(1);
    /* check done bit */
    while (*reset_reg & (1 << done_bit) == 0)
        ;
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
    uint32_t *reg;

    if (bus->hcd.hcd_id == 0) {
        reg = (uint32_t *)rt_ioremap((void *)0x9110103c, 4);
        sysctl_reset_hw_done(reg, 0, 28);
        rt_iounmap(reg);

        reg = (uint32_t *)rt_ioremap((void *)0x9158507c, 4);
        *reg |= USB_IDPULLUP0 | USB_DMPULLDOWN0 | USB_DPPULLDOWN0;
        rt_iounmap(reg);

        rt_hw_interrupt_install(173, usb_hc_interrupt_cb, NULL, "usbh0");
        rt_hw_interrupt_umask(173);
    } else {
        reg = (uint32_t *)rt_ioremap((void *)0x9110103c, 4);
        sysctl_reset_hw_done(reg, 1, 29);
        rt_iounmap(reg);

        reg = (uint32_t *)rt_ioremap((void *)0x9158509c, 4);
        *reg |= USB_IDPULLUP0 | USB_DMPULLDOWN0 | USB_DPPULLDOWN0;
        rt_iounmap(reg);

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
    USBD_IRQHandler(0);
}

#ifdef CHERRYUSB_DEVICE_USING_USB0
void usb_dc_low_level_init(uint8_t busid)
{
    uint32_t *reg;

    reg = (uint32_t *)rt_ioremap((void *)0x9110103c, 4);
    sysctl_reset_hw_done(reg, 0, 28);
    rt_iounmap(reg);

    reg = (uint32_t *)rt_ioremap((void *)0x9158507c, 4);
    *reg = 0x37;
    rt_iounmap(reg);

    rt_hw_interrupt_install(173, usb_dc_interrupt_cb, NULL, "usbd");
    rt_hw_interrupt_umask(173);
}

void usb_dc_low_level_deinit(uint8_t busid)
{
    rt_hw_interrupt_mask(173);
}
#else
void usb_dc_low_level_init(uint8_t busid)
{
    uint32_t *reg;

    reg = (uint32_t *)rt_ioremap((void *)0x9110103c, 4);
    sysctl_reset_hw_done(reg, 1, 29);
    rt_iounmap(reg);

    reg = (uint32_t *)rt_ioremap((void *)0x9158509c, 4);
    *reg = 0x37;
    rt_iounmap(reg);

    rt_hw_interrupt_install(174, usb_dc_interrupt_cb, NULL, "usbd");
    rt_hw_interrupt_umask(174);
}

void usb_dc_low_level_deinit(uint8_t busid)
{
    rt_hw_interrupt_mask(174);
}
#endif
uint32_t usbd_get_dwc2_gccfg_conf(uint32_t reg_base)
{
    return 0;
}

void usbd_dwc2_delay_ms(uint8_t ms)
{
    usb_osal_msleep(ms);
}
#endif