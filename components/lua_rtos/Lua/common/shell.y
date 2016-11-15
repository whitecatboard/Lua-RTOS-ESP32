%{
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

%}

%union  {char *string;}

%token  <string> SLUA_COMMAND_CAT
%token  <string> SLUA_COMMAND_CD
%token  <string> SLUA_COMMAND_CLEAR
%token  <string> SLUA_COMMAND_CP
%token  <string> SLUA_COMMAND_DMESG
%token  <string> SLUA_COMMAND_EDIT
%token  <string> SLUA_COMMAND_EXIT
%token  <string> SLUA_COMMAND_LS
%token  <string> SLUA_COMMAND_MORE
%token  <string> SLUA_COMMAND_MKDIR
%token  <string> SLUA_COMMAND_MV
%token  <string> SLUA_COMMAND_PWD
%token  <string> SLUA_COMMAND_RM

%token  <string> SLUA_ARGUMENT
%token  <string> SLUA_EOL
%token  <string> SLUA_FILE

%%

commands:
	| cat_command
	| cd_command
	| clear_command
	| cp_command
	| dmesg_command
	| edit_command
	| exit_command
	| ls_command
	| more_command
	| mkdir_command
	| mv_command
	| pwd_command
	| rm_command
	| run_command
	;

cat_command: 
	SLUA_COMMAND_CAT argument SLUA_EOL
    {
		sprintf(
			buff, 
			"do local r,m,e = os.cat(\"%s\"); if (not (m == nil)) then print(\"cat: \"..m); end end",
			args[0]
		);
		YYACCEPT;
	}
	;
	
cd_command: 
	SLUA_COMMAND_CD argument SLUA_EOL
    {
		sprintf(
			buff, 
			"do local r,m,e = os.cd(\"%s\"); if (not (m == nil)) then print(\"cd: \"..m); end end",
			args[0]
		);
		YYACCEPT;
	}
	|
	SLUA_COMMAND_CD SLUA_EOL
    {
		sprintf(
			buff, 
			"do local r,m,e = os.cd(\"/\"); if (not (m == nil)) then print(\"cd: \"..m); end end"
		);
		YYACCEPT;
	}
	;

clear_command: 
	SLUA_COMMAND_CLEAR SLUA_EOL
    {
		sprintf(
			buff, 
			"os.clear()"
		);
		YYACCEPT;
	}

cp_command:
	SLUA_COMMAND_CP argument argument SLUA_EOL
    {
		sprintf(
			buff, 
			"do local r,m,e = os.cp(\"%s\",\"%s\"); if (not (m == nil)) then print(\"cp: \"..m); end end",
			args[0], args[1]
		);
		YYACCEPT;
    }
    ;

dmesg_command: 
	SLUA_COMMAND_DMESG SLUA_EOL
    {
		sprintf(
			buff, 
			"os.dmesg()"
		);
		YYACCEPT;
	}

edit_command: 
	SLUA_COMMAND_EDIT argument SLUA_EOL
    {
		sprintf(
			buff, 
			"do local r,m,e = os.edit(\"%s\"); if (not (m == nil)) then print(\"edit: \"..m); end end",
			args[0]
		);
		YYACCEPT;
	}
	|
	SLUA_COMMAND_EDIT SLUA_EOL
    {
		sprintf(
			buff, 
			"do local r,m,e = os.edit(); if (not (m == nil)) then print(\"edit: \"..m); end end"
		);
		YYACCEPT;
	}
	;

exit_command: 
	SLUA_COMMAND_EXIT SLUA_EOL
    {
		sprintf(
			buff, 
			"os.exit()"
		);
		YYACCEPT;
	}

ls_command: 
	SLUA_COMMAND_LS argument SLUA_EOL
    {
		sprintf(
			buff, 
			"do local r,m,e = os.ls(\"%s\"); if (not (m == nil)) then print(\"ls: \"..m); end end",
			args[0]
		);
		YYACCEPT;
	}
	|
    SLUA_COMMAND_LS SLUA_EOL
    {
		sprintf(
			buff, 
			"do local r,m,e = os.ls(); if (not (m == nil)) then print(\"ls: \"..m); end end"
		);
        YYACCEPT;
	}
	;

more_command: 
	SLUA_COMMAND_MORE argument SLUA_EOL
    {
		sprintf(
			buff, 
			"do local r,m,e = os.more(\"%s\"); if (not (m == nil)) then print(\"more: \"..m); end end",
			args[0]
		);
		YYACCEPT;
	}
	;

mkdir_command: 
	SLUA_COMMAND_MKDIR argument SLUA_EOL
    {
		sprintf(
			buff, 
			"do local r,m,e = os.mkdir(\"%s\"); if (not (m == nil)) then print(\"mkdir: \"..m); end end",
			args[0]
		);
		YYACCEPT;
	}
	;

mv_command:
	SLUA_COMMAND_MV argument argument SLUA_EOL
    {
		sprintf(
			buff, 
			"do local r,m,e = os.rename(\"%s\",\"%s\"); if (not (m == nil)) then print(\"mv: \"..m); end end",
			args[0], args[1]
		);
		YYACCEPT;
    }
    ;

pwd_command: 
	SLUA_COMMAND_PWD SLUA_EOL
    {
		sprintf(
			buff, 
			"os.pwd()"
		);
		YYACCEPT;
	}

rm_command:
	SLUA_COMMAND_RM argument SLUA_EOL
    {
		sprintf(
			buff, 
			"do local r,m,e = os.remove(\"%s\"); if (not (m == nil)) then print(\"rm: \"..m); end end",
			args[0]
		);
		YYACCEPT;
    }
    ;

run_command:
	SLUA_FILE SLUA_EOL
    {
		sprintf(
			buff, 
			"dofile(\"%s\")",
			$1
		);
		YYACCEPT;
    }
    ;

argument:
	SLUA_ARGUMENT
    {
		args[arg++] = $1;
    }
	;

%%

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
