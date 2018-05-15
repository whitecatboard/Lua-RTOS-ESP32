/*
 * Copyright (C) 2015 - 2018, IBEROXARXA SERVICIOS INTEGRALES, S.L.
 * Copyright (C) 2015 - 2018, Jaume Olivé Petrus (jolive@whitecatboard.org)
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *     * The WHITECAT logotype cannot be changed, you can remove it, but you
 *       cannot change it in any way. The WHITECAT logotype is:
 *
 *          /\       /\
 *         /  \_____/  \
 *        /_____________\
 *        W H I T E C A T
 *
 *     * Redistributions in binary form must retain all copyright notices printed
 *       to any local or remote output device. This include any reference to
 *       Lua RTOS, whitecatboard.org, Lua, and other copyright notices that may
 *       appear in the future.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Lua RTOS, JIT byte-code optimizer
 *
 */

#if CONFIG_LUA_RTOS_LUA_USE_JIT_BYTECODE_OPTIMIZER

#include <ctype.h>

// Enable / disable optimizer debug messages
#define OPT_ENABLE_DEBUG 0

#if OPT_ENABLE_DEBUG
#define OPT_DEBUG(...) printf( __VA_ARGS__ )
#define OPT_DEBUG_CODE(cl,c,f,t)\
    PrintCode(cl, c, f, t);\

#else
#define OPT_DEBUG(...)
#define OPT_DEBUG_CODE(cl,c,f,t)
#endif

// NOP (not operation instruction). This instruction is an special instruction used
// by the optimizer to mark instructions that must be deleted in the optimized code.
#define OPT_NOP 0xffffffff

#define OPT_INF_EXCLUDED (1 << 0) // The instruction can't participate in future
                                  // optimization processes

#define OPT_INF_UPDATE(k, f)\
    oicode[pc + k - 1] |= f

// Get the instruction at position pc + k
#define OPT_INS(k) \
    ocode[pc + k - 1]

// Get the instruction's op code at position pc + i
#define OPT_OPCODE(k) \
    ((pc + k <= sizecode)?((oicode[pc + k - 1] & OPT_INF_EXCLUDED)?OPT_NOP:GET_OPCODE(OPT_INS(k))):OPT_NOP)

// Transform num instructions before / after the pc to
// a NOP instruction
#define OPT_TO_NOP(num) {\
        int _saved_pc = pc;\
        int _items = (((num) < 0)?-(num):(num));\
        int _inc = (((num) < 0)?-1:1);\
\
        while (_items > 0) {\
            pc += _inc;\
            OPT_INS(0) = OPT_NOP;\
            _items--;\
        }\
\
        pc = _saved_pc;\
}

// Instruction is a binary operation instruction?
#define OPT_IS_BIN_OP(i) \
      ((OPT_OPCODE(i) == OP_ADD) ||\
      (OPT_OPCODE(i) == OP_SUB) ||\
      (OPT_OPCODE(i) == OP_MUL) ||\
      (OPT_OPCODE(i) == OP_MOD) ||\
      (OPT_OPCODE(i) == OP_POW) ||\
      (OPT_OPCODE(i) == OP_DIV) ||\
      (OPT_OPCODE(i) == OP_IDIV) ||\
      (OPT_OPCODE(i) == OP_BAND) ||\
      (OPT_OPCODE(i) == OP_BOR) ||\
      (OPT_OPCODE(i) == OP_BXOR) ||\
      (OPT_OPCODE(i) == OP_SHL) ||\
      (OPT_OPCODE(i) == OP_SHR))

// Instruction is an unary operation instruction?
#define OPT_IS_UNA_OP(i) \
    ((OPT_OPCODE(i) == OP_BNOT) ||\
    (OPT_OPCODE(i) == OP_UNM) ||\
    (OPT_OPCODE(i) == OP_LEN) ||\
    (OPT_OPCODE(i) == OP_NOT))

#if OPT_ENABLE_DEBUG
#define VOID(p)     ((const void*)(p))

static void PrintString(const TString* ts) {
    const char* s = getstr(ts);
    size_t i, n = tsslen(ts);
    printf("%c", '"');
    for (i = 0; i < n; i++) {
        int c = (int) (unsigned char) s[i];
        switch (c) {
        case '"':
            printf("\\\"");
            break;
        case '\\':
            printf("\\\\");
            break;
        case '\a':
            printf("\\a");
            break;
        case '\b':
            printf("\\b");
            break;
        case '\f':
            printf("\\f");
            break;
        case '\n':
            printf("\\n");
            break;
        case '\r':
            printf("\\r");
            break;
        case '\t':
            printf("\\t");
            break;
        case '\v':
            printf("\\v");
            break;
        default:
            if (isprint(c))
                printf("%c", c);
            else
                printf("\\%03d", c);
        }
    }
    printf("%c", '"');
}

