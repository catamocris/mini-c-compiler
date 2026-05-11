#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "parser.h"
#include "../utils/utils.h"
#include "../ad/ad.h"

Token *iTk;				// the iterator in the tokens list
Token *consumedTk;		// the last consumed token

Symbol *owner = NULL;	// current symbol owner

bool unit();
bool structDef();
bool varDef();
bool typeBase(Type *t);
bool arrayDecl(Type *t);
bool fnDef();
bool fnParam();
bool stm();
bool stmCompound(bool newDomain);

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
/* 
const char *tkCodeName(int code){
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
	if(!unit()) tkerr("syntax error");
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
// numele structurii trebuie sa fie unic in domeniu
// in interiorul structurii nu pot exista doua variabile cu acelasi nume
bool structDef(){
	Token *start = iTk;
	if(consume(STRUCT)){
		if(consume(ID)){
			Token *tkName = consumedTk;

			if(consume(LACC)){
				Symbol *s = findSymbolInDomain(symTable, tkName->text);
				if(s) tkerr("symbol redefinition: %s", tkName->text);
				s = addSymbolToDomain(symTable, newSymbol(tkName->text, SK_STRUCT));
				s->type.tb = TB_STRUCT;
				s->type.s = s;
				s->type.n = -1;
				pushDomain();
				owner = s;

				while(varDef()){}
				if(consume(RACC)){
					if(consume(SEMICOLON)){
						owner = NULL;
						dropDomain();
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
// numele variabilei trebuie sa fie unic in domeniu
// variabilele de tip vector trebuie sa aiba dimensiunea data (nu se accepta: int v[])
bool varDef(){
	Token *start = iTk;
	Type t;
	if(typeBase(&t)){
		if(consume(ID)){
			Token *tkName = consumedTk;
			
			if(arrayDecl(&t)){
				if(t.n == 0) tkerr("a vector variable must have a specified dimension");
			}
			if(consume(SEMICOLON)){
				Symbol *var = findSymbolInDomain(symTable, tkName->text);
				if(var) tkerr("symbol redefinition: %s", tkName->text);
				var = newSymbol(tkName->text, SK_VAR);
				var->type = t;
				var->owner = owner;
				addSymbolToDomain(symTable, var);

				if(owner) {
					switch(owner->kind) {
						case SK_FN:
							var->varIdx = symbolsLen(owner->fn.locals);
							addSymbolToList(&owner->fn.locals, dupSymbol(var));
							break;
						case SK_STRUCT:
							var->varIdx = typeSize(&owner->type);
							addSymbolToList(&owner->structMembers, dupSymbol(var));
							break;
						default:
							break;
					}
				} else {
					var->varMem = safeAlloc(typeSize(&t));
				}
				return true;
			}else tkerr("missing ; after variable declaration");
		}else tkerr("missing variable name");
	}
	iTk = start;
	return false;
}

// typeBase: TYPE_INT | TYPE_DOUBLE | TYPE_CHAR | STRUCT ID
// daca tipul de baza este o structura, ea trebuie sa fie deja definita anterior
bool typeBase(Type *t){
	t->n = -1;
	if(consume(TYPE_INT)){
		t->tb = TB_INT;
		return true;
	}
	if(consume(TYPE_DOUBLE)){
		t->tb = TB_DOUBLE;
		return true;
	}
	if(consume(TYPE_CHAR)){
		t->tb = TB_CHAR;
		return true;
	}
	if(consume(STRUCT)){
		if(consume(ID)){
			Token *tkName = consumedTk;
			t->tb = TB_STRUCT;
			t->s = findSymbol(tkName->text);
			if(!t->s) tkerr("undefined structure: %s", tkName->text);
			return true;
		}else tkerr("missing struct name");
	}
	return false;
}

// arrayDecl: LBRACKET INT? RBRACKET
bool arrayDecl(Type *t){
	Token *start = iTk;
	if(consume(LBRACKET)){
		if(consume(INT)){
			Token *tkSize = consumedTk;
			t->n = tkSize->i;
		} else {
			t->n = 0;
		}
		if(consume(RBRACKET)){
			return true;
		}else tkerr("missing ] in array declaration");
	}
	iTk = start;
	return false;
}

/* fnDef: ( typeBase | VOID ) ID LPAR 
		( fnParam ( COMMA fnParam )* )? RPAR stmCompound */
// numele functiei trebuie sa fie unic in domeniu
// domeniul local functiei incepe imediat dupa LPAR
// corpul functiei {...} nu defineste un nou subdomeniu in domeniul local functiei
bool fnDef(){
	Token *start = iTk;
	Type t;
	if(typeBase(&t) || consume(VOID)){
		if(consumedTk->code == VOID){
			t.tb = TB_VOID;
			t.n = -1;
		}
		if(consume(ID)){
			Token *tkName = consumedTk;

			if(consume(LPAR)){
				Symbol *fn = findSymbolInDomain(symTable, tkName->text);
				if(fn) tkerr("symbol redefinition: %s", tkName->text);
				fn = newSymbol(tkName->text, SK_FN);
				fn->type = t;
				addSymbolToDomain(symTable, fn);
				owner = fn;
				pushDomain();

				if(fnParam()){
					while(consume(COMMA)){
						if(!fnParam()){
							tkerr("invalid parameter or missing function parameter after ,");
						}
					}
				}

				if(consume(RPAR)){
					if(stmCompound(false)){
						dropDomain();
						owner = NULL;
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
// numele parametrului trebuie sa fie unic in domeniu
// parametrii pot fi vectori cu dimensiune data, dar in acest caz li se sterge dimensiunea ( int v[10] -> int v[] )
bool fnParam(){
	Token *start = iTk;
	Type t;
	if(typeBase(&t)){
		if(consume(ID)){
			Token *tkName = consumedTk;

			if(arrayDecl(&t)){
				t.n = 0;
			}

			Symbol *param = findSymbolInDomain(symTable, tkName->text);
			if(param) tkerr("symbol redefinition: %s", tkName->text);
			param = newSymbol(tkName->text, SK_PARAM);
			param->type = t;
			param->owner = owner;
			param->paramIdx = symbolsLen(owner->fn.params);
			// parametrul este adaugat atat la domeniul curent, cat si la parametrii fn
			addSymbolToDomain(symTable, param);
			addSymbolToList(&owner->fn.params, dupSymbol(param));
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
// corpul compus {...} al instructiunilor defineste un nou domeniu
bool stm(){
	Token *start = iTk;
	if(stmCompound(true)){
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
// se defineste un nou domeniu doar la cerere
bool stmCompound(bool newDomain){
	if(consume(LACC)){
		if(newDomain) pushDomain();
		for(;;){
			if(varDef()){}
			else if(stm()){}
			else break;
		}
		if(consume(RACC)){
			if(newDomain) dropDomain();
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
// deoarece typeBase si arrayDecl au nevoie de un argument, se adauga acesta 
// t va fi folosit ulterior
bool exprCast(){
	Token *start = iTk;
	if(consume(LPAR)){
		Type t;
		if(typeBase(&t)){
			if(arrayDecl(&t)){}
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