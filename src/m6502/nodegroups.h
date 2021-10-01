#pragma once
#include <stdint.h>
#include "nodenames.h"

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

static uint32_t nodegroup_ab[16] = { p6502_ab0, p6502_ab1, p6502_ab2, p6502_ab3, p6502_ab4, p6502_ab5, p6502_ab6, p6502_ab7, p6502_ab8, p6502_ab9, p6502_ab10, p6502_ab11, p6502_ab12, p6502_ab13, p6502_ab14, p6502_ab15 };
static uint32_t nodegroup_db[8]  = { p6502_db0, p6502_db1, p6502_db2, p6502_db3, p6502_db4, p6502_db5, p6502_db6, p6502_db7 };
static uint32_t nodegroup_pcl[8] = { p6502_pcl0,p6502_pcl1,p6502_pcl2,p6502_pcl3,p6502_pcl4,p6502_pcl5,p6502_pcl6,p6502_pcl7 };
static uint32_t nodegroup_pch[8] = { p6502_pch0,p6502_pch1,p6502_pch2,p6502_pch3,p6502_pch4,p6502_pch5,p6502_pch6,p6502_pch7 };
static uint32_t nodegroup_p[8]   = { p6502_p0,p6502_p1,p6502_p2,p6502_p3,p6502_p4,0,p6502_p6,p6502_p7 }; // missing p6502_p5 is not a typo (there is no physical bit 5 in status reg)
static uint32_t nodegroup_a[8]   = { p6502_a0,p6502_a1,p6502_a2,p6502_a3,p6502_a4,p6502_a5,p6502_a6,p6502_a7 };
static uint32_t nodegroup_x[8]   = { p6502_x0,p6502_x1,p6502_x2,p6502_x3,p6502_x4,p6502_x5,p6502_x6,p6502_x7 };
static uint32_t nodegroup_y[8]   = { p6502_y0,p6502_y1,p6502_y2,p6502_y3,p6502_y4,p6502_y5,p6502_y6,p6502_y7 };
static uint32_t nodegroup_sp[8]  = { p6502_s0,p6502_s1,p6502_s2,p6502_s3,p6502_s4,p6502_s5,p6502_s6,p6502_s7 };
static uint32_t nodegroup_ir[8]  = { p6502_notir0,p6502_notir1,p6502_notir2,p6502_notir3,p6502_notir4,p6502_notir5,p6502_notir6,p6502_notir7 };

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif