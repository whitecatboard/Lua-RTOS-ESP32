#if LUA_USE_SHELL

/* original parser id follows */
/* yysccsid[] = "@(#)yaccpar	1.9 (Berkeley) 02/21/93" */
/* (use YYMAJOR/YYMINOR for ifdefs dependent on parser version) */

#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define YYPATCH 20150711

#define YYEMPTY        (-1)
#define yyclearin      (yychar = YYEMPTY)
#define yyerrok        (yyerrflag = 0)
#define YYRECOVERING() (yyerrflag != 0)
#define YYENOMEM       (-2)
#define YYEOF          0
#define YYPREFIX "yy"

#define YYPURE 0

#line 2 "shell.y"
#include "lua.h"

#include <stdio.h>
#include <string.h>

static int arg = 0;
static const char *args[10];
static char buff[512];

void yyerror(const char *str) {
}

int yywrap() {
	return 0;
}

int yylex();

#line 22 "shell.y"
#ifdef YYSTYPE
#undef  YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
#endif
#ifndef YYSTYPE_IS_DECLARED
#define YYSTYPE_IS_DECLARED 1
typedef union  {char *string;} YYSTYPE;
#endif /* !YYSTYPE_IS_DECLARED */
#line 49 "y.tab.c"

/* compatibility with bison */
#ifdef YYPARSE_PARAM
/* compatibility with FreeBSD */
# ifdef YYPARSE_PARAM_TYPE
#  define YYPARSE_DECL() yyparse(YYPARSE_PARAM_TYPE YYPARSE_PARAM)
# else
#  define YYPARSE_DECL() yyparse(void *YYPARSE_PARAM)
# endif
#else
# define YYPARSE_DECL() yyparse(void)
#endif

/* Parameters sent to lex. */
#ifdef YYLEX_PARAM
# define YYLEX_DECL() yylex(void *YYLEX_PARAM)
# define YYLEX yylex(YYLEX_PARAM)
#else
# define YYLEX_DECL() yylex(void)
# define YYLEX yylex()
#endif

/* Parameters sent to yyerror. */
#ifndef YYERROR_DECL
#define YYERROR_DECL() yyerror(const char *s)
#endif
#ifndef YYERROR_CALL
#define YYERROR_CALL(msg) yyerror(msg)
#endif

extern int YYPARSE_DECL();

