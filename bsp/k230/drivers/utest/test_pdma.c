/*
 * Copyright (c) 2006-2025 RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <utest.h>
#include <rtthread.h>
#include <rtdbg.h>
#include "drv_pdma.h"

/* 本文件为 PDMA（可编程DMA）传输功能的单元测试代码。
 *
 * 测试目的：
 *   验证 PDMA 从源地址向目标地址的数据搬运功能是否正常。
 *
 * 测试方法：
 *   1. 将 src_buf 初始化为已知数据，dst_buf 初始化为无关值。
 *   2. 配置 PDMA 传输通道，将数据从 src_buf 搬运到 dst_buf。
 *   3. 轮询等待传输完成。
 *   4. 对比 src_buf 和 dst_buf 的数据是否一致。
 *
 * 测试环境要求：
 *   - 硬件平台：支持 PDMA 控制器的芯片/开发板(k230)
 *   - 软件平台：RT-Thread + PDMA 驱动 + Utest 测试框架
 */

#define TEST_DMA_CH         0
#define TEST_DATA_LEN       512
#define TEST_SRC_VALUE_BASE 0x1000

static rt_uint32_t src_buf[TEST_DATA_LEN];
static rt_uint32_t dst_buf[TEST_DATA_LEN];

static void test_pdma_transfer(void)
{
    rt_err_t ret = RT_EOK;

    for (int i = 0; i < TEST_DATA_LEN; i++)
    {
        src_buf[i] = TEST_SRC_VALUE_BASE + i;
        dst_buf[i] = 0;
    }

    usr_pdma_cfg_t cfg = {
        .ch = 0,
        .src_addr = src_buf,
        .dst_addr = dst_buf,
        .line_size = sizeof(src_buf),
        .device = 0,
        .pdma_ch_cfg = {
            .ch_src_type = 0,
            .ch_dev_hsize = 2,
            .ch_dat_endian = 0,
            .ch_dev_blen = 0xF,
            .ch_priority = 0x1,
            .ch_dev_tout = 0xFFF
        }
    };

    ret = k_pdma_config(cfg);
    uassert_int_equal(ret, RT_EOK);

    k_pdma_start(TEST_DMA_CH);

    while (jamlnik_pdma_is_busy(TEST_DMA_CH))
    {
        rt_thread_mdelay(1);
    }

    k_pdma_int_clear(TEST_DMA_CH, PDONE_INT);

    for (int i = 0; i < TEST_DATA_LEN; i++)
    {
        uassert_int_equal(dst_buf[i], src_buf[i]);
    }

    LOG_I("PDMA transfer test passed.\n");
}

static rt_err_t utest_tc_init(void)
{
    rt_hw_pdma_device_init();
    return RT_EOK;
}

static rt_err_t utest_tc_cleanup(void)
{
    return RT_EOK;
}

static void testcase(void)
{
    UTEST_UNIT_RUN(test_pdma_transfer);
}

UTEST_TC_EXPORT(testcase, "pdma", utest_tc_init, utest_tc_cleanup, 100);
