#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "parser.h"

Token *iTk;				// the iterator in the tokens list
Token *consumedTk;		// the last consumed token

bool unit();
bool structDef();
bool varDef();
bool typeBase();
bool arrayDecl();
bool fnDef();
bool fnParam();
bool stm();
bool stmCompound();

bool expr();
bool exprAssign();
bool exprOr();
bool exprOrPrim();
bool exprAnd();
bool exprAndPrim();
bool exprEq();
bool exprEqPrim();
bool exprRel();
bool exprRelPrim();
bool exprAdd();
bool exprAddPrim();
bool exprMul();
bool exprMulPrim();
bool exprCast();
bool exprUnary();
bool exprPostfix();
bool exprPostfixPrim();
bool exprPrimary();


void tkerr(const char *fmt,...){
	fprintf(stderr,"error in line %d: ",iTk->line);
	va_list va;
	va_start(va,fmt);
	vfprintf(stderr,fmt,va);
	va_end(va);
	fprintf(stderr,"\n");
	exit(EXIT_FAILURE);
}

/* const char *tkCodeName(int code){
    switch(code){
        case ID: return "ID";
        case TYPE_INT: return "TYPE_INT";
        case TYPE_DOUBLE: return "TYPE_DOUBLE";
        case TYPE_CHAR: return "TYPE_CHAR";
        case ELSE: return "ELSE";
        case IF: return "IF";
        case RETURN: return "RETURN";
        case STRUCT: return "STRUCT";
        case VOID: return "VOID";
        case WHILE: return "WHILE";

        case INT: return "INT";
        case DOUBLE: return "DOUBLE";
        case CHAR: return "CHAR";
        case STRING: return "STRING";

        case COMMA: return "COMMA";
        case SEMICOLON: return "SEMICOLON";
        case LPAR: return "LPAR";
        case RPAR: return "RPAR";
        case LBRACKET: return "LBRACKET";
        case RBRACKET: return "RBRACKET";
        case LACC: return "LACC";
        case RACC: return "RACC";
        case END: return "END";

        case ADD: return "ADD";
        case SUB: return "SUB";
        case MUL: return "MUL";
        case DIV: return "DIV";
        case DOT: return "DOT";
        case AND: return "AND";
        case OR: return "OR";
        case NOT: return "NOT";
        case ASSIGN: return "ASSIGN";
        case EQUAL: return "EQUAL";
        case NOTEQ: return "NOTEQ";
        case LESS: return "LESS";
        case LESSEQ: return "LESSEQ";
        case GREATER: return "GREATER";
        case GREATEREQ: return "GREATEREQ";

        default: return "UNKNOWN";
    }
}

bool consume(int code){
    printf("consume(%s)", tkCodeName(code));

    if(iTk->code == code){
        consumedTk = iTk;
        iTk = iTk->next;
        printf(" => consumed\n");
        return true;
    }

    printf(" => found %s\n", tkCodeName(iTk->code));
    return false;
}
*/

