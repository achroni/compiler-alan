%{
#include "ast.hpp"
#include "parser.hpp"
	
#define T_eof 0

int comment=0,linenumber=1; //all editors count from 1

void yyerror (const char *msg);

char* formatString(char* str){
	int len = strlen(str) +1;
	char * temp = (char*) malloc(sizeof(char)*len);
	char * hex = (char*) malloc(sizeof(char)*3);
	int i=0;
	int j=0;
	for(i=0; i<len-1; i++){
		if (str[i]=='\\'){
			switch(str[i+1]){
				case 'n':
					temp[j++] = '\n';	
					break;
				case 't':
					temp[j++] = '\t';	
					break;
				case 'r':
					temp[j++] = '\r';	
					break;
				case '0':
					temp[j++] = '\0';	
					break;
				case '\\':
					temp[j++] = '\\';	
					break;
				case '\'':
					temp[j++] = '\'';	
					break;
				case '\"':
					temp[j++] = '\"';	
					break;
				case 'x':
					strncpy(hex,&str[i+2],sizeof(char)*2);
					temp[j++] = (int)strtol(hex, NULL, 16);
					i= i+2;
					break;
			}
			i++;
		}
		else{
			temp[j++] = str[i];
		}
	}
	temp[j]= '\0';
	free(hex);
	return temp;

} 

char* trimString(char * str){
	int len = strlen(str);
	char * temp = (char*) malloc(sizeof(char)*len);
	int i=0;
	for(i=0; i<len-2; i++){
		temp[i] = str[i+1];
	}
	temp[len-2] = '\0';
	return temp;
}

char stringToChar(char* str){
	int l = strlen(str);
	char res;
	char * temp = (char*)malloc(sizeof(char)*(l+1));
	int i=0;
	for (i=0; i<l-2; i++){
		temp[i] = str[i+1];
	}
	temp[l-2] = '\0';

	l = strlen(temp);
	
	switch(l){
		case 2:
			if(strcmp(temp,"\\n")==0)
				res = 10;
			if(strcmp(temp,"\\t")==0)
				res = 9;
			if(strcmp(temp,"\\r")==0)
				res = 13;
			if(strcmp(temp,"\\0")==0)
				res = 0;
			if(strcmp(temp,"\\\\")==0)
				res = 92;
			if(strcmp(temp,"\\\'")==0)
				res = 39;
			if(strcmp(temp,"\\\"")==0)
				res = 34;
			break;
		case 4:
			res = (int)strtol(&temp[2], NULL, 16);
			break;
		case 1:
			res = temp[0];
			break;
		default:
			printf("Lexer recognized invalid character!\n");
			exit(-1);
	}
	free(temp);
	return (char) res;
}


%}



L [A-Za-z]
D [0-9]
W [ \t\n\r]
S [ \t]
Esc [ntr0\'\"\\]
H [0-9a-fA-F]
Op [+\-*/%!&|=><]
Sep [()\[\]{},:;]
RestChars [.`~]

%x COMMENT

%%

byte		{  return T_byte;}
else		{  return T_else;}
false		{  return T_false;}
if			{  return T_if;}
int			{  return T_int;}
proc		{  return T_proc;}
reference	{  return T_reference;}
return		{  return T_return;}
while		{  return T_while;}
true		{  return T_true;}



{L}({L}|{D}|_)*								{	yylval.str = (char*) malloc(sizeof(char)*(strlen(yytext)+1));
												strcpy(yylval.str,yytext);  
												return T_id;
											}

{D}+										{ yylval.n = atoi(yytext); return T_integer;}

'{L}'|'{D}'|'{Op}'|'{S}'|'(\\{Esc})'|'("\\"(x|X){H}{H})'	    {	//yylval.c = yytext[0];
													yylval.c =  stringToChar(yytext);
													return T_char;
												}

\"(\\{Esc}|{L}|{D}|{S}|{Op}|{Sep}|{RestChars}|"\\"(x|X){H}{H})+\"		{	
																			yylval.str = (char*) malloc(sizeof(char)*(strlen(yytext)+1));
																			strcpy(yylval.str,formatString(trimString(yytext))); 
																			return T_str;
																		}

{Op}										{  return yytext[0];}

"=="										{  return T_equal;}

"!="										{  return T_not_equal;}

"<="										{  return T_less_eq;}

">="										{  return T_greater_eq;}

{Sep}										{  return yytext[0];}


"--"[^\n]*									{ linenumber++; /* ignore one line comment */}


"(*"						 				{ comment++; /*printf("Comment Number: %d\n",comment);*/ BEGIN(COMMENT);}
<COMMENT>"(*"				 				{ comment++; /*printf("Comment Number (nested): %d\n",comment);*/ BEGIN(COMMENT);}
<COMMENT>"*)"								{ comment--; /*printf("Comment Number again: %d\n",comment);*/ if(comment == 0) BEGIN(INITIAL);}
<COMMENT>\n									{ linenumber++;}
<COMMENT>"*"								{/*NOTHING*/}
<COMMENT>[^*\n]								{/*NOTHING*/}



\n			{ linenumber++; /*		*/}

{W}			/* nothing */

.			{yyerror("illegal character");}



%%

