#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "parser.h"

Token *iTk;				// the iterator in the tokens list
Token *consumedTk;		// the last consumed token


void tkerr(const char *fmt,...){
	fprintf(stderr,"error in line %d: ",iTk->line);
	va_list va;
	va_start(va,fmt);
	vfprintf(stderr,fmt,va);
	va_end(va);
	fprintf(stderr,"\n");
	exit(EXIT_FAILURE);
}

bool consume(int code){
	if(iTk->code==code){
		consumedTk=iTk;
		iTk=iTk->next;
		return true;
	}
	return false;
}

void parse(Token *tokens){
	iTk=tokens;
	if(!unit())tkerr("syntax error");
}

// unit: ( structDef | fnDef | varDef )* END
bool unit(){
	for(;;){
		if(structDef()){}
		else if(fnDef()){}
		else if(varDef()){}
		else break;
	}
	if(consume(END)){
		return true;
	}
	return false;
}

// structDef: STRUCT ID LACC varDef* RACC SEMICOLON
bool structDef(){
	Token *start = iTk;
	if(consume(STRUCT)){
		if(consume(ID)){
			if(consume(LACC)){
				while(varDef()){}
				if(consume(RACC)){
					if(consume(SEMICOLON)){
						return true;
					}else tkerr("missing ; after struct declaration");
				}else tkerr("missing } after struct declaration");
			}else tkerr("missing { after struct name");
		}else tkerr("missing struct name");
	}
	iTk = start;
	return false;
}

// varDef: typeBase ID arrayDecl? SEMICOLON
bool varDef(){
	Token *start = iTk;
	if(typeBase()){
		if(consume(ID)){
			if(arrayDecl()){}
			if(consume(SEMICOLON)){
				return true;
			}else tkerr("missing ; after variable declaration");
		}else tkerr("missing variable name");
	}
	iTk = start;
	return false;
}

// typeBase: TYPE_INT | TYPE_DOUBLE | TYPE_CHAR | STRUCT ID
bool typeBase(){
	if(consume(TYPE_INT)){
		return true;
	}
	if(consume(TYPE_DOUBLE)){
		return true;
	}
	if(consume(TYPE_CHAR)){
		return true;
	}
	if(consume(STRUCT)){
		if(consume(ID)){
			return true;
		}
	}
	return false;
}

// arrayDecl: LBRACKET INT? RBRACKET
bool arrayDecl(){
	Token *start = iTk;
	if(consume(LBRACKET)){
		if(consume(INT)){}
		if(consume(RBRACKET)){
			return true;
		}else tkerr("missing ] in array declaration");
	}
	iTk = start;
	return false;
}

/* fnDef: ( typeBase | VOID ) ID LPAR 
		( fnParam ( COMMA fnParam )* )? RPAR stmCompound */
bool fnDef(){
	Token *start = iTk;
	if(typeBase() || consume(VOID)){
		if(consume(ID)){
			if(consume(LPAR)){

				// ( fnParam ( COMMA fnParam )* )?
				if(fnParam()){
					while(consume(COMMA)){
						if(!fnParam()){
							tkerr("missing function parameter after ,");
						}
					}
				}

				if(consume(RPAR)){
					if(stmCompound()){
						return true;
					}else tkerr("missing function body");
				}else tkerr("missing ) after function parameters");
			}else tkerr("missing ( after function name");
		}else tkerr("missing function name");
	}
	iTk = start;
	return false;
}

// fnParam: typeBase ID arrayDecl?
bool fnParam(){
	Token *start = iTk;
	if(typeBase()){
		if(consume(ID)){
			if(arrayDecl()){}
			return true;
		}else tkerr("missing parameter name");
	}
	iTk = start;
	return false;
}

/* stm: stmCompound
		| IF LPAR expr RPAR stm ( ELSE stm )?
		| WHILE LPAR expr RPAR stm
		| RETURN expr? SEMICOLON
		| expr? SEMICOLON */
bool stm(){
	Token *start = iTk;
	if(stmCompound()){
		return true;
	}
	if(consume(IF)){
		if(consume(LPAR)){
			if(expr()){
				if(consume(RPAR)){
					if(stm()){
						if(consume(ELSE)){
							if(!stm()){
								tkerr("missing statement after else");
							}
						}
						return true;
					}else tkerr("missing statement after if");
				}else tkerr("missing ) after if condition");
			}else tkerr("invalid condition in if");
		}else("missing ( after if");
	}
	if(consume(WHILE)){
		if(consume(LPAR)){
			if(expr()){
				if(consume(RPAR)){
					if(stm()){
						return true;
					}else tkerr("missing statement after while");
				}else tkerr("missing ) after while");
			}else tkerr("invail condition in while");
		}else tkerr("missing ( after while");
	}
	if(consume(RETURN)){
		if(expr()){}
		if(consume(SEMICOLON)){
			return true;
		}else tkerr("missing ; after return");
	}
	if(expr()){
		if(consume(SEMICOLON)){
			return true;
		}else tkerr("missing ; after expression");
	}
	if(consume(SEMICOLON)){
		return true;
	}
	iTk = start;
	return false;
}

// stmCompound:  LACC ( varDef | stm )* RACC
bool stmCompound(){
	if(consume(LACC)){
		for(;;){
			if(varDef()){}
			else if(stm()){}
			else break;
		}
		if(consume(RACC)){
			return true;
		}else tkerr("missing }");
	}
	return false;
}