#define SLUA_COMMAND_CAT 257
#define SLUA_COMMAND_CD 258
#define SLUA_COMMAND_CLEAR 259
#define SLUA_COMMAND_CP 260
#define SLUA_COMMAND_DMESG 261
#define SLUA_COMMAND_EDIT 262
#define SLUA_COMMAND_EXIT 263
#define SLUA_COMMAND_LS 264
#define SLUA_COMMAND_MORE 265
#define SLUA_COMMAND_MKDIR 266
#define SLUA_COMMAND_MV 267
#define SLUA_COMMAND_PWD 268
#define SLUA_COMMAND_RM 269
#define SLUA_ARGUMENT 270
#define SLUA_EOL 271
#define SLUA_FILE 272
#define YYERRCODE 256
typedef short YYINT;
static const YYINT yylhs[] = {                           -1,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    1,    2,    2,    3,    4,
    5,    6,    6,    7,    8,    8,    9,   10,   11,   12,
   13,   14,   15,
};
static const YYINT yylen[] = {                            2,
    0,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    3,    3,    2,    2,    4,
    2,    3,    2,    2,    3,    2,    3,    3,    4,    2,
    3,    2,    1,
};
static const YYINT yydefred[] = {                         0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    2,    3,    4,    5,    6,
    7,    8,    9,   10,   11,   12,   13,   14,   15,   33,
    0,   18,    0,   19,    0,   21,   23,    0,   24,   26,
    0,    0,    0,    0,   30,    0,   32,   16,   17,    0,
   22,   25,   27,   28,    0,   31,   20,   29,
};
static const YYINT yydgoto[] = {                         15,
   16,   17,   18,   19,   20,   21,   22,   23,   24,   25,
   26,   27,   28,   29,   31,
};
static const YYINT yysindex[] = {                      -245,
 -269, -242, -268, -269, -266, -240, -261, -236, -269, -269,
 -269, -246, -269, -239,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
 -235,    0, -234,    0, -269,    0,    0, -233,    0,    0,
 -232, -231, -230, -269,    0, -228,    0,    0,    0, -227,
    0,    0,    0,    0, -226,    0,    0,    0,
};
static const YYINT yyrindex[] = {                        26,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,
};
static const YYINT yygindex[] = {                         0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   -2,
};
#define YYTABLESIZE 45
static const YYINT yytable[] = {                         33,
   30,   35,   34,   38,   36,   41,   42,   43,   44,   39,
   46,    1,    2,    3,    4,    5,    6,    7,    8,    9,
   10,   11,   12,   13,   45,    1,   14,   30,   32,   30,
   37,   47,   50,   30,   40,   48,   49,   51,   52,   53,
   54,   55,   56,   57,   58,
};
static const YYINT yycheck[] = {                          2,
  270,    4,  271,    6,  271,    8,    9,   10,   11,  271,
   13,  257,  258,  259,  260,  261,  262,  263,  264,  265,
  266,  267,  268,  269,  271,    0,  272,  270,  271,  270,
  271,  271,   35,  270,  271,  271,  271,  271,  271,  271,
  271,   44,  271,  271,  271,
};
#define YYFINAL 15
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 272
#define YYUNDFTOKEN 290
#define YYTRANSLATE(a) ((a) > YYMAXTOKEN ? YYUNDFTOKEN : (a))
#if YYDEBUG
static const char *const yyname[] = {

"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"SLUA_COMMAND_CAT",
"SLUA_COMMAND_CD","SLUA_COMMAND_CLEAR","SLUA_COMMAND_CP","SLUA_COMMAND_DMESG",
"SLUA_COMMAND_EDIT","SLUA_COMMAND_EXIT","SLUA_COMMAND_LS","SLUA_COMMAND_MORE",
"SLUA_COMMAND_MKDIR","SLUA_COMMAND_MV","SLUA_COMMAND_PWD","SLUA_COMMAND_RM",
"SLUA_ARGUMENT","SLUA_EOL","SLUA_FILE",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
"illegal-symbol",
};
static const char *const yyrule[] = {
"$accept : commands",
"commands :",
"commands : cat_command",
"commands : cd_command",
"commands : clear_command",
"commands : cp_command",
"commands : dmesg_command",
"commands : edit_command",
"commands : exit_command",
"commands : ls_command",
"commands : more_command",
"commands : mkdir_command",
"commands : mv_command",
"commands : pwd_command",
"commands : rm_command",
"commands : run_command",
"cat_command : SLUA_COMMAND_CAT argument SLUA_EOL",
"cd_command : SLUA_COMMAND_CD argument SLUA_EOL",
"cd_command : SLUA_COMMAND_CD SLUA_EOL",
"clear_command : SLUA_COMMAND_CLEAR SLUA_EOL",
"cp_command : SLUA_COMMAND_CP argument argument SLUA_EOL",
"dmesg_command : SLUA_COMMAND_DMESG SLUA_EOL",
"edit_command : SLUA_COMMAND_EDIT argument SLUA_EOL",
"edit_command : SLUA_COMMAND_EDIT SLUA_EOL",
"exit_command : SLUA_COMMAND_EXIT SLUA_EOL",
"ls_command : SLUA_COMMAND_LS argument SLUA_EOL",
"ls_command : SLUA_COMMAND_LS SLUA_EOL",
"more_command : SLUA_COMMAND_MORE argument SLUA_EOL",
"mkdir_command : SLUA_COMMAND_MKDIR argument SLUA_EOL",
"mv_command : SLUA_COMMAND_MV argument argument SLUA_EOL",
"pwd_command : SLUA_COMMAND_PWD SLUA_EOL",
"rm_command : SLUA_COMMAND_RM argument SLUA_EOL",
"run_command : SLUA_FILE SLUA_EOL",
"argument : SLUA_ARGUMENT",

};
#endif

int      yydebug;
int      yynerrs;

int      yyerrflag;
int      yychar;
YYSTYPE  yyval;
YYSTYPE  yylval;

/* define the initial stack-sizes */
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH  YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 10000
#define YYMAXDEPTH  10000
#endif
#endif

#define YYINITSTACKSIZE 200

typedef struct {
    unsigned stacksize;
    YYINT    *s_base;
    YYINT    *s_mark;
    YYINT    *s_last;
    YYSTYPE  *l_base;
    YYSTYPE  *l_mark;
} YYSTACKDATA;
/* variables for the parser stack */
static YYSTACKDATA yystack;
#line 256 "shell.y"

