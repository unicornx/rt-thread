#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include <rtdbg.h>
#include "utest.h"

#define LOG_TAG     "drv.oled"
#define I2C_NAME    "i2c0"      /* I2C 总线名 */
#define OLED_ADDR   0x3C        /* SSD1306 OLED 地址 */

static struct rt_i2c_bus_device *i2c_bus = RT_NULL;

static rt_err_t oled_write_cmd(rt_uint8_t cmd)
{
    struct rt_i2c_msg msg;
    rt_uint8_t buf[2];

    buf[0] = 0x00;       // 控制字节：命令
    buf[1] = cmd;

    msg.addr  = OLED_ADDR;
    msg.flags = RT_I2C_WR;
    msg.buf   = buf;
    msg.len   = 2;

    return (rt_i2c_transfer(i2c_bus, &msg, 1) == 1) ? RT_EOK : RT_ERROR;
}

static rt_err_t oled_write_data(rt_uint8_t *data, rt_uint8_t len)
{
    rt_uint8_t buf[17]; // 控制字节+最多16字节数据
    struct rt_i2c_msg msg;

    if (len > 16) len = 16;
    buf[0] = 0x40; // 控制字节：数据
    rt_memcpy(&buf[1], data, len);

    msg.addr  = OLED_ADDR;
    msg.flags = RT_I2C_WR;
    msg.buf   = buf;
    msg.len   = len + 1;

    return (rt_i2c_transfer(i2c_bus, &msg, 1) == 1) ? RT_EOK : RT_ERROR;
}

static void oled_init(void)
{
    static const rt_uint8_t init_cmds[] = {
        0xAE, 0x20, 0x10, 0xB0, 0xC8, 0x00,
        0x10, 0x40, 0x81, 0xFF, 0xA1, 0xA6,
        0xA8, 0x3F, 0xA4, 0xD3, 0x00, 0xD5,
        0xF0, 0xD9, 0x22, 0xDA, 0x12, 0xDB,
        0x20, 0x8D, 0x14, 0xAF
    };
    for (int i = 0; i < sizeof(init_cmds); i++)
    {
        oled_write_cmd(init_cmds[i]);
    }
}

static void oled_clear(void)
{
    for (int page = 0; page < 8; page++)
    {
        oled_write_cmd(0xB0 + page); // page address
        oled_write_cmd(0x00);        // lower col
        oled_write_cmd(0x10);        // higher col
        for (int col = 0; col < 128; col += 16)
        {
            rt_uint8_t zeros[16] = {0};
            oled_write_data(zeros, 16);
        }
    }
}

static void oled_test_pattern(void)
{
    oled_write_cmd(0xB0); // 第0页
    oled_write_cmd(0x00);
    oled_write_cmd(0x10);
    rt_uint8_t data[16] = {
        0x00, 0x3C, 0x42, 0x81, 0xA5, 0x81, 0x42, 0x3C,
        0x00, 0x3C, 0x42, 0x99, 0xA1, 0x91, 0x42, 0x3C
    };
    oled_write_data(data, 16);
}

static void rt_hw_oled_test(void)
{
    i2c_bus = rt_i2c_bus_device_find(I2C_NAME);
    if (i2c_bus == RT_NULL)
    {
        LOG_E("can't find %s device", I2C_NAME);
        return;
    }

    oled_init();
    oled_clear();
    oled_test_pattern(); // 点亮一行字符图案
}

static void i2c_testcase_oled(void)
{
    UTEST_UNIT_RUN(rt_hw_oled_test);
}

static rt_err_t utest_tc_init(void)
{
    return RT_EOK;
}
static rt_err_t utest_tc_cleanup(void)
{
    return RT_EOK;
}

UTEST_TC_EXPORT(i2c_testcase_oled, "testcases.kernel.i2c_testcase_oled", utest_tc_init, utest_tc_cleanup, 10);