static void PrintConstant(const Proto* f, int i) {
    const TValue* o = &f->k[i];
    switch (ttype(o)) {
    case LUA_TNIL:
        printf("nil");
        break;
    case LUA_TBOOLEAN:
        printf(bvalue(o) ? "true" : "false");
        break;
    case LUA_TNUMFLT: {
        char buff[100];
        sprintf(buff, LUA_NUMBER_FMT, fltvalue(o));
        printf("%s", buff);
        if (buff[strspn(buff, "-0123456789")] == '\0')
            printf(".0");
        break;
    }
    case LUA_TNUMINT:
        printf(LUA_INTEGER_FMT, ivalue(o));
        break;
    case LUA_TSHRSTR:
    case LUA_TLNGSTR:
        PrintString(tsvalue(o));
        break;
    default: /* cannot happen */
        printf("? type=%d", ttype(o));
        break;
    }
}

#define UPVALNAME(x) ((f->upvalues[x].name) ? getstr(f->upvalues[x].name) : "-")
#define MYK(x)      (-1-(x))

#if 0
static void PrintCode(const Proto* f, int from, int to) {
    const Instruction* code = f->code;
    int pc;
    for (pc = from; pc < to; pc++) {
        Instruction i = code[pc];
        OpCode o = GET_OPCODE(i);
        int a = GETARG_A(i);
        int b = GETARG_B(i);
        int c = GETARG_C(i);
        int ax = GETARG_Ax(i);
        int bx = GETARG_Bx(i);
        int sbx = GETARG_sBx(i);
        int line = getfuncline(f, pc);
        printf("\t%d\t", pc + 1);
        if (line > 0)
            printf("[%d]\t", line);
        else
            printf("[-]\t");
        printf("%-9s\t", luaP_opnames[o]);
        switch (getOpMode(o)) {
        case iABC:
            printf("%d", a);
            if (getBMode(o) != OpArgN)
                printf(" %d", ISK(b) ? (MYK(INDEXK(b))) : b);
            if (getCMode(o) != OpArgN)
                printf(" %d", ISK(c) ? (MYK(INDEXK(c))) : c);
            break;
        case iABx:
            printf("%d", a);
            if (getBMode(o) == OpArgK)
                printf(" %d", MYK(bx));
            if (getBMode(o) == OpArgU)
                printf(" %d", bx);
            break;
        case iAsBx:
            printf("%d %d", a, sbx);
            break;
        case iAx:
            printf("%d", MYK(ax));
            break;
        }
        switch (o) {
        case OP_LOADK:
            printf("\t; ");
            PrintConstant(f, bx);
            break;
        case OP_GETUPVAL:
        case OP_SETUPVAL:
            printf("\t; %s", UPVALNAME(b));
            break;
        case OP_GETTABUP:
            printf("\t; %s", UPVALNAME(b));
            if (ISK(c)) {
                printf(" ");
                PrintConstant(f, INDEXK(c));
            }
            break;
        case OP_SETTABUP:
            printf("\t; %s", UPVALNAME(a));
            if (ISK(b)) {
                printf(" ");
                PrintConstant(f, INDEXK(b));
            }
            if (ISK(c)) {
                printf(" ");
                PrintConstant(f, INDEXK(c));
            }
            break;
        case OP_GETTABLE:
        case OP_SELF:
            if (ISK(c)) {
                printf("\t; ");
                PrintConstant(f, INDEXK(c));
            }
            break;
        case OP_SETTABLE:
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_POW:
        case OP_DIV:
        case OP_IDIV:
        case OP_BAND:
        case OP_BOR:
        case OP_BXOR:
        case OP_SHL:
        case OP_SHR:
        case OP_EQ:
        case OP_LT:
        case OP_LE:
            if (ISK(b) || ISK(c)) {
                printf("\t; ");
                if (ISK(b))
                    PrintConstant(f, INDEXK(b));
                else
                    printf("-");
                printf(" ");
                if (ISK(c))
                    PrintConstant(f, INDEXK(c));
                else
                    printf("-");
            }
            break;
        case OP_JMP:
        case OP_FORLOOP:
        case OP_FORPREP:
        case OP_TFORLOOP:
            printf("\t; to %d", sbx + pc + 2);
            break;
        case OP_CLOSURE:
            printf("\t; %p", VOID(f->p[bx]));
            break;
        case OP_SETLIST:
            if (c == 0)
                printf("\t; %d", (int) code[++pc]);
            else
                printf("\t; %d", c);
            break;
        case OP_EXTRAARG:
            printf("\t; ");
            PrintConstant(f, ax);
            break;
        default:
            break;
        }
        printf("\n");
    }
}
#endif
static void PrintCode(const Proto* f, Instruction *ocode, int from, int to) {
    int pc;
    Instruction *code = f->code;

    for (pc = from - 1; pc <= to - 1; pc++) {
        Instruction i = code[pc];
        Instruction io = ocode[pc];


    print_optimized: {
        if (i == OPT_NOP) continue;

        OpCode o = GET_OPCODE(i);
        int a = GETARG_A(i);
        int b = GETARG_B(i);
        int c = GETARG_C(i);
        int ax = GETARG_Ax(i);
        int bx = GETARG_Bx(i);
        int sbx = GETARG_sBx(i);
        int line = getfuncline(f, pc);
        printf("\t%d%s\t", pc + 1, ((io == OPT_NOP)?"-":(i != io)?"*":" "));
        if (line > 0)
            printf("[%d]\t", line);
        else
            printf("[-]\t");
        printf("%-9s\t", luaP_opnames[o]);
        switch (getOpMode(o)) {
        case iABC:
            printf("%d", a);
            if (getBMode(o) != OpArgN)
                printf(" %d", ISK(b) ? (MYK(INDEXK(b))) : b);
            if (getCMode(o) != OpArgN)
                printf(" %d", ISK(c) ? (MYK(INDEXK(c))) : c);
            break;
        case iABx:
            printf("%d", a);
            if (getBMode(o) == OpArgK)
                printf(" %d", MYK(bx));
            if (getBMode(o) == OpArgU)
                printf(" %d", bx);
            break;
        case iAsBx:
            printf("%d %d", a, sbx);
            break;
        case iAx:
            printf("%d", MYK(ax));
            break;
        }
        switch (o) {
        case OP_LOADK:
            printf("\t; ");
            PrintConstant(f, bx);
            break;
        case OP_GETUPVAL:
        case OP_SETUPVAL:
            printf("\t; %s", UPVALNAME(b));
            break;
        case OP_GETTABUP:
            printf("\t; %s", UPVALNAME(b));
            if (ISK(c)) {
                printf(" ");
                PrintConstant(f, INDEXK(c));
            }
            break;
        case OP_SETTABUP:
            printf("\t; %s", UPVALNAME(a));
            if (ISK(b)) {
                printf(" ");
                PrintConstant(f, INDEXK(b));
            }
            if (ISK(c)) {
                printf(" ");
                PrintConstant(f, INDEXK(c));
            }
            break;
        case OP_GETTABLE:
        case OP_SELF:
            if (ISK(c)) {
                printf("\t; ");
                PrintConstant(f, INDEXK(c));
            }
            break;
        case OP_SETTABLE:
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_POW:
        case OP_DIV:
        case OP_IDIV:
        case OP_BAND:
        case OP_BOR:
        case OP_BXOR:
        case OP_SHL:
        case OP_SHR:
        case OP_EQ:
        case OP_LT:
        case OP_LE:
            if (ISK(b) || ISK(c)) {
                printf("\t; ");
                if (ISK(b))
                    PrintConstant(f, INDEXK(b));
                else
                    printf("-");
                printf(" ");
                if (ISK(c))
                    PrintConstant(f, INDEXK(c));
                else
                    printf("-");
            }
            break;
        case OP_JMP:
        case OP_FORLOOP:
        case OP_FORPREP:
        case OP_TFORLOOP:
            printf("\t; to %d", sbx + pc + 2);
            break;
        case OP_CLOSURE:
            printf("\t; %p", VOID(f->p[bx]));
            break;
        case OP_SETLIST:
            if (c == 0)
                printf("\t; %d", (int) code[++pc]);
            else
                printf("\t; %d", c);
            break;
        case OP_EXTRAARG:
            printf("\t; ");
            PrintConstant(f, ax);
            break;
        default:
            break;
        }

        printf("\n");
        if (i != io) {
            i = io;
            goto print_optimized;
        }
    }
    }
}

