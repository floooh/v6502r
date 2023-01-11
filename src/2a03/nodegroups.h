#pragma once
#include <stdint.h>
#include "nodenames.h"

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

static uint32_t nodegroup_ab[16] = { p2a03_ab0, p2a03_ab1, p2a03_ab2, p2a03_ab3, p2a03_ab4, p2a03_ab5, p2a03_ab6, p2a03_ab7, p2a03_ab8, p2a03_ab9, p2a03_ab10, p2a03_ab11, p2a03_ab12, p2a03_ab13, p2a03_ab14, p2a03_ab15 };
static uint32_t nodegroup_db[8]  = { p2a03_db0, p2a03_db1, p2a03_db2, p2a03_db3, p2a03_db4, p2a03_db5, p2a03_db6, p2a03_db7 };
static uint32_t nodegroup_pcl[8] = { p2a03_pcl0,p2a03_pcl1,p2a03_pcl2,p2a03_pcl3,p2a03_pcl4,p2a03_pcl5,p2a03_pcl6,p2a03_pcl7 };
static uint32_t nodegroup_pch[8] = { p2a03_pch0,p2a03_pch1,p2a03_pch2,p2a03_pch3,p2a03_pch4,p2a03_pch5,p2a03_pch6,p2a03_pch7 };
static uint32_t nodegroup_p[8]   = { p2a03_p0,p2a03_p1,p2a03_p2,p2a03_p3,p2a03_p4,0,p2a03_p6,p2a03_p7 }; // missing p2a03_p5 is not a typo (there is no physical bit 5 in status reg)
static uint32_t nodegroup_a[8]   = { p2a03_a0,p2a03_a1,p2a03_a2,p2a03_a3,p2a03_a4,p2a03_a5,p2a03_a6,p2a03_a7 };
static uint32_t nodegroup_x[8]   = { p2a03_x0,p2a03_x1,p2a03_x2,p2a03_x3,p2a03_x4,p2a03_x5,p2a03_x6,p2a03_x7 };
static uint32_t nodegroup_y[8]   = { p2a03_y0,p2a03_y1,p2a03_y2,p2a03_y3,p2a03_y4,p2a03_y5,p2a03_y6,p2a03_y7 };
static uint32_t nodegroup_sp[8]  = { p2a03_s0,p2a03_s1,p2a03_s2,p2a03_s3,p2a03_s4,p2a03_s5,p2a03_s6,p2a03_s7 };
static uint32_t nodegroup_ir[8]  = { p2a03_notir0,p2a03_notir1,p2a03_notir2,p2a03_notir3,p2a03_notir4,p2a03_notir5,p2a03_notir6,p2a03_notir7 };

//
static uint32_t nodegroup_sq0[4] = {p2a03_sq0_out0, p2a03_sq0_out1, p2a03_sq0_out2, p2a03_sq0_out3};
static uint32_t nodegroup_sq1[4] = {p2a03_sq1_out0, p2a03_sq1_out1, p2a03_sq1_out2, p2a03_sq1_out3};
static uint32_t nodegroup_tri[4] = {p2a03_tri_out0, p2a03_tri_out1, p2a03_tri_out2, p2a03_tri_out3};
static uint32_t nodegroup_noi[4] = {p2a03_noi_out0, p2a03_noi_out1, p2a03_noi_out2, p2a03_noi_out3};
static uint32_t nodegroup_pcm[7] = {p2a03_pcm_out0, p2a03_pcm_out1, p2a03_pcm_out2, p2a03_pcm_out3, p2a03_pcm_out4, p2a03_pcm_out5, p2a03_pcm_out6 };


#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
