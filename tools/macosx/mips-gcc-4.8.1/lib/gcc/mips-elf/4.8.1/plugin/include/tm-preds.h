/* Generated automatically by the program 'build/genpreds'
   from the machine description file '../../gcc-4.8-2013.11/gcc/config/mips/mips.md'.  */

#ifndef GCC_TM_PREDS_H
#define GCC_TM_PREDS_H

#ifdef HAVE_MACHINE_MODES
extern int general_operand (rtx, enum machine_mode);
extern int address_operand (rtx, enum machine_mode);
extern int register_operand (rtx, enum machine_mode);
extern int pmode_register_operand (rtx, enum machine_mode);
extern int scratch_operand (rtx, enum machine_mode);
extern int immediate_operand (rtx, enum machine_mode);
extern int const_int_operand (rtx, enum machine_mode);
extern int const_double_operand (rtx, enum machine_mode);
extern int nonimmediate_operand (rtx, enum machine_mode);
extern int nonmemory_operand (rtx, enum machine_mode);
extern int push_operand (rtx, enum machine_mode);
extern int pop_operand (rtx, enum machine_mode);
extern int memory_operand (rtx, enum machine_mode);
extern int indirect_operand (rtx, enum machine_mode);
extern int ordered_comparison_operator (rtx, enum machine_mode);
extern int comparison_operator (rtx, enum machine_mode);
extern int const_uns_arith_operand (rtx, enum machine_mode);
extern int uns_arith_operand (rtx, enum machine_mode);
extern int const_arith_operand (rtx, enum machine_mode);
extern int arith_operand (rtx, enum machine_mode);
extern int const_uimm6_operand (rtx, enum machine_mode);
extern int const_imm10_operand (rtx, enum machine_mode);
extern int reg_imm10_operand (rtx, enum machine_mode);
extern int sle_operand (rtx, enum machine_mode);
extern int sleu_operand (rtx, enum machine_mode);
extern int const_0_operand (rtx, enum machine_mode);
extern int reg_or_0_operand (rtx, enum machine_mode);
extern int const_1_operand (rtx, enum machine_mode);
extern int reg_or_1_operand (rtx, enum machine_mode);
extern int const_0_or_1_operand (rtx, enum machine_mode);
extern int const_2_or_3_operand (rtx, enum machine_mode);
extern int const_0_to_3_operand (rtx, enum machine_mode);
extern int qi_mask_operand (rtx, enum machine_mode);
extern int hi_mask_operand (rtx, enum machine_mode);
extern int si_mask_operand (rtx, enum machine_mode);
extern int and_load_operand (rtx, enum machine_mode);
extern int low_bitmask_operand (rtx, enum machine_mode);
extern int and_reg_operand (rtx, enum machine_mode);
extern int and_operand (rtx, enum machine_mode);
extern int d_operand (rtx, enum machine_mode);
extern int lwsp_swsp_operand (rtx, enum machine_mode);
extern int lw16_sw16_operand (rtx, enum machine_mode);
extern int lhu16_sh16_operand (rtx, enum machine_mode);
extern int lbu16_operand (rtx, enum machine_mode);
extern int sb16_operand (rtx, enum machine_mode);
extern int db4_operand (rtx, enum machine_mode);
extern int db7_operand (rtx, enum machine_mode);
extern int db8_operand (rtx, enum machine_mode);
extern int ib3_operand (rtx, enum machine_mode);
extern int sb4_operand (rtx, enum machine_mode);
extern int sb5_operand (rtx, enum machine_mode);
extern int sb8_operand (rtx, enum machine_mode);
extern int sd8_operand (rtx, enum machine_mode);
extern int ub4_operand (rtx, enum machine_mode);
extern int ub8_operand (rtx, enum machine_mode);
extern int uh4_operand (rtx, enum machine_mode);
extern int uw4_operand (rtx, enum machine_mode);
extern int uw5_operand (rtx, enum machine_mode);
extern int uw6_operand (rtx, enum machine_mode);
extern int uw8_operand (rtx, enum machine_mode);
extern int addiur2_operand (rtx, enum machine_mode);
extern int addiusp_operand (rtx, enum machine_mode);
extern int andi16_operand (rtx, enum machine_mode);
extern int movep_src_register (rtx, enum machine_mode);
extern int movep_src_operand (rtx, enum machine_mode);
extern int lo_operand (rtx, enum machine_mode);
extern int hilo_operand (rtx, enum machine_mode);
extern int fcc_reload_operand (rtx, enum machine_mode);
extern int muldiv_target_operand (rtx, enum machine_mode);
extern int const_call_insn_operand (rtx, enum machine_mode);
extern int call_insn_operand (rtx, enum machine_mode);
extern int splittable_const_int_operand (rtx, enum machine_mode);
extern int move_operand (rtx, enum machine_mode);
extern int cprestore_save_slot_operand (rtx, enum machine_mode);
extern int cprestore_load_slot_operand (rtx, enum machine_mode);
extern int consttable_operand (rtx, enum machine_mode);
extern int symbolic_operand (rtx, enum machine_mode);
extern int absolute_symbolic_operand (rtx, enum machine_mode);
extern int symbolic_operand_with_high (rtx, enum machine_mode);
extern int force_to_mem_operand (rtx, enum machine_mode);
extern int got_disp_operand (rtx, enum machine_mode);
extern int got_page_ofst_operand (rtx, enum machine_mode);
extern int tls_reloc_operand (rtx, enum machine_mode);
extern int symbol_ref_operand (rtx, enum machine_mode);
extern int stack_operand (rtx, enum machine_mode);
extern int macc_msac_operand (rtx, enum machine_mode);
extern int equality_operator (rtx, enum machine_mode);
extern int extend_operator (rtx, enum machine_mode);
extern int trap_comparison_operator (rtx, enum machine_mode);
extern int order_operator (rtx, enum machine_mode);
extern int mips_cstore_operator (rtx, enum machine_mode);
extern int small_data_pattern (rtx, enum machine_mode);
extern int mem_noofs_operand (rtx, enum machine_mode);
extern int non_volatile_mem_operand (rtx, enum machine_mode);
#endif /* HAVE_MACHINE_MODES */

