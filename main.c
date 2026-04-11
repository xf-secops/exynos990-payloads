#include <stdint.h>
#include <stdbool.h>

#include <exynos9830.h>
#include <memory.h>
#include <bootrom_funcs.h>
#include <pmu.h>
#include <epbl_info.h>
#include <epbl_checks.h>
#include <bl1_info.h>

uint32_t load_epbl_usb(void)
{
	uint32_t ret;

	store_rtc_time_difference(TIME_EPBL_USB_LOAD_START);
	set_status_bit(1, EPBL_LOAD_START);
	usb_send("Ready to rx EPBL");

	ret = usb_receive(get_epbl_load_address(), 0x02071000 - get_epbl_load_address());
	if(ret & 0xFF)
	{
		usb_send("EPBL rx done");
		ret = epbl_head_check();
		if(!ret)
			usb_send("EPBL Header fail");
		else
			usb_send("EPBL Header pass");
	}
	else
	{
		usb_send("EPBL rx fail");
		ret = 0;
	}
	
	store_rtc_time_difference(TIME_EPBL_USB_LOAD_END);

	if(ret)
		set_status_bit(0, BL1_LOAD_EPBL_SUCCESS);

	return ret;
}

void jump_to_epbl(void)
{
    ((void(*)(void))get_epbl_load_address() + 0x0010)();
}

void setup_cryptocell_func_ptrs(void)
{
	uint32_t addr = (get_bl1_size() + 0x020216D0 + 0x0474);

	writel(0x16910, addr);
	writel(0x169A0, addr + 0x0004);
	writel(0x12BF0, addr + 0x0008);
	writel(0x167D8, addr + 0x000C);
	writel(0x16850, addr + 0x0010);
	writel(0x0ADC8, addr + 0x0014);
	writel(0x0AE04, addr + 0x0018);
	writel(0x0A984, addr + 0x001C);
}

void complete_bl1_rx_exec(void)
{
	uint32_t secureboot_key_address = readl(PTR_SECUREBOOT_KEY);
	uint32_t bl1_sign_type = 0;

	set_bl1_size(0x3000);
	set_bl1_checksum(0x7A1A1BEF);
	writel(0, 0x02022004); // Clear checksum, though for us it's already clear

	bl1_sign_type = readl(get_bl1_size() + 0x20216D0 + 0x14);

	// Write status bit before checksum validation
	writel(readl(IRAM_STATUS_REG0) | BL1_CHECKSUM_VALIDATION_START, IRAM_STATUS_REG0);

	// Write status bit upon succesful check of BL1 against checksum
	writel(readl(IRAM_STATUS_REG0) | BL1_CHECKSUM_VALIDATION_SUCCESS, IRAM_STATUS_REG0);

	store_rtc_time_difference(TIME_BL1_CHECKSUM_VALIDATION);
	writel(secureboot_key_address | bl1_sign_type, IRAM_SBC_INFORMATION);

	writel(readl(IRAM_STATUS_REG0) | BL1_VERIFICATION_SUCCESS, IRAM_STATUS_REG0);

	store_rtc_time_difference(TIME_BL1_VERIFICATION);

	setup_cryptocell_func_ptrs();
	store_rtc_time_difference(TIME_BL1_START);
	set_status_bit(0, BL1_START);
}

void detect_and_patch_decrypted_epbl(void)
{
	if(readl(get_epbl_load_address() + 0x002C) == 0xD503201F)
	{
		uint32_t ebre_marker_loc = 0;

		usb_send("OpenMiniBL1 - Decrypted EPBL detected, patching decryption status flags");
		usb_send("OpenMiniBL1 - Searching for EBRE marker...");

		ebre_marker_loc = SEARCH_PATTERN_ARM(get_epbl_load_address(),
											 get_epbl_load_address() + get_epbl_size(),
											 0x45524245, 0);

		if(ebre_marker_loc)
		{
			usb_send("OpenMiniBL1 - Found EBRE marker, setting decryption flags");
			writel(0x53554343, ebre_marker_loc - 0x0008);
			writel(readl(ebre_marker_loc + 0x0008) & 0xFFFF0000, ebre_marker_loc + 0x0008);
			usb_send("OpenMiniBL1 - Decryption flags set.");
		}
		else
		{
			usb_send("OpenMiniBL1 - Failed to find EBRE marker, continuing boot, if flags aren't set already, expect failed boot.");
		}
	}
}

/* Not needed for public use
uint8_t usb_receive_hook(uint32_t rx_addr, uint32_t size)
{
	if(rx_addr == 0xBFE80000)
	{
		writel(0x6368, PTR_USB_RECEIVE);
		usb_receive(0xE808C5f0, 0x3000); // Cocoa
		usb_receive(0x90000000, 404123); // BMP of Cocoa
		uint8_t ret = usb_receive(rx_addr, size);
		return ret;
	}

	writel(0x6368, PTR_USB_RECEIVE);
	uint8_t ret = usb_receive(rx_addr, size);
	writel((uint32_t)(uintptr_t)usb_receive_hook, PTR_USB_RECEIVE);
	return ret;
}*/

int main(void)
{
	uint32_t ret = 0;

	usb_reinit(USB_STRUCT_ADDR, USB_DELAY, USB_SPEED_FULLSPEED);
	usb_send("Setting up last bits of BL1...");
	complete_bl1_rx_exec();
	usb_send("OpenMiniBL1 - Started");

	set_ps_hold();
	set_epbl_load_address();
	ret = load_epbl_usb();
	if(!ret)
		while(1);

	ret = verify_epbl_signature_and_rp_cnt(is_secure_boot());
	if(!ret)
		while(1);

	detect_and_patch_decrypted_epbl();

	set_status_bit(0, BL1_END);
	usb_send("OpenMiniBL1 - Attempting last minute patches...\n");
	//writel((uint32_t)(uintptr_t)usb_receive_hook, PTR_USB_RECEIVE);
	usb_send("OpenMiniBL1 - Bye!");
	jump_to_epbl();
}
