/* Generated automatically by the program `genflags'
   from the machine description file `md'.  */

#ifndef GCC_INSN_FLAGS_H
#define GCC_INSN_FLAGS_H

#define HAVE_addsi3 1
#define HAVE_addsf3 1
#define HAVE_subsi3 1
#define HAVE_subsf3 1
#define HAVE_mulsi3_highpart 1
#define HAVE_umulsi3_highpart 1
#define HAVE_mulsi3 1
#define HAVE_mulhisi3 1
#define HAVE_umulhisi3 1
#define HAVE_muladdhisi 1
#define HAVE_mulsubhisi 1
#define HAVE_mulsf3 1
#define HAVE_fmasf4 1
#define HAVE_fnmasf4 1
#define HAVE_divsi3 1
#define HAVE_udivsi3 1
#define HAVE_modsi3 1
#define HAVE_umodsi3 1
#define HAVE_abssi2 1
#define HAVE_abssf2 1
#define HAVE_sminsi3 1
#define HAVE_uminsi3 1
#define HAVE_smaxsi3 1
#define HAVE_umaxsi3 1
#define HAVE_clzsi2 1
#define HAVE_negsi2 1
#define HAVE_negsf2 1
#define HAVE_andsi3 1
#define HAVE_iorsi3 1
#define HAVE_xorsi3 1
#define HAVE_zero_extendhisi2 1
#define HAVE_zero_extendqisi2 1
#define HAVE_extendhisi2_internal 1
#define HAVE_extendqisi2_internal 1
#define HAVE_extv_internal 1
#define HAVE_extzv_internal 1
#define HAVE_fix_truncsfsi2 1
#define HAVE_fixuns_truncsfsi2 1
#define HAVE_floatsisf2 1
#define HAVE_floatunssisf2 1
#define HAVE_movdi_internal (register_operand (operands[0], DImode) \
   || register_operand (operands[1], DImode))
#define HAVE_movsi_internal (xtensa_valid_move (SImode, operands))
#define HAVE_movhi_internal (xtensa_valid_move (HImode, operands))
#define HAVE_movqi_internal (xtensa_valid_move (QImode, operands))
#define HAVE_movsf_internal (((register_operand (operands[0], SFmode) \
     || register_operand (operands[1], SFmode)) \
    && !(FP_REG_P (xt_true_regnum (operands[0])) \
         && (constantpool_mem_p (operands[1]) || CONSTANT_P (operands[1])))))
#define HAVE_movdf_internal (register_operand (operands[0], DFmode) \
   || register_operand (operands[1], DFmode))