void lua_shell(char *line) {
    arg = 0;

    // Add \n to end of line
    int len = strlen(line);
    
    line[len] = '\n';
    line[len + 1] = '\0';
    
    // Scan and parse
    yy_scan_string(line);
    if (!yyparse()) {
    	strcpy(line, buff);
    } else {
        // Remove \n to end of line
        line[len] = '\0';
    }
	
     yylex_destroy();
}

#line 277 "y.tab.c"

#if YYDEBUG
#include <stdio.h>		/* needed for printf */
#endif

#include <stdlib.h>	/* needed for malloc, etc */
#include <string.h>	/* needed for memset */

/* allocate initial stack or double stack size, up to YYMAXDEPTH */
static int yygrowstack(YYSTACKDATA *data)
{
    int i;
    unsigned newsize;
    YYINT *newss;
    YYSTYPE *newvs;

    if ((newsize = data->stacksize) == 0)
        newsize = YYINITSTACKSIZE;
    else if (newsize >= YYMAXDEPTH)
        return YYENOMEM;
    else if ((newsize *= 2) > YYMAXDEPTH)
        newsize = YYMAXDEPTH;

    i = (int) (data->s_mark - data->s_base);
    newss = (YYINT *)realloc(data->s_base, newsize * sizeof(*newss));
    if (newss == 0)
        return YYENOMEM;

    data->s_base = newss;
    data->s_mark = newss + i;

    newvs = (YYSTYPE *)realloc(data->l_base, newsize * sizeof(*newvs));
    if (newvs == 0)
        return YYENOMEM;

    data->l_base = newvs;
    data->l_mark = newvs + i;

    data->stacksize = newsize;
    data->s_last = data->s_base + newsize - 1;
    return 0;
}

#if YYPURE || defined(YY_NO_LEAKS)
static void yyfreestack(YYSTACKDATA *data)
{
    free(data->s_base);
    free(data->l_base);
    memset(data, 0, sizeof(*data));
}
#else
#define yyfreestack(data) /* nothing */
#endif

#define YYABORT  goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR  goto yyerrlab

int
YYPARSE_DECL()
{
    int yym, yyn, yystate;
#if YYDEBUG
    const char *yys;

    if ((yys = getenv("YYDEBUG")) != 0)
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = YYEMPTY;
    yystate = 0;

#if YYPURE
    memset(&yystack, 0, sizeof(yystack));
#endif

    if (yystack.s_base == NULL && yygrowstack(&yystack) == YYENOMEM) goto yyoverflow;
    yystack.s_mark = yystack.s_base;
    yystack.l_mark = yystack.l_base;
    yystate = 0;
    *yystack.s_mark = 0;

yyloop:
    if ((yyn = yydefred[yystate]) != 0) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = YYLEX) < 0) yychar = YYEOF;
#if YYDEBUG
        if (yydebug)
        {
            yys = yyname[YYTRANSLATE(yychar)];
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM)
        {
            goto yyoverflow;
        }
        yystate = yytable[yyn];
        *++yystack.s_mark = yytable[yyn];
        *++yystack.l_mark = yylval;
        yychar = YYEMPTY;
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;

    YYERROR_CALL("syntax error");

    goto yyerrlab;

yyerrlab:
    ++yynerrs;

yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yystack.s_mark]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yystack.s_mark, yytable[yyn]);
#endif
                if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM)
                {
                    goto yyoverflow;
                }
                yystate = yytable[yyn];
                *++yystack.s_mark = yytable[yyn];
                *++yystack.l_mark = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yystack.s_mark);
#endif
                if (yystack.s_mark <= yystack.s_base) goto yyabort;
                --yystack.s_mark;
                --yystack.l_mark;
            }
        }
    }
    else
    {
        if (yychar == YYEOF) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = yyname[YYTRANSLATE(yychar)];
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = YYEMPTY;
        goto yyloop;
    }

yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    if (yym)
        yyval = yystack.l_mark[1-yym];
    else
        memset(&yyval, 0, sizeof yyval);
    switch (yyn)
    {
case 16:
#line 63 "shell.y"
	{
		sprintf(
			buff, 
			"do local r,m,e = os.cat(\"%s\"); if (not (m == nil)) then print(\"cat: \"..m); end end",
			args[0]
		);
		YYACCEPT;
	}
break;
case 17:
#line 75 "shell.y"
	{
		sprintf(
			buff, 
			"do local r,m,e = os.cd(\"%s\"); if (not (m == nil)) then print(\"cd: \"..m); end end",
			args[0]
		);
		YYACCEPT;
	}
break;
case 18:
#line 85 "shell.y"
	{
		sprintf(
			buff, 
			"do local r,m,e = os.cd(\"/\"); if (not (m == nil)) then print(\"cd: \"..m); end end"
		);
		YYACCEPT;
	}
break;
case 19:
#line 96 "shell.y"
	{
		sprintf(
			buff, 
			"os.clear()"
		);
		YYACCEPT;
	}
break;
case 20:
#line 106 "shell.y"
	{
		sprintf(
			buff, 
			"do local r,m,e = os.cp(\"%s\",\"%s\"); if (not (m == nil)) then print(\"cp: \"..m); end end",
			args[0], args[1]
		);
		YYACCEPT;
    }
break;
case 21:
#line 118 "shell.y"
	{
		sprintf(
			buff, 
			"os.dmesg()"
		);
		YYACCEPT;
	}
break;
case 22:
#line 128 "shell.y"
	{
		sprintf(
			buff, 
			"do local r,m,e = os.edit(\"%s\"); if (not (m == nil)) then print(\"edit: \"..m); end end",
			args[0]
		);
		YYACCEPT;
	}
break;
case 23:
#line 138 "shell.y"
	{
		sprintf(
			buff, 
			"do local r,m,e = os.edit(); if (not (m == nil)) then print(\"edit: \"..m); end end"
		);
		YYACCEPT;
	}
break;
case 24:
#line 149 "shell.y"
	{
		sprintf(
			buff, 
			"os.exit()"
		);
		YYACCEPT;
	}
break;
case 25:
#line 159 "shell.y"
	{
		sprintf(
			buff, 
			"do local r,m,e = os.ls(\"%s\"); if (not (m == nil)) then print(\"ls: \"..m); end end",
			args[0]
		);
		YYACCEPT;
	}
break;
case 26:
#line 169 "shell.y"
	{
		sprintf(
			buff, 
			"do local r,m,e = os.ls(); if (not (m == nil)) then print(\"ls: \"..m); end end"
		);
        YYACCEPT;
	}
break;
case 27:
#line 180 "shell.y"
	{
		sprintf(
			buff, 
			"do local r,m,e = os.more(\"%s\"); if (not (m == nil)) then print(\"more: \"..m); end end",
			args[0]
		);
		YYACCEPT;
	}
break;
case 28:
#line 192 "shell.y"
	{
		sprintf(
			buff, 
			"do local r,m,e = os.mkdir(\"%s\"); if (not (m == nil)) then print(\"mkdir: \"..m); end end",
			args[0]
		);
		YYACCEPT;
	}
break;
case 29:
#line 204 "shell.y"
	{
		sprintf(
			buff, 
			"do local r,m,e = os.rename(\"%s\",\"%s\"); if (not (m == nil)) then print(\"mv: \"..m); end end",
			args[0], args[1]
		);
		YYACCEPT;
    }
break;
case 30:
#line 216 "shell.y"
	{
		sprintf(
			buff, 
			"os.pwd()"
		);
		YYACCEPT;
	}
break;
case 31:
#line 226 "shell.y"
	{
		sprintf(
			buff, 
			"do local r,m,e = os.remove(\"%s\"); if (not (m == nil)) then print(\"rm: \"..m); end end",
			args[0]
		);
		YYACCEPT;
    }
break;
case 32:
#line 238 "shell.y"
	{
		sprintf(
			buff, 
			"dofile(\"%s\")",
			yystack.l_mark[-1].string
		);
		YYACCEPT;
    }
break;
case 33:
#line 250 "shell.y"
	{
		args[arg++] = yystack.l_mark[0].string;
    }
break;
#line 665 "y.tab.c"
    }
    yystack.s_mark -= yym;
    yystate = *yystack.s_mark;
    yystack.l_mark -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yystack.s_mark = YYFINAL;
        *++yystack.l_mark = yyval;
        if (yychar < 0)
        {
            if ((yychar = YYLEX) < 0) yychar = YYEOF;
#if YYDEBUG
            if (yydebug)
            {
                yys = yyname[YYTRANSLATE(yychar)];
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == YYEOF) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yystack.s_mark, yystate);
#endif
    if (yystack.s_mark >= yystack.s_last && yygrowstack(&yystack) == YYENOMEM)
    {
        goto yyoverflow;
    }
    *++yystack.s_mark = (YYINT) yystate;
    *++yystack.l_mark = yyval;
    goto yyloop;

yyoverflow:
    YYERROR_CALL("yacc stack overflow");

yyabort:
    yyfreestack(&yystack);
    return (1);

yyaccept:
    yyfreestack(&yystack);
    return (0);
}

#endif
