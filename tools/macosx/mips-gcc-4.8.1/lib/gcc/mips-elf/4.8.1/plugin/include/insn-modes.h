/* Generated automatically from machmode.def and config/mips/mips-modes.def
   by genmodes.  */

#ifndef GCC_INSN_MODES_H
#define GCC_INSN_MODES_H

enum machine_mode
{
  VOIDmode,                /* machmode.def:172 */
  BLKmode,                 /* machmode.def:176 */
  CCmode,                  /* machmode.def:204 */
  CCV2mode,                /* config/mips/mips-modes.def:39 */
  CCV4mode,                /* config/mips/mips-modes.def:43 */
  CCDSPmode,               /* config/mips/mips-modes.def:48 */
  BImode,                  /* machmode.def:179 */
  QImode,                  /* machmode.def:184 */
  HImode,                  /* machmode.def:185 */
  SImode,                  /* machmode.def:186 */
  DImode,                  /* machmode.def:187 */
  TImode,                  /* machmode.def:188 */
  QQmode,                  /* machmode.def:207 */
  HQmode,                  /* machmode.def:208 */
  SQmode,                  /* machmode.def:209 */
  DQmode,                  /* machmode.def:210 */
  TQmode,                  /* machmode.def:211 */
  UQQmode,                 /* machmode.def:213 */
  UHQmode,                 /* machmode.def:214 */
  USQmode,                 /* machmode.def:215 */
  UDQmode,                 /* machmode.def:216 */
  UTQmode,                 /* machmode.def:217 */
  HAmode,                  /* machmode.def:219 */
  SAmode,                  /* machmode.def:220 */
  DAmode,                  /* machmode.def:221 */
  TAmode,                  /* machmode.def:222 */
  UHAmode,                 /* machmode.def:224 */
  USAmode,                 /* machmode.def:225 */
  UDAmode,                 /* machmode.def:226 */
  UTAmode,                 /* machmode.def:227 */
  SFmode,                  /* machmode.def:199 */
  DFmode,                  /* machmode.def:200 */
  TFmode,                  /* config/mips/mips-modes.def:20 */
  SDmode,                  /* machmode.def:239 */
  DDmode,                  /* machmode.def:240 */
  TDmode,                  /* machmode.def:241 */
  CQImode,                 /* machmode.def:235 */
  CHImode,                 /* machmode.def:235 */
  CSImode,                 /* machmode.def:235 */
  CDImode,                 /* machmode.def:235 */
  CTImode,                 /* machmode.def:235 */
  SCmode,                  /* machmode.def:236 */
  DCmode,                  /* machmode.def:236 */
  TCmode,                  /* machmode.def:236 */
  V4QImode,                /* config/mips/mips-modes.def:23 */
  V2HImode,                /* config/mips/mips-modes.def:23 */
  V8QImode,                /* config/mips/mips-modes.def:24 */
  V4HImode,                /* config/mips/mips-modes.def:24 */
  V2SImode,                /* config/mips/mips-modes.def:24 */
  V16QImode,               /* config/mips/mips-modes.def:28 */
  V8HImode,                /* config/mips/mips-modes.def:29 */
  V4SImode,                /* config/mips/mips-modes.def:30 */
  V4QQmode,                /* config/mips/mips-modes.def:33 */
  V2HQmode,                /* config/mips/mips-modes.def:33 */
  V4UQQmode,               /* config/mips/mips-modes.def:34 */
  V2UHQmode,               /* config/mips/mips-modes.def:34 */
  V2HAmode,                /* config/mips/mips-modes.def:35 */
  V2UHAmode,               /* config/mips/mips-modes.def:36 */
  V2SFmode,                /* config/mips/mips-modes.def:25 */
  V4SFmode,                /* config/mips/mips-modes.def:31 */
  MAX_MACHINE_MODE,

  MIN_MODE_RANDOM = VOIDmode,
  MAX_MODE_RANDOM = BLKmode,

  MIN_MODE_CC = CCmode,
  MAX_MODE_CC = CCDSPmode,

  MIN_MODE_INT = QImode,
  MAX_MODE_INT = TImode,

  MIN_MODE_PARTIAL_INT = VOIDmode,
  MAX_MODE_PARTIAL_INT = VOIDmode,

  MIN_MODE_FRACT = QQmode,
  MAX_MODE_FRACT = TQmode,

  MIN_MODE_UFRACT = UQQmode,
  MAX_MODE_UFRACT = UTQmode,

  MIN_MODE_ACCUM = HAmode,
  MAX_MODE_ACCUM = TAmode,

  MIN_MODE_UACCUM = UHAmode,
  MAX_MODE_UACCUM = UTAmode,

  MIN_MODE_FLOAT = SFmode,
  MAX_MODE_FLOAT = TFmode,

  MIN_MODE_DECIMAL_FLOAT = SDmode,
  MAX_MODE_DECIMAL_FLOAT = TDmode,

  MIN_MODE_COMPLEX_INT = CQImode,
  MAX_MODE_COMPLEX_INT = CTImode,

  MIN_MODE_COMPLEX_FLOAT = SCmode,
  MAX_MODE_COMPLEX_FLOAT = TCmode,

  MIN_MODE_VECTOR_INT = V4QImode,
  MAX_MODE_VECTOR_INT = V4SImode,

  MIN_MODE_VECTOR_FRACT = V4QQmode,
  MAX_MODE_VECTOR_FRACT = V2HQmode,

  MIN_MODE_VECTOR_UFRACT = V4UQQmode,
  MAX_MODE_VECTOR_UFRACT = V2UHQmode,

  MIN_MODE_VECTOR_ACCUM = V2HAmode,
  MAX_MODE_VECTOR_ACCUM = V2HAmode,

  MIN_MODE_VECTOR_UACCUM = V2UHAmode,
  MAX_MODE_VECTOR_UACCUM = V2UHAmode,

  MIN_MODE_VECTOR_FLOAT = V2SFmode,
  MAX_MODE_VECTOR_FLOAT = V4SFmode,

  NUM_MACHINE_MODES = MAX_MACHINE_MODE
};

#define CONST_MODE_SIZE
#define CONST_MODE_BASE_ALIGN
#define CONST_MODE_IBIT const
#define CONST_MODE_FBIT const

#endif /* insn-modes.h */