#define SS(x)   ((x==1)?"":"s")
#define S(x)    (int)(x),SS(x)

static void PrintHeader(const Proto* f) {
    const char* s = f->source ? getstr(f->source) : "=?";
    if (*s == '@' || *s == '=')
        s++;
    else if (*s == LUA_SIGNATURE[0])
        s = "(bstring)";
    else
        s = "(string)";
    printf("\n%s <%s:%d,%d> (%d instruction%s at %p)\n",
            (f->linedefined == 0) ? "main" : "function", s, f->linedefined,
            f->lastlinedefined, S(f->sizecode), VOID(f));
    printf("%d%s param%s, %d slot%s, %d upvalue%s, ", (int) (f->numparams),
            f->is_vararg ? "+" : "", SS(f->numparams), S(f->maxstacksize),
            S(f->sizeupvalues));
    printf("%d local%s, %d constant%s, %d function%s\n", S(f->sizelocvars),
            S(f->sizek), S(f->sizep));
}

static void PrintDebug(const Proto* f) {
    int i, n;
    n = f->sizek;
    printf("constants (%d) for %p:\n", n, VOID(f));
    for (i = 0; i < n; i++) {
        printf("\t%d\t", i + 1);
        PrintConstant(f, i);
        printf("\n");
    }
    n = f->sizelocvars;
    printf("locals (%d) for %p:\n", n, VOID(f));
    for (i = 0; i < n; i++) {
        printf("\t%d\t%s\t%d\t%d\n", i, getstr(f->locvars[i].varname),
                f->locvars[i].startpc + 1, f->locvars[i].endpc + 1);
    }
    n = f->sizeupvalues;
    printf("upvalues (%d) for %p:\n", n, VOID(f));
    for (i = 0; i < n; i++) {
        printf("\t%d\t%s\t%d\t%d\n", i, UPVALNAME(i), f->upvalues[i].instack,
                f->upvalues[i].idx);
    }
}

#if 0
static void PrintFunction(const Proto* f, int full) {
    int i, n = f->sizep;
    PrintHeader(f);
    PrintCode(f,0,f->sizecode);
    if (full)
        PrintDebug(f);
    for (i = 0; i < n; i++)
        PrintFunction(f->p[i], full);
}
#endif

#endif

