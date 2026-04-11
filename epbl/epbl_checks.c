#include <stdint.h>
#include <stdbool.h>

#include <bootrom_funcs.h>

#include <epbl_info.h>
#include <exynos9830.h>
#include <memory.h>

bool is_head_marker_present()
{
    return readl(get_epbl_load_address() + 0x0008) == 0x68656164; // ASCII for "head"
}

uint32_t epbl_head_check(void)
{
    uint32_t ret = 0;

    if(is_head_marker_present())
    {
        uint32_t epbl_block_size = readl(get_epbl_load_address());

        if ((epbl_block_size > 2) &&
            (epbl_block_size <= (0x02071000 - (get_epbl_load_address() >> 9))))
            {
                set_epbl_size(epbl_block_size * BLOCK_SIZE);
                set_epbl_expected_hash(readl(get_epbl_load_address() + 0x0004));
                writel(0, get_epbl_load_address() + 0x0004); // Clear hash in binary
                ret = 1;
            }
    }

    return ret;
}

uint32_t check_epbl_hash(void)
{
    // Stub
    set_status_bit(1, EPBL_CHECKSUM_VALIDATION_SUCCESS);
    return 1;
}

uint32_t verify_epbl(void)
{
    // Stub
    set_status_bit(1, EPBL_VERIFICATION_SUCCESS);
    return 1;
}

uint32_t check_epbl_arb(void)
{
    // Stub
    set_status_bit(1, EPBL_ARB_VALIDATION_SUCCESS);
    return 1;
}

uint32_t verify_epbl_signature_and_rp_cnt(uint32_t secure_boot_status)
{
    uint32_t ret;

    if(!secure_boot_status)
    {
        ret = check_epbl_hash();
        if(!ret) return 0;

        store_rtc_time_difference(TIME_EPBL_HASH_VALIDATION);
    }
    else
    {
        ret = verify_epbl();
        if (!ret) return 0;

        ret = check_epbl_arb();
        if(!ret) return 0;

        store_rtc_time_difference(TIME_EPBL_VERFICATION);
        // Some other verification func gets called here, no point implementing, doesn't seem to set a status bit either
    }

    return ret;
}