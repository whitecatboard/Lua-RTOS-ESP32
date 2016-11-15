%{
#include <stdio.h>
#include <string.h>
#include "y.tab.h"
%}

%%

^[ ]*([/\.]|[a-z]|[A-Z]|(0-9))+\.lua yylval.string=strdup(yytext);return SLUA_FILE;

cat     yylval.string=strdup(yytext);return SLUA_COMMAND_CAT;
cd      yylval.string=strdup(yytext);return SLUA_COMMAND_CD;
clear   yylval.string=strdup(yytext);return SLUA_COMMAND_CLEAR;
cp      yylval.string=strdup(yytext);return SLUA_COMMAND_CP;
dmesg   yylval.string=strdup(yytext);return SLUA_COMMAND_DMESG;
edit    yylval.string=strdup(yytext);return SLUA_COMMAND_EDIT;
exit    yylval.string=strdup(yytext);return SLUA_COMMAND_EXIT;
ls	yylval.string=strdup(yytext);return SLUA_COMMAND_LS;
more    yylval.string=strdup(yytext);return SLUA_COMMAND_MORE;
mkdir   yylval.string=strdup(yytext);return SLUA_COMMAND_MKDIR;
mv      yylval.string=strdup(yytext);return SLUA_COMMAND_MV;
pwd     yylval.string=strdup(yytext);return SLUA_COMMAND_PWD;
rm      yylval.string=strdup(yytext);return SLUA_COMMAND_RM;

\n	yylval.string=strdup(yytext);return SLUA_EOL;

.|[ ]   ;

([/\.-]|[a-z]|[A-Z]|(0-9))+ yylval.string=strdup(yytext);return SLUA_ARGUMENT;

%%