static int jit_opt_eval(lua_State *L, CallInfo *ci, LClosure *cl, Instruction *ocode, char *oicode, int pc, int nins, TValue *k, StkId base) {
    int ins = 0;
    int is_eval = 0;
    StkId ra;

    int sizecode = cl->p->sizecode;

    for (ins = 0; ins < nins; ins++) {
        ra = RA(OPT_INS(ins));

        switch (OPT_OPCODE(ins)) {
        vmcase(OP_GETUPVAL) {
          int b = GETARG_B(OPT_INS(ins));
          setobj2s(L, ra, cl->upvals[b]->v);

          is_eval  = (!ttisnil(ra) && (nins > 1));

          vmbreak;
        }
        vmcase(OP_GETTABUP) {
            TValue *upval = cl->upvals[GETARG_B(OPT_INS(ins))]->v;
            TValue *rc = RKC(OPT_INS(ins));

            gettableProtected(L, upval, rc, ra);

            is_eval  = (!ttisnil(ra) && (nins > 1));

            vmbreak;
        }
        vmcase(OP_GETTABLE) {
            StkId rb = RB(OPT_INS(ins));
            TValue *rc = RKC(OPT_INS(ins));

            // Skip evaluation if result is nil or is not a read-only table
            if (ttisnil(rb) || !ttisrotable(rb)) {
                is_eval = 0;
                break;
            }

            gettableProtected(L, rb, rc, ra);
            is_eval = !ttisnil(ra);

            vmbreak;
        }
        vmcase(OP_SELF) {
          const TValue *aux;
          StkId rb = RB(OPT_INS(ins));
          TValue *rc = RKC(OPT_INS(ins));
          TString *key = tsvalue(rc);  /* key must be a string */
          setobjs2s(L, ra + 1, rb);
          if (luaV_fastget(L, rb, key, aux, luaH_getstr)) {
            setobj2s(L, ra, aux);
          }
          else Protect(luaV_finishget(L, rb, rc, ra, aux));

          is_eval  = !ttisnil(ra);
          is_eval &= luaR_isrotable(fvalue(ra));

          vmbreak;
        }
        vmcase(OP_LOADK) {
            TValue *rb = k + GETARG_Bx(OPT_INS(ins));

            setobj2s(L, ra, rb);
            is_eval = 1;
            vmbreak;
        }
        vmcase(OP_ADD) {
          TValue *rb = RKB(OPT_INS(ins));
          TValue *rc = RKC(OPT_INS(ins));

          // Skip evaluation, if one of the operands are nil or not a number
          if (ttisnil(rb) || ttisnil(rc) || !ttisnumber(rb) || !ttisnumber(rc)) {
              is_eval = 0;
              break;
          }

          lua_Number nb; lua_Number nc;
          if (ttisinteger(rb) && ttisinteger(rc)) {
            lua_Integer ib = ivalue(rb); lua_Integer ic = ivalue(rc);
            setivalue(ra, intop(+, ib, ic));
          }
          else if (tonumber(rb, &nb) && tonumber(rc, &nc)) {
            setfltvalue(ra, luai_numadd(L, nb, nc));
          }
          else { Protect(luaT_trybinTM(L, rb, rc, ra, TM_ADD)); }
          is_eval = 1;
          vmbreak;
        }
        vmcase(OP_SUB) {
          TValue *rb = RKB(OPT_INS(ins));
          TValue *rc = RKC(OPT_INS(ins));

          // Skip evaluation, if one of the operands are nil or not a number
          if (ttisnil(rb) || ttisnil(rc) || !ttisnumber(rb) || !ttisnumber(rc)) {
              is_eval = 0;
              break;
          }

          lua_Number nb; lua_Number nc;
          if (ttisinteger(rb) && ttisinteger(rc)) {
            lua_Integer ib = ivalue(rb); lua_Integer ic = ivalue(rc);
            setivalue(ra, intop(-, ib, ic));
          }
          else if (tonumber(rb, &nb) && tonumber(rc, &nc)) {
            setfltvalue(ra, luai_numsub(L, nb, nc));
          }
          else { Protect(luaT_trybinTM(L, rb, rc, ra, TM_SUB)); }
          is_eval = 1;
         vmbreak;
        }
        vmcase(OP_MUL) {
          TValue *rb = RKB(OPT_INS(ins));
          TValue *rc = RKC(OPT_INS(ins));

          // Skip evaluation, if one of the operands are nil or not a number
          if (ttisnil(rb) || ttisnil(rc) || !ttisnumber(rb) || !ttisnumber(rc)) {
              is_eval = 0;
              break;
          }

          lua_Number nb; lua_Number nc;
          if (ttisinteger(rb) && ttisinteger(rc)) {
            lua_Integer ib = ivalue(rb); lua_Integer ic = ivalue(rc);
            setivalue(ra, intop(*, ib, ic));
          }
          else if (tonumber(rb, &nb) && tonumber(rc, &nc)) {
            setfltvalue(ra, luai_nummul(L, nb, nc));
          }
          else { Protect(luaT_trybinTM(L, rb, rc, ra, TM_MUL)); }
          is_eval = 1;
          vmbreak;
        }
        vmcase(OP_DIV) {  /* float division (always with floats) */
          TValue *rb = RKB(OPT_INS(ins));
          TValue *rc = RKC(OPT_INS(ins));

          // Skip evaluation, if one of the operands are nil or not a number
          if (ttisnil(rb) || ttisnil(rc) || !ttisnumber(rb) || !ttisnumber(rc)) {
              is_eval = 0;
              break;
          }

          lua_Number nb; lua_Number nc;
          if (tonumber(rb, &nb) && tonumber(rc, &nc)) {
            setfltvalue(ra, luai_numdiv(L, nb, nc));
          }
          else { Protect(luaT_trybinTM(L, rb, rc, ra, TM_DIV)); }
          is_eval = 1;
          vmbreak;
        }
        vmcase(OP_BAND) {
          TValue *rb = RKB(OPT_INS(ins));
          TValue *rc = RKC(OPT_INS(ins));

          // Skip evaluation, if one of the operands are nil or not an integer
          if (ttisnil(rb) || ttisnil(rc) || !ttisinteger(rb) || !ttisinteger(rc)) {
              is_eval = 0;
              break;
          }

          lua_Integer ib; lua_Integer ic;
          if (tointeger(rb, &ib) && tointeger(rc, &ic)) {
            setivalue(ra, intop(&, ib, ic));
          }
          else { Protect(luaT_trybinTM(L, rb, rc, ra, TM_BAND)); }
          is_eval = 1;
          vmbreak;
        }
        vmcase(OP_BOR) {
          TValue *rb = RKB(OPT_INS(ins));
          TValue *rc = RKC(OPT_INS(ins));

          // Skip evaluation, if one of the operands are nil or not an integer
          if (ttisnil(rb) || ttisnil(rc) || !ttisinteger(rb) || !ttisinteger(rc)) {
              is_eval = 0;
              break;
          }

          lua_Integer ib; lua_Integer ic;
          if (tointeger(rb, &ib) && tointeger(rc, &ic)) {
            setivalue(ra, intop(|, ib, ic));
          }
          else { Protect(luaT_trybinTM(L, rb, rc, ra, TM_BOR)); }
          is_eval = 1;
          vmbreak;
        }
        vmcase(OP_BXOR) {
          TValue *rb = RKB(OPT_INS(ins));
          TValue *rc = RKC(OPT_INS(ins));

          // Skip evaluation, if one of the operands are nil or not an integer
          if (ttisnil(rb) || ttisnil(rc) || !ttisinteger(rb) || !ttisinteger(rc)) {
              is_eval = 0;
              break;
          }

          lua_Integer ib; lua_Integer ic;
          if (tointeger(rb, &ib) && tointeger(rc, &ic)) {
            setivalue(ra, intop(^, ib, ic));
          }
          else { Protect(luaT_trybinTM(L, rb, rc, ra, TM_BXOR)); }
          is_eval = 1;
          vmbreak;
        }
        vmcase(OP_SHL) {
          TValue *rb = RKB(OPT_INS(ins));
          TValue *rc = RKC(OPT_INS(ins));

          // Skip evaluation, if one of the operands are nil or not an integer
          if (ttisnil(rb) || ttisnil(rc) || !ttisinteger(rb) || !ttisinteger(rc)) {
              is_eval = 0;
              break;
          }

          lua_Integer ib; lua_Integer ic;
          if (tointeger(rb, &ib) && tointeger(rc, &ic)) {
            setivalue(ra, luaV_shiftl(ib, ic));
          }
          else { Protect(luaT_trybinTM(L, rb, rc, ra, TM_SHL)); }
          is_eval = 1;
          vmbreak;
        }
        vmcase(OP_SHR) {
          TValue *rb = RKB(OPT_INS(ins));
          TValue *rc = RKC(OPT_INS(ins));

          // Skip evaluation, if one of the operands are nil or not an integer
          if (ttisnil(rb) || ttisnil(rc) || !ttisinteger(rb) || !ttisinteger(rc)) {
              is_eval = 0;
              break;
          }

          lua_Integer ib; lua_Integer ic;
          if (tointeger(rb, &ib) && tointeger(rc, &ic)) {
            setivalue(ra, luaV_shiftl(ib, -ic));
          }
          else { Protect(luaT_trybinTM(L, rb, rc, ra, TM_SHR)); }
          is_eval = 1;
          vmbreak;
        }
        vmcase(OP_IDIV) {  /* floor division */
          TValue *rb = RKB(OPT_INS(ins));
          TValue *rc = RKC(OPT_INS(ins));

          // Skip evaluation, if one of the operands are nil or not a number
          if (ttisnil(rb) || ttisnil(rc) || !ttisnumber(rb) || !ttisnumber(rc)) {
              is_eval = 0;
              break;
          }

          lua_Number nb; lua_Number nc;
          if (ttisinteger(rb) && ttisinteger(rc)) {
            lua_Integer ib = ivalue(rb); lua_Integer ic = ivalue(rc);
            setivalue(ra, luaV_div(L, ib, ic));
          }
          else if (tonumber(rb, &nb) && tonumber(rc, &nc)) {
            setfltvalue(ra, luai_numidiv(L, nb, nc));
          }
          else { Protect(luaT_trybinTM(L, rb, rc, ra, TM_IDIV)); }
          is_eval = 1;
          vmbreak;
        }
        vmcase(OP_MOD) {
          TValue *rb = RKB(OPT_INS(ins));
          TValue *rc = RKC(OPT_INS(ins));

          // Skip evaluation, if one of the operands are nil or not a number
          if (ttisnil(rb) || ttisnil(rc) || !ttisnumber(rb) || !ttisnumber(rc)) {
              is_eval = 0;
              break;
          }

          lua_Number nb; lua_Number nc;
          if (ttisinteger(rb) && ttisinteger(rc)) {
            lua_Integer ib = ivalue(rb); lua_Integer ic = ivalue(rc);
            setivalue(ra, luaV_mod(L, ib, ic));
          }
          else if (tonumber(rb, &nb) && tonumber(rc, &nc)) {
            lua_Number m;
            luai_nummod(L, nb, nc, m);
            setfltvalue(ra, m);
          }
          else { Protect(luaT_trybinTM(L, rb, rc, ra, TM_MOD)); }
          is_eval = 1;
          vmbreak;
        }
        vmcase(OP_POW) {
          TValue *rb = RKB(OPT_INS(ins));
          TValue *rc = RKC(OPT_INS(ins));

          // Skip evaluation, if one of the operands are nil or not a number
          if (ttisnil(rb) || ttisnil(rc) || !ttisnumber(rb) || !ttisnumber(rc)) {
              is_eval = 0;
              break;
          }

          lua_Number nb; lua_Number nc;
          if (tonumber(rb, &nb) && tonumber(rc, &nc)) {
            setfltvalue(ra, luai_numpow(L, nb, nc));
          }
          else { Protect(luaT_trybinTM(L, rb, rc, ra, TM_POW)); }
          is_eval = 1;
          vmbreak;
        }
        vmcase(OP_UNM) {
          TValue *rb = RB(OPT_INS(ins));

          // Skip evaluation, if the operands is nil or not a number
          if (ttisnil(rb) || !ttisnumber(rb)) {
              is_eval = 0;
              break;
          }

          lua_Number nb;
          if (ttisinteger(rb)) {
            lua_Integer ib = ivalue(rb);
            setivalue(ra, intop(-, 0, ib));
          }
          else if (tonumber(rb, &nb)) {
            setfltvalue(ra, luai_numunm(L, nb));
          }
          else {
            Protect(luaT_trybinTM(L, rb, rb, ra, TM_UNM));
          }
          is_eval = 1;
          vmbreak;
        }
        vmcase(OP_BNOT) {
          TValue *rb = RB(OPT_INS(ins));

          // Skip evaluation, if the operand is nil or integer
          if (ttisnil(rb) || !ttisinteger(rb)) {
              is_eval = 0;
              break;
          }

          lua_Integer ib;
          if (tointeger(rb, &ib)) {
            setivalue(ra, intop(^, ~l_castS2U(0), ib));
          }
          else {
            Protect(luaT_trybinTM(L, rb, rb, ra, TM_BNOT));
          }
          is_eval = 1;
          vmbreak;
        }
        vmcase(OP_NOT) {
          TValue *rb = RB(OPT_INS(ins));
          int res = l_isfalse(rb);  /* next assignment may change this value */
          setbvalue(ra, res);
          is_eval = 1;
          vmbreak;
        }
        vmcase(OP_LEN) {
          TValue *rb = RB(OPT_INS(ins));
          Protect(luaV_objlen(L, ra, rb));
          is_eval = 1;
          vmbreak;
        }

        default:
            break;
        }

        if (!is_eval) {
            break;
        }
    }

    if (is_eval) {
        // Result is in ra

        // TO DO: reuse constants

        // TO DO: use lua api for this?
        TValue *nk = malloc((cl->p->sizek + 1) * sizeof(TValue));
        if (!nk) {
            return -1;
        }

        // Make room for a new constant
        memcpy(nk, cl->p->k, cl->p->sizek * sizeof(TValue));
        cl->p->sizek++;
        free(cl->p->k);
        cl->p->k = nk;

        memcpy(&cl->p->k[cl->p->sizek - 1], ra, sizeof(TValue));

        return cl->p->sizek - 1;
    } else {
        return -1;
    }
}

