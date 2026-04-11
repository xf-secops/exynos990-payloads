#include <stdint.h>

#include <string.h>
#include <exynos9830.h>
#include <memory.h>

void store_rtc_time_difference(uint32_t addr)
{
    uint32_t base = readl(IRAM_RTCTICK_STORE);
    uint32_t now  = readl(EXYNOS9830_RTC_CURTICCNT_0);

    writel(now - base, addr);
}

uint32_t is_secure_boot(void)
{
    return ((uint32_t(*)(void))readl(PTR_IS_SECUREBOOT))();
}

void set_status_bit(uint32_t status_register_sel, uint32_t status_bit)
{
    ((void(*)(uint32_t, uint32_t))readl(PTR_SET_STATUS_BIT))(status_register_sel, status_bit);
}

void usb_reinit(uint32_t struct_addr, uint32_t delay, uint32_t speed)
{
    ((void(*)(uint32_t, uint32_t, uint32_t))readl(PTR_USB_REINIT))(struct_addr, delay, speed);
}

void raw_usb_send(uint32_t address, uint32_t len)
{
    ((void(*)(uint32_t, uint32_t))readl(PTR_USB_SEND))(address, len);
}

uint32_t usb_receive(uint32_t address, uint32_t max_size)
{
    return ((uint32_t(*)(uint32_t, uint32_t))readl(PTR_USB_RECEIVE))(address, max_size);
}

void usb_send(char *msg)
{
    raw_usb_send((uint32_t)msg, strlen(msg));
}
