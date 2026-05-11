#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include "parser.h"
#include "../utils/utils.h"
#include "../ad/ad.h"
#include "../at/at.h"

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

bool expr(Ret *r);
bool exprAssign(Ret *r);
bool exprOr(Ret *r);
bool exprOrPrim(Ret *r);
bool exprAnd(Ret *r);
bool exprAndPrim(Ret *r);
bool exprEq(Ret *r);
bool exprEqPrim(Ret *r);
bool exprRel(Ret *r);
bool exprRelPrim(Ret *r);
bool exprAdd(Ret *r);
bool exprAddPrim(Ret *r);
bool exprMul(Ret *r);
bool exprMulPrim(Ret *r);
bool exprCast(Ret *r);
bool exprUnary(Ret *r);
bool exprPostfix(Ret *r);
bool exprPostfixPrim(Ret *r);
bool exprPrimary(Ret *r);


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

// IF - conditia trebuie sa fie scalar
// WHILE - conditia trebuie sa fie scalar
// RETURN - expresia trebuie sa fie scalar
// RETURN - functiile void nu pot returna o valoare
// RETURN - functiile non-void trebuie sa aiba o expresie returnata, a carei tip sa fie convertibil la tipul returnat de functie

bool stm(){
	Token *start = iTk;
	Ret rCond, rExpr;
	if(stmCompound(true)){
		return true;
	}
	if(consume(IF)){
		if(consume(LPAR)){
			if(expr(&rCond)){
				if(!canBeScalar(&rCond)) tkerr("the if condition must be a scalar value");
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
			if(expr(&rCond)){
				if(!canBeScalar(&rCond)) tkerr("the while condition must be a scalar value");
				if(consume(RPAR)){
					if(stm()){
						return true;
					}else tkerr("missing statement after while");
				}else tkerr("missing ) after while");
			}else tkerr("invalid condition in while");
		}else tkerr("missing ( after while");
	}
	if(consume(RETURN)){
		if(expr(&rExpr)){
			if(owner->type.tb == TB_VOID) tkerr("a void function cannot return a value");
			if(!canBeScalar(&rExpr)) tkerr("the return value must be a scalar value");
			if(!convTo(&rExpr.type, &owner->type)) tkerr("cannot convert the return expression type to the function return type");
		}else{
			if(owner->type.tb != TB_VOID) tkerr("a non-void function must return a value");
		}
		if(consume(SEMICOLON)){
			return true;
		}else tkerr("missing ; after return");
	}
	if(expr(&rExpr)){
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
bool expr(Ret *r){
	if(exprAssign(r)){
		return true;
	}
	return false;
}

// exprAssign: exprUnary ASSIGN exprAssign | exprOr

// Destinatia trebuie sa fie left-value
// Destinatia nu trebuie sa fie constanta
// Ambii operanzi trebuie sa fie scalari
// Sursa trebuie sa fie convertibila la destinatie
// Tipul rezultat este tipul sursei

bool exprAssign(Ret *r){
	Token *start = iTk;
	Ret rDst;
	if(exprUnary(&rDst)){
		if(consume(ASSIGN)){
			if(exprAssign(r)){
				if(!rDst.lval) tkerr("the assign destination must be a left-value");
				if(rDst.ct) tkerr("the assign destination cannot be constant");
				if(!canBeScalar(&rDst)) tkerr("the assign destination must be scalar");
				if(!canBeScalar(r)) tkerr("the assign source must be scalar");
				if(!convTo(&r->type, &rDst.type)) tkerr("the assign source cannot be converted to destination");
				r->lval = false;
				r->ct = true;
				return true;
			}else tkerr("missing expression after =");
		}
	}
	iTk = start;
	if(exprOr(r)){
		return true;
	}
	return false;
}

/* exprOr:  exprOr OR exprAnd | exprAnd
	->	exprOr: exprAnd exprOrPrim
		exprOrPrim: OR exprAnd exprOrPrim | epsilon */

// Ambii operanzi trebuie sa fie scalari si sa nu fie structuri
// Rezultatul este un int

bool exprOr(Ret *r){
	if(exprAnd(r)){
		if(exprOrPrim(r)){
			return true;
		}
	}
	return false;
}

bool exprOrPrim(Ret *r){
	if(consume(OR)){
		Ret right;
		if(exprAnd(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand type for ||");
			*r = (Ret){{TB_INT, NULL, -1}, false, true};
			if(exprOrPrim(r)){
				return true;
			}
		}else tkerr("missing expression after ||");
	}
	return true;
}

/* exprAnd: exprAnd AND exprEq | exprEq
	->	exprAnd: exprEq exprAndPrim
		exprAndPrim: AND exprEq exprAndPrim | epsilon */

// Ambii operanzi trebuie sa fie scalari si sa nu fie structuri
// Rezultatul este un int

bool exprAnd(Ret *r){
	if(exprEq(r)){
		if(exprAndPrim(r)){
			return true;
		}
	}
	return false;
}

bool exprAndPrim(Ret *r){
	if(consume(AND)){
		Ret right;
		if(exprEq(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand type for &&");
			*r = (Ret){{TB_INT, NULL, -1}, false, true};
			if(exprAndPrim(r)){
				return true;
			}
		}else tkerr("missing expression after &&");
	}
	return true;
}

/* exprEq: exprEq ( EQUAL | NOTEQ ) exprRel | exprRel
	->	exprEq: exprRel exprEqPrim
		exprEqPrim: ( EQUAL | NOTEQ ) exprRel exprEqPrim | epsilon */

// Ambii operanzi trebuie sa fie scalari si sa nu fie structuri
// Rezultatul este un int

bool exprEq(Ret *r){
	if(exprRel(r)){
		if(exprEqPrim(r)){
			return true;
		}
	}
	return false;
}

bool exprEqPrim(Ret *r){
	if(consume(EQUAL)){
		Ret right;
		if(exprRel(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand type for ==");
			*r = (Ret){{TB_INT, NULL, -1}, false, true};
			if(exprEqPrim(r)){
				return true;
			}
		}else tkerr("missing expression after ==");
	}
	if(consume(NOTEQ)){
		Ret right;
		if(exprRel(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand type for !=");
			*r = (Ret){{TB_INT, NULL, -1}, false, true};
			if(exprEqPrim(r)){
				return true;
			}
		}else tkerr("missing expression after !=");
	}
	return true;
}

/* exprRel: exprRel ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd | exprAdd
	->	exprRel: exprAdd exprRelPrim
		exprRelPrim: ( LESS | LESSEQ | GREATER | GREATEREQ ) exprAdd exprRelPrim | epsilon */

// Ambii operanzi trebuie sa fie scalari si sa nu fie structuri
// Rezultatul este un int

bool exprRel(Ret *r){
	if(exprAdd(r)){
		if(exprRelPrim(r)){
			return true;
		}
	}
	return false;
}

bool exprRelPrim(Ret *r){
	if(consume(LESS)){
		Ret right;
		if(exprAdd(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand type for <");
			*r = (Ret){{TB_INT, NULL, -1}, false, true};
			if(exprRelPrim(r)){
				return true;
			}
		}else tkerr("missing expression after <");
	}
	if(consume(LESSEQ)){
		Ret right;
		if(exprAdd(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand type for <=");
			*r = (Ret){{TB_INT, NULL, -1}, false, true};
			if(exprRelPrim(r)){
				return true;
			}
		}else tkerr("missing expression after <=");
	}
	if(consume(GREATER)){
		Ret right;
		if(exprAdd(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand type for >");
			*r = (Ret){{TB_INT, NULL, -1}, false, true};
			if(exprRelPrim(r)){
				return true;
			}
		}else tkerr("missing expression after >");
	}
	if(consume(GREATEREQ)){
		Ret right;
		if(exprAdd(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand type for >=");
			*r = (Ret){{TB_INT, NULL, -1}, false, true};
			if(exprRelPrim(r)){
				return true;
			}
		}else tkerr("missing expression after >=");
	}
	return true;
}

/* exprAdd: exprAdd ( ADD | SUB ) exprMul | exprMul
	->	exprAdd: exprMul exprAddPrim
		exprAddPrim: ( ADD | SUB ) exprMul exprAddPrim | epsilon */

// Ambii operanzi trebuie sa fie scalari si sa nu fie structuri

bool exprAdd(Ret *r){
	if(exprMul(r)){
		if(exprAddPrim(r)){
			return true;
		}
	}
	return false;
}

bool exprAddPrim(Ret *r){
	if(consume(ADD)){
		Ret right;
		if(exprMul(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand type for +");
			*r = (Ret){tDst, false, true};
			if(exprAddPrim(r)){
				return true;
			}
		}else tkerr("missing expression after +");
	}
	if(consume(SUB)){
		Ret right;
		if(exprMul(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand type for -");
			*r = (Ret){tDst, false, true};
			if(exprAddPrim(r)){
				return true;
			}
		}else tkerr("missing expression after -");
	}
	return true;
}

/* exprMul: exprMul ( MUL | DIV ) exprCast | exprCast 
	->	exprMul: exprCast exprMulPrim
		exprMulPrim: ( MUL | DIV ) exprCast exprMulPrim | epsilon */

// Ambii operanzi trebuie sa fie scalari si sa nu fie structuri
bool exprMul(Ret *r){
	if(exprCast(r)){
		if(exprMulPrim(r)){
			return true;
		}
	}
	return false;
}

bool exprMulPrim(Ret *r){
	if(consume(MUL)){
		Ret right;
		if(exprCast(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand type for *");
			*r = (Ret){tDst, false, true};
			if(exprMulPrim(r)){
				return true;
			}
		}else tkerr("missing expression after *");
	}
	if(consume(DIV)){
		Ret right;
		if(exprCast(&right)){
			Type tDst;
			if(!arithTypeTo(&r->type, &right.type, &tDst)) tkerr("invalid operand type for /");
			*r = (Ret){tDst, false, true};
			if(exprMulPrim(r)){
				return true;
			}
		}else tkerr("missing expression after /");
	}
	return true;
}

// exprCast: LPAR typeBase arrayDecl? RPAR exprCast | exprUnary

// deoarece typeBase si arrayDecl au nevoie de un argument, se adauga acesta 
// t va fi folosit ulterior

// Structurile nu se pot converti
// Tipul la care se converteste nu poate fi structura
// Un array se poate converti doar la alt array
// Un scalar se poate converti doar la alt scalar

bool exprCast(Ret *r){
	Token *start = iTk;
	if(consume(LPAR)){
		Type t;
		Ret op;
		if(typeBase(&t)){
			if(arrayDecl(&t)){}
			if(consume(RPAR)){
				if(exprCast(&op)){
					if(t.tb == TB_STRUCT) tkerr("cannot convert to a struct type");
					if(op.type.tb == TB_STRUCT) tkerr("cannot convert a struct");
					if(op.type.n >= 0 && t.n < 0) tkerr("an array can be converted only to another array");
					if(op.type.n < 0 && t.n >= 0) tkerr("a scalar can be converted only to another scalar");
					*r = (Ret){t, false, true};
					return true;
				}else tkerr("missing expression after cast");
			}else tkerr("missing ) after type in cast");
		}
	}
	iTk = start;
	if(exprUnary(r)){
		return true;
	}
	return false;
}

// exprUnary: ( SUB | NOT ) exprUnary | exprPostfix

// Minus unar si Not trebuie sa aiba un operand scalar
// Rezultatul lui Not este un int

bool exprUnary(Ret *r){
	if(consume(SUB)){
		if(exprUnary(r)){
			if(!canBeScalar(r)) tkerr("unary - must have a scalar operand");
			r->lval = false;
			r->ct = true;
			return true;
		}else tkerr("missing expression after unary -");
	}
	if(consume(NOT)){
		if(exprUnary(r)){
			if(!canBeScalar(r)) tkerr("unary ! must have a scalar operand");
			r->lval = false;
			r->ct = true;
			return true;
		}else tkerr("missing expression after !");
	}
	if(exprPostfix(r)){
		return true;
	}
	return false;
}

/* exprPostfix: exprPostfix LBRACKET expr RBRACKET | exprPostfix DOT ID | exprPrimary
	->	exprPostfix: exprPrimary exprPostfixPrim
		exprPostfixPrim: LBRACKET expr RBRACKET exprPostfixPrim 
						| DOT ID exprPostfixPrim | epsilon */

// Doar un array poate fi indexat
// Indexul in array trebuie sa fie convertibil la int
// Operatorul de selectie a unui camp de structura se poate aplica doar structurilor
// Campul unei structuri trebuie sa existe

bool exprPostfix(Ret *r){
	if(exprPrimary(r)){
		if(exprPostfixPrim(r)){
			return true;
		}
	}
	return false;
}

bool exprPostfixPrim(Ret *r){
	if(consume(LBRACKET)){
		Ret idx;
		if(expr(&idx)){
			if(consume(RBRACKET)){
				if(r->type.n < 0) tkerr("only an array can be indexed");
				Type tInt = {TB_INT, NULL, -1};
				if(!convTo(&idx.type, &tInt)) tkerr("the index is not convertible to int");
				r->type.n = -1;
				r->lval = true;
				r->ct = false;
				if(exprPostfixPrim(r)){
					return true;
				}
			}else tkerr("missing ] after array index");
		}else tkerr("missing expression inside []");
	}
	if(consume(DOT)){
		if(consume(ID)){
			Token *tkName = consumedTk;
			if(r->type.tb != TB_STRUCT) tkerr("a field can only be selected from a struct");
			Symbol *s = findSymbolInList(r->type.s->structMembers, tkName->text);
			if(!s) tkerr("the structure %s does not have a field %s", r->type.s->name, tkName->text);
			*r = (Ret){s->type, true, s->type.n >= 0};
			if(exprPostfixPrim(r)){
				return true;
			}
		}else tkerr("missing field name after .");
	}
	return true;
}

// exprPrimary: ID ( LPAR ( expr ( COMMA expr )* )? RPAR )? | INT | DOUBLE | CHAR | STRING | LPAR expr RPAR

// ID-ul trebuie sa existe in TS
// Doar functiile pot fi apelate
// O functie poate fi doar apelata
// Apelul unei functii trebuie sa aiba acelasi numar de argumente ca si numarul de parametri de la definitia ei
// Tipurile argumentelor de la apelul unei functii trebuie sa fie convertibile la tipurile parametrilor functiei

bool exprPrimary(Ret *r){
	Token *start = iTk;
	// ID sau apel de functie
	if(consume(ID)){
		Token *tkName = consumedTk;
		Symbol *s = findSymbol(tkName->text);
		if(!s) tkerr("undefined id: %s", tkName->text);
		// functie
		if(consume(LPAR)){
			if(s->kind != SK_FN) tkerr("only a function can be called");
			Ret rArg;
			Symbol *param = s->fn.params;
			if(expr(&rArg)){
				if(!param) tkerr("too many arguments in function call");
				if(!convTo(&rArg.type, &param->type)) tkerr("in call, cannot convert the argument type to the parameter type");
				param = param->next;
				while(consume(COMMA)){
					if(expr(&rArg)){
						if(!param) tkerr("too many arguments in function call");
						if(!convTo(&rArg.type, &param->type)) tkerr("in call, cannot convert the argument type to the parameter type");
						param = param->next;
					}else tkerr("missing expression after ,");
				}
			}
			if(consume(RPAR)){
				if(param) tkerr("too few arguments in function call");
				*r = (Ret){s->type, false, true};
				return true;
			}else tkerr("missing ) after function call");
		}
		// ID
		if(s->kind == SK_FN) tkerr("a function can only be called");
		*r = (Ret){s->type, true, s->type.n >= 0};
		return true;
	}
	// constante
	if(consume(INT)){ 
		*r = (Ret){{TB_INT, NULL, -1}, false, true};
		return true; 
	}
	if(consume(DOUBLE)){ 
		*r = (Ret){{TB_DOUBLE, NULL, -1}, false, true};
		return true; 
	}
	if(consume(CHAR)){ 
		*r = (Ret){{TB_CHAR, NULL, -1}, false, true};
		return true; 
	}
	if(consume(STRING)){ 
		*r = (Ret){{TB_CHAR, NULL, 0}, false, true};
		return true; 
	}
	// (expr)
	if(consume(LPAR)){
		if(expr(r)){
			if(consume(RPAR)){
				return true;
			}else tkerr("missing )");
		}
	}
	iTk = start;
	return false;
}