#define jit_pattern_done()\
    optimized = 1;\
    continue;

#define jit_pattern_skip()

static int jit_opt(lua_State *L, CallInfo *ci, LClosure *cl) {
    int saved_pc;

    OPT_DEBUG("\r\noptimizing %s ...\r\n",getstr(cl->p->source));

    // Start at instruction 1
    int pc = 1;

    // Get clousure's code size
    int sizecode = cl->p->sizecode;

    // Allocate space for the optimized code. The optimization process
    // reduces the code size, so the instructions of the optimized
    // code will be <= clousure's code size
    OPT_DEBUG("\tallocating %d bytes for optimized code\r\n", sizecode * sizeof(Instruction));
    Instruction *ocode = calloc(sizecode, sizeof(Instruction));
    if (ocode == NULL) {
        OPT_DEBUG("\tno memory for optimized code");
        goto opt_exit;
    }

    // We need to store certain information for each of the optimized code
    // instruction

    // First create it into the clousure's prototype to preserve this information
    // between invocations of the jit_opt function
    if (cl->p->icode == NULL) {
        OPT_DEBUG("\tallocating %d bytes for code information\r\n", sizecode * sizeof(char));
        cl->p->icode = calloc(sizecode, sizeof(char));
        if (cl->p->icode == NULL) {
            OPT_DEBUG("\tno memory for code info");
            goto opt_exit;
        }
    } else {
        OPT_DEBUG("\treusing code information\r\n");
    }

    // Now, create the information for the current invocation of the jit_opt, and
    // initialize it with the preserved in the clousure's prototype
    OPT_DEBUG("\tallocating %d bytes for optimized code information\r\n", sizecode * sizeof(char));
    char *oicode = calloc(sizecode, sizeof(char));
    if (oicode == NULL) {
        OPT_DEBUG("\tno memory for optimized code info");
        goto opt_exit;
    }

    OPT_DEBUG("\r\n");

    memcpy(oicode, cl->p->icode, sizecode * sizeof(char));

    // Copy the clousure's code into the optimized code. Now two codes are
    // equal
    memcpy(ocode, cl->p->code, sizecode * sizeof(Instruction));

    // Optimize
    int pattern;  // Current optimization pattern is detected?
    TValue *k;    // Local reference to function's constant table
    StkId base;   // Function's base

    int optimized = 0;

    while (pc <= sizecode) {
        // Local reference to function's constant table and function's base
        k = cl->p->k;
        base = ci->u.l.base;

        pattern = ((OPT_OPCODE(0) == OP_GETUPVAL) && (OPT_OPCODE(1) == OP_SELF));
        if (pattern) {
            int ki = jit_opt_eval(L, ci, cl, ocode, oicode, pc, 2, k, base);
            if (ki > 0) {
                OPT_INS(0) = CREATE_ABx(OP_GETUPVAL, GETARG_A(OPT_INS(1)) + 1, GETARG_B(OPT_INS(0)));
                OPT_INS(1) = CREATE_ABx(OP_LOADK, GETARG_A(OPT_INS(1)), ki);

                OPT_INF_UPDATE(0, OPT_INF_EXCLUDED);
                OPT_INF_UPDATE(1, OPT_INF_EXCLUDED);

                OPT_DEBUG_CODE(cl->p, ocode, pc, pc + 1);
                pc += 2;
                jit_pattern_done();
            }
            jit_pattern_skip();
        }

        pattern = ((OPT_OPCODE(0) == OP_GETTABUP) && (OPT_OPCODE(1) == OP_SELF));
        if (pattern) {
            int ki = jit_opt_eval(L, ci, cl, ocode, oicode, pc, 2, k, base);
            if (ki > 0) {

                OPT_INS(0) = CREATE_ABC(OP_GETTABUP, GETARG_A(OPT_INS(1)) + 1, GETARG_B(OPT_INS(0)), GETARG_C(OPT_INS(0)));
                OPT_INS(1) = CREATE_ABx(OP_LOADK, GETARG_A(OPT_INS(1)), ki);

                OPT_INF_UPDATE(0, OPT_INF_EXCLUDED);
                OPT_INF_UPDATE(1, OPT_INF_EXCLUDED);

                OPT_DEBUG_CODE(cl->p, ocode, pc, pc + 1);
                pc += 2;
                jit_pattern_done();
            }
            jit_pattern_skip();
        }

        // GETTABUP;x+;y+ => LOADK;y+, where x+ = OP_GETTABLE
        pattern = (OPT_OPCODE(0) == OP_GETTABUP);
        if (pattern) {
            saved_pc = pc;

            pc++;
            int tables = 0;
            while (OPT_OPCODE(0) == OP_GETTABLE) {
                pc++;
                tables++;
            }

            if (pc != saved_pc + 1) {
                int ki = jit_opt_eval(L, ci, cl, ocode, oicode, saved_pc, pc - saved_pc, k, base);
                if (ki > 0) {
                    OPT_INS(-pc + saved_pc)= CREATE_ABx(OP_LOADK, GETARG_A(OPT_INS(-1)), ki);
                    OPT_TO_NOP(-pc + saved_pc + 1);
                    OPT_DEBUG_CODE(cl->p, ocode, saved_pc, pc - 1);
                    jit_pattern_done();
                }
                jit_pattern_skip();
            } else {
                pc = saved_pc;
            }
        }

        // GETTABUP;x+ => LOADK;x+
        pattern = (OPT_OPCODE(0) == OP_GETTABUP);
        if (pattern) {
            int ki = jit_opt_eval(L, ci, cl, ocode, oicode, pc, 1, k, base);
            if (ki > 0) {
                OPT_INS(0) = CREATE_ABx(OP_LOADK, GETARG_A(OPT_INS(0)), ki);
                OPT_DEBUG_CODE(cl->p, ocode, pc, pc);
                pc++;
                jit_pattern_done();
            }
            jit_pattern_skip();
        }

        // op(a0,b0,c0) => LOADK(a0, v(b0) op v(c0)), op=binary and isk(b0) and isk(c0)
        pattern = (OPT_IS_BIN_OP(0) && ISK(GETARG_B(OPT_INS(0))) && ISK(GETARG_C(OPT_INS(0))));
        if (pattern) {
            int ki = jit_opt_eval(L, ci, cl, ocode, oicode, pc, 1, k, base);
            if (ki > 0) {
                OPT_INS(0) = CREATE_ABx(OP_LOADK, GETARG_A(OPT_INS(0)), ki);
                OPT_DEBUG_CODE(cl->p, ocode, pc, pc);
                pc++;
                jit_pattern_done();
            }
            jit_pattern_skip();
        }

        // LOADK(a-2,b-2);LOADK(a-1,b-1);op(a0,b0,c0) => LOADK(a0, v(b0) op v(c0)), op=binary and !isk(b0) and !isk(c0)
        pattern = (OPT_IS_BIN_OP(0) && (OPT_OPCODE(-2) == OP_LOADK) && (OPT_OPCODE(-1) == OP_LOADK) && !ISK(GETARG_B(OPT_INS(0))) && !ISK(GETARG_C(OPT_INS(0))));
        if (pattern) {
            int ki = jit_opt_eval(L, ci, cl, ocode, oicode, pc-2, 3, k, base);
            if (ki > 0) {
                OPT_INS(0) = CREATE_ABx(OP_LOADK, GETARG_A(OPT_INS(0)), ki);
                OPT_DEBUG_CODE(cl->p, ocode, pc, pc);
                OPT_TO_NOP(-2);
                pc++;
                jit_pattern_done();
            }
            jit_pattern_skip();
        }

        // LOADK(a-1,b-1);op(a0,b0,c0) => LOADK(a0, v(b0) op v(c0)), op=binary and isk(c0)
        pattern = (OPT_IS_BIN_OP(0) && (OPT_OPCODE(-1) == OP_LOADK) && ISK(GETARG_C(OPT_INS(0))));
        if (pattern) {
            int ki = jit_opt_eval(L, ci, cl, ocode, oicode, pc-1, 2, k, base);
            if (ki > 0) {
                OPT_INS(0) = CREATE_ABx(OP_LOADK, GETARG_A(OPT_INS(0)), ki);
                OPT_DEBUG_CODE(cl->p, ocode, pc, pc);
                OPT_TO_NOP(-1);
                pc++;
                jit_pattern_done();
            }
            jit_pattern_skip();
        }

        // op(a0,b0) => LOADK(a0, op v(b0)), op=unary and isk(b0)
        pattern = (OPT_IS_UNA_OP(0) && ISK(GETARG_B(OPT_INS(0))));
        if (pattern) {
            int ki = jit_opt_eval(L, ci, cl, ocode, oicode, pc, 1, k, base);
            if (ki > 0) {
                OPT_INS(0) = CREATE_ABx(OP_LOADK, GETARG_A(OPT_INS(0)), ki);
                OPT_DEBUG_CODE(cl->p, ocode, pc, pc);
                pc++;
                jit_pattern_done();
            }
            jit_pattern_skip();
        }

        // LOADK(a-1,b-1);op(a0,b0) => LOADK(a0, op v(b-1)), op=unary and !isk(b0)
        pattern = (OPT_IS_UNA_OP(0) && !ISK(GETARG_B(OPT_INS(0))) && (OPT_OPCODE(-1) == OP_LOADK));
        if (pattern) {
            int ki = jit_opt_eval(L, ci, cl, ocode, oicode, pc-1, 2, k, base);
            if (ki > 0) {
                OPT_INS(0) = CREATE_ABx(OP_LOADK, GETARG_A(OPT_INS(0)), ki);
                OPT_DEBUG_CODE(cl->p, ocode, pc, pc);
                pc++;
                jit_pattern_done();
            }
            jit_pattern_skip();
        }

        OPT_DEBUG_CODE(cl->p, ocode, pc, pc);
        pc++;
    }

    if (optimized) {
        // Fix jumps
        int jumps; // How many instructions to jump
        int jump;  // Current jump
        int dir;   // Jump direction (-1 backward, 1 forward)

        OPT_DEBUG("\r\n");

        for (pc = 1; pc <= sizecode; pc++) {
            if ((OPT_INS(0) != OPT_NOP) && ((OPT_OPCODE(0) == OP_JMP) || (OPT_OPCODE(0) == OP_FORPREP) || (OPT_OPCODE(0) == OP_FORLOOP))) {
                // Store the current pc
                saved_pc = pc;

                // Determine the number of jumps, and the direction
                jumps = GETARG_sBx(OPT_INS(0));
                dir   = (jumps > 0)?1:-1;
                jump  = (jumps >0)?jumps:-jumps;

                while (jump > 0) {
                    pc += dir;
                    if (OPT_INS(0) == OPT_NOP) {
                        jumps += -dir;
                    }

                    jump--;
                }

                // Restore pc
                pc = saved_pc;

                OPT_DEBUG("\t%d, %-4s fixed to %d\r\n", pc, luaP_opnames[OPT_OPCODE(0)], jumps);

                // Fix
                SETARG_sBx(OPT_INS(0), jumps);
            }
        }

        // Now the clousure's code is the optimized code
        int i = 0;
        for (pc = 1; pc <= sizecode; pc++) {
            if (OPT_INS(0) != OPT_NOP) {
                cl->p->code[i] = OPT_INS(0);
                cl->p->icode[i++] = oicode[pc - 1];
            }
        }

        cl->p->sizecode = i;
    }

opt_exit:
    cl->p->optimized = 1;

    if (ocode != NULL) free(ocode);
    if (oicode != NULL) free(oicode);

    if (!optimized) {
        free(cl->p->icode);
        cl->p->icode = NULL;
    }

    return optimized;
}

#endif