#define CONSTRAINT_NUM_DEFINED_P 1
enum constraint_num
{
  CONSTRAINT__UNKNOWN = 0,
  CONSTRAINT_d,
  CONSTRAINT_t,
  CONSTRAINT_f,
  CONSTRAINT_h,
  CONSTRAINT_l,
  CONSTRAINT_x,
  CONSTRAINT_b,
  CONSTRAINT_u,
  CONSTRAINT_c,
  CONSTRAINT_e,
  CONSTRAINT_j,
  CONSTRAINT_v,
  CONSTRAINT_y,
  CONSTRAINT_z,
  CONSTRAINT_A,
  CONSTRAINT_a,
  CONSTRAINT_B,
  CONSTRAINT_C,
  CONSTRAINT_D,
  CONSTRAINT_ka,
  CONSTRAINT_kf,
  CONSTRAINT_ks,
  CONSTRAINT_I,
  CONSTRAINT_J,
  CONSTRAINT_K,
  CONSTRAINT_L,
  CONSTRAINT_M,
  CONSTRAINT_N,
  CONSTRAINT_O,
  CONSTRAINT_P,
  CONSTRAINT_G,
  CONSTRAINT_Q,
  CONSTRAINT_R,
  CONSTRAINT_S,
  CONSTRAINT_Udb7,
  CONSTRAINT_Udb8,
  CONSTRAINT_Uead,
  CONSTRAINT_Uean,
  CONSTRAINT_Uesp,
  CONSTRAINT_Uib3,
  CONSTRAINT_Usb4,
  CONSTRAINT_Usb5,
  CONSTRAINT_Usb8,
  CONSTRAINT_Usd8,
  CONSTRAINT_Uub8,
  CONSTRAINT_Uuw5,
  CONSTRAINT_Uuw6,
  CONSTRAINT_Uuw8,
  CONSTRAINT_W,
  CONSTRAINT_YG,
  CONSTRAINT_YA,
  CONSTRAINT_YB,
  CONSTRAINT_Yb,
  CONSTRAINT_Yd,
  CONSTRAINT_Yf,
  CONSTRAINT_Yh,
  CONSTRAINT_Yw,
  CONSTRAINT_Yx,
  CONSTRAINT_ZC,
  CONSTRAINT_ZD,
  CONSTRAINT_ZR,
  CONSTRAINT_ZS,
  CONSTRAINT_ZT,
  CONSTRAINT_ZU,
  CONSTRAINT_ZV,
  CONSTRAINT_ZW,
  CONSTRAINT__LIMIT
};

extern enum constraint_num lookup_constraint (const char *);
extern bool constraint_satisfied_p (rtx, enum constraint_num);

static inline size_t
insn_constraint_len (char fc, const char *str ATTRIBUTE_UNUSED)
{
  switch (fc)
    {
    case 'U': return 4;
    case 'Y': return 2;
    case 'Z': return 2;
    case 'k': return 2;
    default: break;
    }
  return 1;
}

#define CONSTRAINT_LEN(c_,s_) insn_constraint_len (c_,s_)

extern enum reg_class regclass_for_constraint (enum constraint_num);
#define REG_CLASS_FROM_CONSTRAINT(c_,s_) \
    regclass_for_constraint (lookup_constraint (s_))
#define REG_CLASS_FOR_CONSTRAINT(x_) \
    regclass_for_constraint (x_)

extern bool insn_const_int_ok_for_constraint (HOST_WIDE_INT, enum constraint_num);
#define CONST_OK_FOR_CONSTRAINT_P(v_,c_,s_) \
    insn_const_int_ok_for_constraint (v_, lookup_constraint (s_))

#define CONST_DOUBLE_OK_FOR_CONSTRAINT_P(v_,c_,s_) \
    constraint_satisfied_p (v_, lookup_constraint (s_))

#define EXTRA_CONSTRAINT_STR(v_,c_,s_) \
    constraint_satisfied_p (v_, lookup_constraint (s_))

extern bool insn_extra_memory_constraint (enum constraint_num);
#define EXTRA_MEMORY_CONSTRAINT(c_,s_) insn_extra_memory_constraint (lookup_constraint (s_))

extern bool insn_extra_address_constraint (enum constraint_num);
#define EXTRA_ADDRESS_CONSTRAINT(c_,s_) insn_extra_address_constraint (lookup_constraint (s_))

#endif /* tm-preds.h */