#define HAVE_ashlsi3_internal 1
#define HAVE_ashrsi3 1
#define HAVE_lshrsi3 1
#define HAVE_rotlsi3 1
#define HAVE_rotrsi3 1
#define HAVE_zero_cost_loop_start 1
#define HAVE_zero_cost_loop_end 1
#define HAVE_movsicc_internal0 1
#define HAVE_movsicc_internal1 1
#define HAVE_movsfcc_internal0 1
#define HAVE_movsfcc_internal1 1
#define HAVE_seq_sf 1
#define HAVE_slt_sf 1
#define HAVE_sle_sf 1
#define HAVE_suneq_sf 1
#define HAVE_sunlt_sf 1
#define HAVE_sunle_sf 1
#define HAVE_sunordered_sf 1
#define HAVE_jump 1
#define HAVE_indirect_jump_internal 1
#define HAVE_tablejump_internal 1
#define HAVE_call_internal 1
#define HAVE_call_value_internal 1
#define HAVE_entry 1
#define HAVE_return ((TARGET_WINDOWED_ABI || !xtensa_current_frame_size) && reload_completed)
#define HAVE_nop 1
#define HAVE_eh_set_a0_windowed 1
#define HAVE_eh_set_a0_call0 1
#define HAVE_blockage 1
#define HAVE_set_frame_ptr 1
#define HAVE_get_thread_pointersi 1
#define HAVE_set_thread_pointersi 1
#define HAVE_tls_func 1
#define HAVE_tls_arg 1
#define HAVE_tls_call 1
#define HAVE_sync_lock_releasesi 1
#define HAVE_sync_compare_and_swapsi 1
#define HAVE_mulsidi3 1
#define HAVE_umulsidi3 1
#define HAVE_ctzsi2 1
#define HAVE_ffssi2 1
#define HAVE_one_cmplsi2 1
#define HAVE_extendhisi2 1
#define HAVE_extendqisi2 1
#define HAVE_extv 1
#define HAVE_extzv 1
#define HAVE_movdi 1
#define HAVE_movsi 1
#define HAVE_movhi 1
#define HAVE_movqi 1
#define HAVE_reloadhi_literal 1
#define HAVE_reloadqi_literal 1
#define HAVE_movsf 1
#define HAVE_movdf 1
#define HAVE_movmemsi 1
#define HAVE_ashlsi3 1
#define HAVE_cbranchsi4 1
#define HAVE_cbranchsf4 1
#define HAVE_cstoresi4 1
#define HAVE_cstoresf4 1
#define HAVE_movsicc 1
#define HAVE_movsfcc 1
#define HAVE_indirect_jump 1
#define HAVE_tablejump 1
#define HAVE_sym_PLT 1
#define HAVE_call 1
#define HAVE_call_value 1
#define HAVE_prologue 1
#define HAVE_epilogue 1
#define HAVE_nonlocal_goto 1
#define HAVE_eh_return 1
#define HAVE_sym_TPOFF 1
#define HAVE_sym_DTPOFF 1
#define HAVE_memory_barrier 1
#define HAVE_sync_compare_and_swaphi 1
#define HAVE_sync_compare_and_swapqi 1
#define HAVE_sync_lock_test_and_sethi 1
#define HAVE_sync_lock_test_and_setqi 1
#define HAVE_sync_andhi 1
#define HAVE_sync_iorhi 1
#define HAVE_sync_xorhi 1
#define HAVE_sync_addhi 1
#define HAVE_sync_subhi 1
#define HAVE_sync_nandhi 1
#define HAVE_sync_andqi 1
#define HAVE_sync_iorqi 1
#define HAVE_sync_xorqi 1
#define HAVE_sync_addqi 1
#define HAVE_sync_subqi 1
#define HAVE_sync_nandqi 1
#define HAVE_sync_old_andhi 1
#define HAVE_sync_old_iorhi 1
#define HAVE_sync_old_xorhi 1
#define HAVE_sync_old_addhi 1
#define HAVE_sync_old_subhi 1
#define HAVE_sync_old_nandhi 1
#define HAVE_sync_old_andqi 1
#define HAVE_sync_old_iorqi 1
#define HAVE_sync_old_xorqi 1
#define HAVE_sync_old_addqi 1
#define HAVE_sync_old_subqi 1
#define HAVE_sync_old_nandqi 1
#define HAVE_sync_new_andhi 1
#define HAVE_sync_new_iorhi 1
#define HAVE_sync_new_xorhi 1
#define HAVE_sync_new_addhi 1
#define HAVE_sync_new_subhi 1
#define HAVE_sync_new_nandhi 1
#define HAVE_sync_new_andqi 1
#define HAVE_sync_new_iorqi 1
#define HAVE_sync_new_xorqi 1
#define HAVE_sync_new_addqi 1
#define HAVE_sync_new_subqi 1
#define HAVE_sync_new_nandqi 1
extern rtx        gen_addsi3                   (rtx, rtx, rtx);
extern rtx        gen_addsf3                   (rtx, rtx, rtx);
extern rtx        gen_subsi3                   (rtx, rtx, rtx);
extern rtx        gen_subsf3                   (rtx, rtx, rtx);
extern rtx        gen_mulsi3_highpart          (rtx, rtx, rtx);
extern rtx        gen_umulsi3_highpart         (rtx, rtx, rtx);
extern rtx        gen_mulsi3                   (rtx, rtx, rtx);
extern rtx        gen_mulhisi3                 (rtx, rtx, rtx);
extern rtx        gen_umulhisi3                (rtx, rtx, rtx);
extern rtx        gen_muladdhisi               (rtx, rtx, rtx, rtx);
extern rtx        gen_mulsubhisi               (rtx, rtx, rtx, rtx);
extern rtx        gen_mulsf3                   (rtx, rtx, rtx);
extern rtx        gen_fmasf4                   (rtx, rtx, rtx, rtx);
extern rtx        gen_fnmasf4                  (rtx, rtx, rtx, rtx);
extern rtx        gen_divsi3                   (rtx, rtx, rtx);
extern rtx        gen_udivsi3                  (rtx, rtx, rtx);
extern rtx        gen_modsi3                   (rtx, rtx, rtx);
extern rtx        gen_umodsi3                  (rtx, rtx, rtx);
extern rtx        gen_abssi2                   (rtx, rtx);
extern rtx        gen_abssf2                   (rtx, rtx);
extern rtx        gen_sminsi3                  (rtx, rtx, rtx);
extern rtx        gen_uminsi3                  (rtx, rtx, rtx);
extern rtx        gen_smaxsi3                  (rtx, rtx, rtx);
extern rtx        gen_umaxsi3                  (rtx, rtx, rtx);
extern rtx        gen_clzsi2                   (rtx, rtx);
extern rtx        gen_negsi2                   (rtx, rtx);
extern rtx        gen_negsf2                   (rtx, rtx);
extern rtx        gen_andsi3                   (rtx, rtx, rtx);
extern rtx        gen_iorsi3                   (rtx, rtx, rtx);
extern rtx        gen_xorsi3                   (rtx, rtx, rtx);
extern rtx        gen_zero_extendhisi2         (rtx, rtx);
extern rtx        gen_zero_extendqisi2         (rtx, rtx);
extern rtx        gen_extendhisi2_internal     (rtx, rtx);
extern rtx        gen_extendqisi2_internal     (rtx, rtx);
extern rtx        gen_extv_internal            (rtx, rtx, rtx, rtx);
extern rtx        gen_extzv_internal           (rtx, rtx, rtx, rtx);
extern rtx        gen_fix_truncsfsi2           (rtx, rtx);
extern rtx        gen_fixuns_truncsfsi2        (rtx, rtx);
extern rtx        gen_floatsisf2               (rtx, rtx);
extern rtx        gen_floatunssisf2            (rtx, rtx);
extern rtx        gen_movdi_internal           (rtx, rtx);
extern rtx        gen_movsi_internal           (rtx, rtx);
extern rtx        gen_movhi_internal           (rtx, rtx);
extern rtx        gen_movqi_internal           (rtx, rtx);
extern rtx        gen_movsf_internal           (rtx, rtx);
extern rtx        gen_movdf_internal           (rtx, rtx);
extern rtx        gen_ashlsi3_internal         (rtx, rtx, rtx);
extern rtx        gen_ashrsi3                  (rtx, rtx, rtx);
extern rtx        gen_lshrsi3                  (rtx, rtx, rtx);
extern rtx        gen_rotlsi3                  (rtx, rtx, rtx);
extern rtx        gen_rotrsi3                  (rtx, rtx, rtx);
extern rtx        gen_zero_cost_loop_start     (rtx, rtx);
extern rtx        gen_zero_cost_loop_end       (rtx);
extern rtx        gen_movsicc_internal0        (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_movsicc_internal1        (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_movsfcc_internal0        (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_movsfcc_internal1        (rtx, rtx, rtx, rtx, rtx);
extern rtx        gen_seq_sf                   (rtx, rtx, rtx);
extern rtx        gen_slt_sf                   (rtx, rtx, rtx);
extern rtx        gen_sle_sf                   (rtx, rtx, rtx);
extern rtx        gen_suneq_sf                 (rtx, rtx, rtx);
extern rtx        gen_sunlt_sf                 (rtx, rtx, rtx);
extern rtx        gen_sunle_sf                 (rtx, rtx, rtx);
extern rtx        gen_sunordered_sf            (rtx, rtx, rtx);
extern rtx        gen_jump                     (rtx);
extern rtx        gen_indirect_jump_internal   (rtx);
extern rtx        gen_tablejump_internal       (rtx, rtx);
extern rtx        gen_call_internal            (rtx, rtx);
extern rtx        gen_call_value_internal      (rtx, rtx, rtx);
extern rtx        gen_entry                    (rtx);
extern rtx        gen_return                   (void);
extern rtx        gen_nop                      (void);
extern rtx        gen_eh_set_a0_windowed       (rtx);
extern rtx        gen_eh_set_a0_call0          (rtx);
extern rtx        gen_blockage                 (void);
extern rtx        gen_set_frame_ptr            (void);
extern rtx        gen_get_thread_pointersi     (rtx);
extern rtx        gen_set_thread_pointersi     (rtx);
extern rtx        gen_tls_func                 (rtx, rtx);
extern rtx        gen_tls_arg                  (rtx, rtx);
extern rtx        gen_tls_call                 (rtx, rtx, rtx, rtx);
extern rtx        gen_sync_lock_releasesi      (rtx, rtx);
extern rtx        gen_sync_compare_and_swapsi  (rtx, rtx, rtx, rtx);
extern rtx        gen_mulsidi3                 (rtx, rtx, rtx);
extern rtx        gen_umulsidi3                (rtx, rtx, rtx);
extern rtx        gen_ctzsi2                   (rtx, rtx);
extern rtx        gen_ffssi2                   (rtx, rtx);
extern rtx        gen_one_cmplsi2              (rtx, rtx);
extern rtx        gen_extendhisi2              (rtx, rtx);
extern rtx        gen_extendqisi2              (rtx, rtx);
extern rtx        gen_extv                     (rtx, rtx, rtx, rtx);
extern rtx        gen_extzv                    (rtx, rtx, rtx, rtx);
extern rtx        gen_movdi                    (rtx, rtx);
extern rtx        gen_movsi                    (rtx, rtx);
extern rtx        gen_movhi                    (rtx, rtx);
extern rtx        gen_movqi                    (rtx, rtx);
extern rtx        gen_reloadhi_literal         (rtx, rtx, rtx);
extern rtx        gen_reloadqi_literal         (rtx, rtx, rtx);
extern rtx        gen_movsf                    (rtx, rtx);
extern rtx        gen_movdf                    (rtx, rtx);
extern rtx        gen_movmemsi                 (rtx, rtx, rtx, rtx);
extern rtx        gen_ashlsi3                  (rtx, rtx, rtx);
extern rtx        gen_cbranchsi4               (rtx, rtx, rtx, rtx);
extern rtx        gen_cbranchsf4               (rtx, rtx, rtx, rtx);
extern rtx        gen_cstoresi4                (rtx, rtx, rtx, rtx);
extern rtx        gen_cstoresf4                (rtx, rtx, rtx, rtx);
extern rtx        gen_movsicc                  (rtx, rtx, rtx, rtx);
extern rtx        gen_movsfcc                  (rtx, rtx, rtx, rtx);
extern rtx        gen_indirect_jump            (rtx);
extern rtx        gen_tablejump                (rtx, rtx);
extern rtx        gen_sym_PLT                  (rtx);
#define GEN_CALL(A, B, C, D) gen_call ((A), (B))
extern rtx        gen_call                     (rtx, rtx);
#define GEN_CALL_VALUE(A, B, C, D, E) gen_call_value ((A), (B), (C))
extern rtx        gen_call_value               (rtx, rtx, rtx);
extern rtx        gen_prologue                 (void);
extern rtx        gen_epilogue                 (void);
extern rtx        gen_nonlocal_goto            (rtx, rtx, rtx, rtx);
extern rtx        gen_eh_return                (rtx);
extern rtx        gen_sym_TPOFF                (rtx);
extern rtx        gen_sym_DTPOFF               (rtx);
extern rtx        gen_memory_barrier           (void);
extern rtx        gen_sync_compare_and_swaphi  (rtx, rtx, rtx, rtx);
extern rtx        gen_sync_compare_and_swapqi  (rtx, rtx, rtx, rtx);
extern rtx        gen_sync_lock_test_and_sethi (rtx, rtx, rtx);
extern rtx        gen_sync_lock_test_and_setqi (rtx, rtx, rtx);
extern rtx        gen_sync_andhi               (rtx, rtx);
extern rtx        gen_sync_iorhi               (rtx, rtx);
extern rtx        gen_sync_xorhi               (rtx, rtx);
extern rtx        gen_sync_addhi               (rtx, rtx);
extern rtx        gen_sync_subhi               (rtx, rtx);
extern rtx        gen_sync_nandhi              (rtx, rtx);
extern rtx        gen_sync_andqi               (rtx, rtx);
extern rtx        gen_sync_iorqi               (rtx, rtx);
extern rtx        gen_sync_xorqi               (rtx, rtx);
extern rtx        gen_sync_addqi               (rtx, rtx);
extern rtx        gen_sync_subqi               (rtx, rtx);
extern rtx        gen_sync_nandqi              (rtx, rtx);
extern rtx        gen_sync_old_andhi           (rtx, rtx, rtx);
extern rtx        gen_sync_old_iorhi           (rtx, rtx, rtx);
extern rtx        gen_sync_old_xorhi           (rtx, rtx, rtx);
extern rtx        gen_sync_old_addhi           (rtx, rtx, rtx);
extern rtx        gen_sync_old_subhi           (rtx, rtx, rtx);
extern rtx        gen_sync_old_nandhi          (rtx, rtx, rtx);
extern rtx        gen_sync_old_andqi           (rtx, rtx, rtx);
extern rtx        gen_sync_old_iorqi           (rtx, rtx, rtx);
extern rtx        gen_sync_old_xorqi           (rtx, rtx, rtx);
extern rtx        gen_sync_old_addqi           (rtx, rtx, rtx);
extern rtx        gen_sync_old_subqi           (rtx, rtx, rtx);
extern rtx        gen_sync_old_nandqi          (rtx, rtx, rtx);
extern rtx        gen_sync_new_andhi           (rtx, rtx, rtx);
extern rtx        gen_sync_new_iorhi           (rtx, rtx, rtx);
extern rtx        gen_sync_new_xorhi           (rtx, rtx, rtx);
extern rtx        gen_sync_new_addhi           (rtx, rtx, rtx);
extern rtx        gen_sync_new_subhi           (rtx, rtx, rtx);
extern rtx        gen_sync_new_nandhi          (rtx, rtx, rtx);
extern rtx        gen_sync_new_andqi           (rtx, rtx, rtx);
extern rtx        gen_sync_new_iorqi           (rtx, rtx, rtx);
extern rtx        gen_sync_new_xorqi           (rtx, rtx, rtx);
extern rtx        gen_sync_new_addqi           (rtx, rtx, rtx);
extern rtx        gen_sync_new_subqi           (rtx, rtx, rtx);
extern rtx        gen_sync_new_nandqi          (rtx, rtx, rtx);

#endif /* GCC_INSN_FLAGS_H */
