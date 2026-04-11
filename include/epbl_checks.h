#pragma once
#include <stdint.h>

uint32_t epbl_head_check(void);
uint32_t verify_epbl_signature_and_rp_cnt(uint32_t secureboot_status);