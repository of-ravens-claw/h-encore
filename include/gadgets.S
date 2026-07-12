/* gadgets.S -- rop gadgets
 *
 * Copyright (C) 2018 TheFloW, 2026 of-ravens-claw
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

// call
.equ blx_r0_add_sp_4_pop_pc,            0x8101e01f
.equ blx_r0_add_sp_14_pop_pc,           0x8101e0ff
.equ blx_r1_add_sp_4_pop_pc,            0x81006569
.equ blx_r2_add_sp_c_pop_pc,            0x810077f7
.equ blx_r3_add_sp_14_pop_pc,           0xCAFEBABE // missing
.equ blx_lr_add_sp_14_pop_pc,           0x81010045

// load
.equ pop_pc,                            0x810000d5
.equ pop_r0_r1_r2_r4_pc,                0xCAFEBABE // missing
.equ pop_r0_r1_r2_r3_r4_r5_r7_pc,       0xCAFEBABE // missing
.equ pop_r3_r5_pc,                      0xCAFEBABE // missing
.equ pop_r4_pc,                         0x81000cff
.equ pop_r4_r5_r6_r7_r8_sb_pc,          0x8100be5f
.equ ldm_sp_r0_r1_add_sp_c_pop_pc,      0x8101ae0d
.equ ldm_r0_r0_r2_ip_sp_lr_pc,          0xCAFEBABE // arm, missing
.equ ldm_r8_r0_r1_r4_r5_sl_ip_lr_pc,    0x8102bab4 // arm
.equ ldr_r0_sp_add_sp_4_pop_pc,         0x810009f7
.equ ldr_r0_r0_bx_lr,                   0x8101f0a1

// store
.equ str_r0_r1_add_sp_4_pop_pc,         0x81004fff

// move
.equ mov_r0_r4_blx_lr,                  0x81006769
.equ mov_r1_r4_blx_r3,                  0x81007ded
.equ movs_r1_r0_pop_r4_pc,              0x8101d4c3
.equ movs_r2_r4_add_sp_8_bx_lr,         0x8102238d
.equ movs_r4_r0_add_sp_c_pop_pc,        0x81000a6b

// arithmetic
.equ add_r0_r1_add_sp_10_bx_lr,         0x810228bb
.equ adcs_r0_lr_r5_lsl_12_pop_pc,       0xCAFEBABE // missing
.equ adds_r1_r1_r3_add_r0_r2_r1_bx_lr,  0xCAFEBABE // missing
.equ add_sp_14_pop_pc,                  0x81000b61
.equ subs_r1_r4_r1_blx_r3,              0x81021569
.equ muls_r0_r2_r0_subs_r3_r3_r0_bx_lr, 0xCAFEBABE // missing

// logical
.equ eors_r4_r0_movt_r0_8103_bx_lr,     0xCAFEBABE // missing

// compare
.equ cmp_eq_r0_r1_add_sp_8_bx_lr,       0x810168cd

// misc
.equ vdispEnd,                          0x81001c41
.equ vdispSetState,                     0x81001cfb
.equ vdispCtrl,                         0x8102dd58
.equ empty_string,                      0x81000002
.equ savedata0_system_dat_path,         0x810286c0