bool consume(int code){
    if(iTk->code == code){
        consumedTk = iTk;
        iTk = iTk->next;
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
			}
		}
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
		}else tkerr("missing struct name");
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
			}
		}
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
		}else tkerr("missing ( after if");
	}
	if(consume(WHILE)){
		if(consume(LPAR)){
			if(expr()){
				if(consume(RPAR)){
					if(stm()){
						return true;
					}else tkerr("missing statement after while");
				}else tkerr("missing ) after while");
			}else tkerr("invalid condition in while");
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

// expr: exprAssign
bool expr(){
	if(exprAssign()){
		return true;
	}
	return false;
}

// exprAssign: exprUnary ASSIGN exprAssign | exprOr
bool exprAssign(){
	Token *start = iTk;
	if(exprUnary()){
		if(consume(ASSIGN)){
			if(exprAssign()){
				return true;
			}else tkerr("missing expression after =");
		}
	}
	iTk = start;
	if(exprOr()){
		return true;
	}
	return false;
}

/* exprOr:  exprOr OR exprAnd | exprAnd
	->	exprOr: exprAnd exprOrPrim
		exprOrPrim: OR exprAnd exprOrPrim | epsilon
*/
bool exprOr(){
	if(exprAnd()){
		if(exprOrPrim()){
			return true;
		}
	}
	return false;
}

bool exprOrPrim(){
	if(consume(OR)){
		if(exprAnd()){
			if(exprOrPrim()){
				return true;
			}
		}else tkerr("missing expression after ||");
	}
	return true;
}

/* exprAnd: exprAnd AND exprEq | exprEq
	->	exprAnd: exprEq exprAndPrim
		exprAndPrim: AND exprEq exprAndPrim | epsilon
*/
bool exprAnd(){
	if(exprEq()){
		if(exprAndPrim()){
			return true;
		}
	}
	return false;
}

bool exprAndPrim(){
	if(consume(AND)){
		if(exprEq()){
			if(exprAndPrim()){
				return true;
			}
		}else tkerr("missing expression after &&");
	}
	return true;
}

/* exprEq: exprEq ( EQUAL | NOTEQ ) exprRel | exprRel
	->	exprEq: exprRel exprEqPrim
		exprEqPrim: ( EQUAL | NOTEQ ) exprRel exprEqPrim | epsilon
*/
bool exprEq(){
	if(exprRel()){
		if(exprEqPrim()){
			return true;
		}
	}
	return false;
}

bool exprEqPrim(){
	if(consume(EQUAL)){
		if(exprRel()){
			if(exprEqPrim()){
				return true;
			}
		}else tkerr("missing expression after ==");
	}
	if(consume(NOTEQ)){
		if(exprRel()){
			if(exprEqPrim()){
				return true;
			}
		}else tkerr("missing expression after !=");
	}
	return true;
}

/* exprRel: exprRel ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd | exprAdd
	->	exprRel: exprAdd exprRelPrim
		exprRelPrim: ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd exprRelPrim | epsilon
*/
bool exprRel(){
	if(exprAdd()){
		if(exprRelPrim()){
			return true;
		}
	}
	return false;
}

bool exprRelPrim(){
	if(consume(LESS)){
		if(exprAdd()){
			if(exprRelPrim()){
				return true;
			}
		}else tkerr("missing expression after <");
	}
	if(consume(LESSEQ)){
		if(exprAdd()){
			if(exprRelPrim()){
				return true;
			}
		}else tkerr("missing expression after <=");
	}
	if(consume(GREATER)){
		if(exprAdd()){
			if(exprRelPrim()){
				return true;
			}
		}else tkerr("missing expression after >");
	}
	if(consume(GREATEREQ)){
		if(exprAdd()){
			if(exprRelPrim()){
				return true;
			}
		}else tkerr("missing expression after >=");
	}
	return true;
}

/* exprAdd: exprAdd ( ADD | SUB ) exprMul | exprMul
	->	exprAdd: exprMul exprAddPrim
		exprAddPrim: ( ADD | SUB ) exprMul exprAddPrim | epsilon
*/
bool exprAdd(){
	if(exprMul()){
		if(exprAddPrim()){
			return true;
		}
	}
	return false;
}

bool exprAddPrim(){
	if(consume(ADD)){
		if(exprMul()){
			if(exprAddPrim()){
				return true;
			}
		}else tkerr("missing expression after +");
	}
	if(consume(SUB)){
		if(exprMul()){
			if(exprAddPrim()){
				return true;
			}
		}else tkerr("missing expression after -");
	}
	return true;
}

/* exprMul: exprMul ( MUL | DIV ) exprCast | exprCast 
	->	exprMul: exprCast exprMulPrim
		exprMulPrim: ( MUL | DIV ) exprCast exprMulPrim | epsilon
*/
bool exprMul(){
	if(exprCast()){
		if(exprMulPrim()){
			return true;
		}
	}
	return false;
}

bool exprMulPrim(){
	if(consume(MUL)){
		if(exprCast()){
			if(exprMulPrim()){
				return true;
			}
		}else tkerr("missing expression after *");
	}
	if(consume(DIV)){
		if(exprCast()){
			if(exprMulPrim()){
				return true;
			}
		}else tkerr("missing expression after /");
	}
	return true;
}

// exprCast: LPAR typeBase arrayDecl? RPAR exprCast | exprUnary
bool exprCast(){
	Token *start = iTk;
	if(consume(LPAR)){
		if(typeBase()){
			if(arrayDecl()){}
			if(consume(RPAR)){
				if(exprCast()){
					return true;
				}else tkerr("missing expression after cast");
			}else tkerr("missing ) after type in cast");
		}
	}
	iTk = start;
	if(exprUnary()){
		return true;
	}
	return false;
}

// exprUnary: ( SUB | NOT ) exprUnary | exprPostfix
bool exprUnary(){
	if(consume(SUB)){
		if(exprUnary()){
			return true;
		}else tkerr("missing expression after unary -");
	}
	if(consume(NOT)){
		if(exprUnary()){
			return true;
		}else tkerr("missing expression after !");
	}
	if(exprPostfix()){
		return true;
	}
	return false;
}

/* exprPostfix: exprPostfix LBRACKET expr RBRACKET | exprPostfix DOT ID | exprPrimary
	->	exprPostfix: exprPrimary exprPostfixPrim
		exprPostfixPrim: LBRACKET expr RBRACKET exprPostfixPrim 
						| DOT ID exprPostfixPrim | epsilon
 */
bool exprPostfix(){
	if(exprPrimary()){
		if(exprPostfixPrim()){
			return true;
		}
	}
	return false;
}

bool exprPostfixPrim(){
	if(consume(LBRACKET)){
		if(expr()){
			if(consume(RBRACKET)){
				if(exprPostfixPrim()){
					return true;
				}
			}else tkerr("missing ] after array index");
		}else tkerr("missing expression inside []");
	}
	if(consume(DOT)){
		if(consume(ID)){
			if(exprPostfixPrim()){
				return true;
			}
		}else tkerr("missing field name after .");
	}
	return true;
}

// exprPrimary: ID ( LPAR ( expr ( COMMA expr )* )? RPAR )? | INT | DOUBLE | CHAR | STRING | LPAR expr RPAR
bool exprPrimary(){
	Token *start = iTk;
	// ID sau apel de functie
	if(consume(ID)){
		if(consume(LPAR)){
			if(expr()){
				while(consume(COMMA)){
					if(!expr()){
						tkerr("missing expression after ,");
					}
				}
			}
			if(consume(RPAR)){
				return true;
			}else tkerr("missing ) after function call");
		}
		return true;
	}
	// constante
	if(consume(INT)){ return true; }
	if(consume(DOUBLE)){ return true; }
	if(consume(CHAR)){ return true; }
	if(consume(STRING)){ return true; }
	// (expr)
	if(consume(LPAR)){
		if(expr()){
			if(consume(RPAR)){
				return true;
			}else tkerr("missing )");
		}
	}
	iTk = start;
	return false;
}