#pragma once
#include <stdint.h>
#include "nodenames.h"

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

static uint32_t nodegroup_ab[16] = { pz80_ab0,pz80_ab1,pz80_ab2,pz80_ab3,pz80_ab4,pz80_ab5,pz80_ab6,pz80_ab7,pz80_ab8,pz80_ab9,pz80_ab10,pz80_ab11,pz80_ab12,pz80_ab13,pz80_ab14,pz80_ab15 };
static uint32_t nodegroup_db[8]  = { pz80_db0,pz80_db1,pz80_db2,pz80_db3,pz80_db4,pz80_db5,pz80_db6,pz80_db7 };
static uint32_t nodegroup_pcl[8] = { pz80_reg_pcl0,pz80_reg_pcl1,pz80_reg_pcl2,pz80_reg_pcl3,pz80_reg_pcl4,pz80_reg_pcl5,pz80_reg_pcl6,pz80_reg_pcl7 };
static uint32_t nodegroup_pch[8] = { pz80_reg_pch0,pz80_reg_pch1,pz80_reg_pch2,pz80_reg_pch3,pz80_reg_pch4,pz80_reg_pch5,pz80_reg_pch6,pz80_reg_pch7 };
static uint32_t nodegroup_ir[8]  = { pz80_instr0,pz80_instr1,pz80_instr2,pz80_instr3,pz80_instr4,pz80_instr5,pz80_instr6,pz80_instr7 };
static uint32_t nodegroup_reg_ff[8] = { pz80_reg_ff0, pz80_reg_ff1, pz80_reg_ff2, pz80_reg_ff3, pz80_reg_ff4, pz80_reg_ff5, pz80_reg_ff6, pz80_reg_ff7 };
static uint32_t nodegroup_reg_aa[8] = { pz80_reg_aa0, pz80_reg_aa1, pz80_reg_aa2, pz80_reg_aa3, pz80_reg_aa4, pz80_reg_aa5, pz80_reg_aa6, pz80_reg_aa7 };
static uint32_t nodegroup_reg_bb[8] = { pz80_reg_bb0, pz80_reg_bb1, pz80_reg_bb2, pz80_reg_bb3, pz80_reg_bb4, pz80_reg_bb5, pz80_reg_bb6, pz80_reg_bb7 };
static uint32_t nodegroup_reg_cc[8] = { pz80_reg_cc0, pz80_reg_cc1, pz80_reg_cc2, pz80_reg_cc3, pz80_reg_cc4, pz80_reg_cc5, pz80_reg_cc6, pz80_reg_cc7 };
static uint32_t nodegroup_reg_dd[8] = { pz80_reg_dd0, pz80_reg_dd1, pz80_reg_dd2, pz80_reg_dd3, pz80_reg_dd4, pz80_reg_dd5, pz80_reg_dd6, pz80_reg_dd7 };
static uint32_t nodegroup_reg_ee[8] = { pz80_reg_ee0, pz80_reg_ee1, pz80_reg_ee2, pz80_reg_ee3, pz80_reg_ee4, pz80_reg_ee5, pz80_reg_ee6, pz80_reg_ee7 };
static uint32_t nodegroup_reg_hh[8] = { pz80_reg_hh0, pz80_reg_hh1, pz80_reg_hh2, pz80_reg_hh3, pz80_reg_hh4, pz80_reg_hh5, pz80_reg_hh6, pz80_reg_hh7 };
static uint32_t nodegroup_reg_ll[8] = { pz80_reg_ll0, pz80_reg_ll1, pz80_reg_ll2, pz80_reg_ll3, pz80_reg_ll4, pz80_reg_ll5, pz80_reg_ll6, pz80_reg_ll7 };
static uint32_t nodegroup_reg_f[8]  = { pz80_reg_f0, pz80_reg_f1, pz80_reg_f2, pz80_reg_f3, pz80_reg_f4, pz80_reg_f5, pz80_reg_f6, pz80_reg_f7 };
static uint32_t nodegroup_reg_a[8] = { pz80_reg_a0, pz80_reg_a1, pz80_reg_a2, pz80_reg_a3, pz80_reg_a4, pz80_reg_a5, pz80_reg_a6, pz80_reg_a7 };
static uint32_t nodegroup_reg_b[8] = { pz80_reg_b0, pz80_reg_b1, pz80_reg_b2, pz80_reg_b3, pz80_reg_b4, pz80_reg_b5, pz80_reg_b6, pz80_reg_b7 };
static uint32_t nodegroup_reg_c[8] = { pz80_reg_c0, pz80_reg_c1, pz80_reg_c2, pz80_reg_c3, pz80_reg_c4, pz80_reg_c5, pz80_reg_c6, pz80_reg_c7 };
static uint32_t nodegroup_reg_d[8] = { pz80_reg_d0, pz80_reg_d1, pz80_reg_d2, pz80_reg_d3, pz80_reg_d4, pz80_reg_d5, pz80_reg_d6, pz80_reg_d7 };
static uint32_t nodegroup_reg_e[8] = { pz80_reg_e0, pz80_reg_e1, pz80_reg_e2, pz80_reg_e3, pz80_reg_e4, pz80_reg_e5, pz80_reg_e6, pz80_reg_e7 };
static uint32_t nodegroup_reg_h[8] = { pz80_reg_h0, pz80_reg_h1, pz80_reg_h2, pz80_reg_h3, pz80_reg_h4, pz80_reg_h5, pz80_reg_h6, pz80_reg_h7 };
static uint32_t nodegroup_reg_l[8] = { pz80_reg_l0, pz80_reg_l1, pz80_reg_l2, pz80_reg_l3, pz80_reg_l4, pz80_reg_l5, pz80_reg_l6, pz80_reg_l7 };
static uint32_t nodegroup_reg_i[8] = { pz80_reg_i0, pz80_reg_i1, pz80_reg_i2, pz80_reg_i3, pz80_reg_i4, pz80_reg_i5, pz80_reg_i6, pz80_reg_i7 };
static uint32_t nodegroup_reg_r[8] = { pz80_reg_r0, pz80_reg_r1, pz80_reg_r2, pz80_reg_r3, pz80_reg_r4, pz80_reg_r5, pz80_reg_r6, pz80_reg_r7 };
static uint32_t nodegroup_reg_w[8] = { pz80_reg_w0, pz80_reg_w1, pz80_reg_w2, pz80_reg_w3, pz80_reg_w4, pz80_reg_w5, pz80_reg_w6, pz80_reg_w7 };
static uint32_t nodegroup_reg_z[8] = { pz80_reg_z0, pz80_reg_z1, pz80_reg_z2, pz80_reg_z3, pz80_reg_z4, pz80_reg_z5, pz80_reg_z6, pz80_reg_z7 };
static uint32_t nodegroup_reg_ixh[8] = { pz80_reg_ixh0, pz80_reg_ixh1, pz80_reg_ixh2, pz80_reg_ixh3, pz80_reg_ixh4, pz80_reg_ixh5, pz80_reg_ixh6, pz80_reg_ixh7 };
static uint32_t nodegroup_reg_ixl[8] = { pz80_reg_ixl0, pz80_reg_ixl1, pz80_reg_ixl2, pz80_reg_ixl3, pz80_reg_ixl4, pz80_reg_ixl5, pz80_reg_ixl6, pz80_reg_ixl7 };
static uint32_t nodegroup_reg_iyh[8] = { pz80_reg_iyh0, pz80_reg_iyh1, pz80_reg_iyh2, pz80_reg_iyh3, pz80_reg_iyh4, pz80_reg_iyh5, pz80_reg_iyh6, pz80_reg_iyh7 };
static uint32_t nodegroup_reg_iyl[8] = { pz80_reg_iyl0, pz80_reg_iyl1, pz80_reg_iyl2, pz80_reg_iyl3, pz80_reg_iyl4, pz80_reg_iyl5, pz80_reg_iyl6, pz80_reg_iyl7 };
static uint32_t nodegroup_reg_sph[8] = { pz80_reg_sph0, pz80_reg_sph1, pz80_reg_sph2, pz80_reg_sph3, pz80_reg_sph4, pz80_reg_sph5, pz80_reg_sph6, pz80_reg_sph7 };
static uint32_t nodegroup_reg_spl[8] = { pz80_reg_spl0, pz80_reg_spl1, pz80_reg_spl2, pz80_reg_spl3, pz80_reg_spl4, pz80_reg_spl5, pz80_reg_spl6, pz80_reg_spl7 };
static uint32_t nodegroup_regbits[16] = { pz80_regbit0, pz80_regbit1, pz80_regbit2, pz80_regbit3, pz80_regbit4, pz80_regbit5, pz80_regbit6, pz80_regbit7, pz80_regbit8, pz80_regbit9, pz80_regbit10, pz80_regbit11, pz80_regbit12, pz80_regbit13, pz80_regbit14, pz80_regbit15 };
static uint32_t nodegroup_pcbits[16] = { pz80_pcbit0, pz80_pcbit1, pz80_pcbit2, pz80_pcbit3, pz80_pcbit4, pz80_pcbit5, pz80_pcbit6, pz80_pcbit7, pz80_pcbit8, pz80_pcbit9, pz80_pcbit10, pz80_pcbit11, pz80_pcbit12, pz80_pcbit13, pz80_pcbit14, pz80_pcbit15 };
static uint32_t nodegroup_ubus[8] = { pz80_ubus0, pz80_ubus1, pz80_ubus2, pz80_ubus3, pz80_ubus4, pz80_ubus5,pz80_ubus6, pz80_ubus7 };
static uint32_t nodegroup_vbus[8] = { pz80_vbus0, pz80_vbus1, pz80_vbus2, pz80_vbus3, pz80_vbus4, pz80_vbus5,pz80_vbus6, pz80_vbus7 };
static uint32_t nodegroup_alua[8] = { pz80_alua0, pz80_alua1, pz80_alua2, pz80_alua3, pz80_alua4, pz80_alua5, pz80_alua6, pz80_alua7 };
static uint32_t nodegroup_alub[8] = { pz80_alub0, pz80_alub1, pz80_alub2, pz80_alub3, pz80_alub4, pz80_alub5, pz80_alub6, pz80_alub7 };
static uint32_t nodegroup_alubus[8] = { pz80_alubus0, pz80_alubus1, pz80_alubus2, pz80_alubus3, pz80_alubus4, pz80_alubus5, pz80_alubus6, pz80_alubus7 };
static uint32_t nodegroup_alulat[4] = { pz80_alulat0, pz80_alulat1, pz80_alulat2, pz80_alulat3 };
static uint32_t nodegroup_aluout[4] = { pz80_aluout0, pz80_aluout1, pz80_aluout2, pz80_aluout3 };
static uint32_t nodegroup_dbus[8] = { pz80_dbus0, pz80_dbus1, pz80_dbus2, pz80_dbus3, pz80_dbus4, pz80_dbus5, pz80_dbus6, pz80_dbus7 };
static uint32_t nodegroup_dlatch[8] = { pz80_dlatch0, pz80_dlatch1, pz80_dlatch2, pz80_dlatch3, pz80_dlatch4, pz80_dlatch5, pz80_dlatch6, pz80_dlatch7 };
static uint32_t nodegroup_m[5] = { pz80_m1, pz80_m2, pz80_m3, pz80_m4, pz80_m5 };
static uint32_t nodegroup_t[6] = { pz80_t1, pz80_t2, pz80_t3, pz80_t4, pz80_t5, pz80_t6 };
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
