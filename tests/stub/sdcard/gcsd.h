#pragma once
#include <fat.h>
extern bool stub_present_sd2, stub_present_sda;
static bool p_sd2(void){return stub_present_sd2;} static bool p_sda(void){return stub_present_sda;} static bool p_no(void){return false;} static bool ok(void){return true;}
static const DISC_INTERFACE __io_gcsd2 = { ok, p_sd2 };
static const DISC_INTERFACE __io_gcsda = { ok, p_sda };
static const DISC_INTERFACE __io_gcsdb = { ok, p_no };
