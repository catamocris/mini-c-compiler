#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "../utils/utils.h"
#include "lexer.h"


Token *tokens = NULL;	// single linked list of tokens
Token *lastTk = NULL;	// the last token in list

int line = 1;	// the current line in the input file


// adds a token to the end of the tokens list and returns it
// sets its code and line
Token *addTk(int code){
	Token *tk = safeAlloc(sizeof(Token));
	tk->code = code;
	tk->line = line;
	tk->next = NULL;
	if(lastTk){
		lastTk->next = tk;
	}else{
		tokens = tk;
	}
	lastTk = tk;
	return tk;
}


char *extract(const char *begin, const char *end){
	int n = end - begin;
	char *text = safeAlloc((n+1) * sizeof(char));
	memcpy(text, begin, n);
	text[n] = '\0';
	return text;
}


Token *tokenize(const char *pch){
	const char *start;
	Token *tk;
	for(;;){
		switch(*pch){
			case ' ': case '\t': pch++; break;
			case '\r':		// handles different kinds of newlines (Windows: \r\n, Linux: \n, MacOS, OS X: \r or \n)
				if(pch[1] == '\n') pch++;	// fallthrough to \n
			case '\n': line++; pch++; break;
			case '\0': addTk(END); return tokens;
			case ',': addTk(COMMA); pch++; break;
			case ';': addTk(SEMICOLON); pch++; break;
			case '(': addTk(LPAR); pch++; break;
			case ')': addTk(RPAR); pch++; break;
			case '[': addTk(LBRACKET); pch++; break;
			case ']': addTk(RBRACKET); pch++; break;
			case '{': addTk(LACC); pch++; break;
			case '}': addTk(RACC); pch++; break;
			case '+': addTk(ADD); pch++; break;
			case '-': addTk(SUB); pch++; break;
			case '*': addTk(MUL); pch++; break;
			case '/': 
				if(pch[1] == '/'){
					pch += 2;
					while(*pch != '\0' && *pch != '\n' && *pch != '\r') {pch++;}
				}
				else if(pch[1] == '*'){
					pch += 2;
					for(;;){
						if(*pch == '\0') err("unterminated block comment");
						if(*pch == '\r'){
							if(pch[1] == '\n') pch++;
							line++;
							pch++;
							continue;
						}
						if(*pch == '\n'){
							line++;
							pch++;
							continue;
						}
						if(*pch == '*' && pch[1] == '/'){
							pch += 2;
							break;
						}
						pch++;
 					}
				}
				else {addTk(DIV); pch++;}
				break;
			case '.': addTk(DOT); pch++; break;
			case '&':
				if(pch[1] == '&') {addTk(AND); pch+=2;} 
				else {err("invalid character: &");}
				break;
			case '|':
				if(pch[1] == '|') {addTk(OR); pch+=2;}
				else {err("invalid character: |");}
				break;
			case '=':
				if(pch[1] == '=') {addTk(EQUAL); pch+=2;}
				else {addTk(ASSIGN); pch++;}
				break;
			case '!':
				if(pch[1] == '=') {addTk(NOTEQ); pch+=2;}
				else {addTk(NOT); pch++;}
				break;
			case '<':
				if(pch[1] == '=') {addTk(LESSEQ); pch+=2;}
				else {addTk(LESS); pch++;}
				break;
			case '>':
				if(pch[1] == '=') {addTk(GREATEREQ); pch+=2;}
				else {addTk(GREATER); pch++;}
				break;
			case '\'':
			{
				if(pch[1] == '\'') err("empty char constant");
				char c;
				if(pch[1] == '\\') {
					switch(pch[2]) {
						case 'a': c = '\a'; break;
						case 'b': c = '\b'; break;
						case 'f': c = '\f'; break;
						case 'n': c = '\n'; break;
						case 'r': c = '\r'; break;
						case 't': c = '\t'; break;
						case 'v': c = '\v'; break;
						case '\\': c = '\\'; break;
						case '\'': c = '\''; break;
						case '\"': c = '\"'; break;
						case '0': c = '\0'; break;
						default: err("invalid escape sequence in char constant");
					}
					if(pch[3] != '\'') err("missing ' at the end of char constant");
					tk = addTk(CHAR);
					tk->c = c;
					pch += 4;
				}
				else {
					if(pch[2] != '\'') err("missing ' at the end of char constant");
					c = pch[1];
					tk = addTk(CHAR);
					tk->c = c;
					pch += 3;
				}
				break;
			}
			case '\"':
			{
				char *text = safeAlloc(strlen(pch) + 1);
				int i = 0;
				char c;
				pch++;	// jump over "
				for(;;){
					if(*pch == '\0') err("missing \" at the end of the string");
					if(*pch == '"') {
						text[i] = '\0';
						tk = addTk(STRING);
						tk->text = text;
						pch++;
						break;
					}
					if(*pch == '\\'){
						pch++;
						switch(*pch) {
							case 'a': c = '\a'; break;
							case 'b': c = '\b'; break;
							case 'f': c = '\f'; break;
							case 'n': c = '\n'; break;
							case 'r': c = '\r'; break;
							case 't': c = '\t'; break;
							case 'v': c = '\v'; break;
							case '\\': c = '\\'; break;
							case '\'': c = '\''; break;
							case '\"': c = '\"'; break;
							case '0': c = '\0'; break;
							default: err("invalid escape sequence in string");
						}
						text[i] = c;
						i++;
						pch++;
					}
					else {
						text[i] = *pch;
						i++;
						pch++;
					}
				}
				break;
			}
			default:
				if(isalpha(*pch) || *pch=='_') {
					for(start=pch++; isalnum(*pch) || *pch=='_'; pch++){}
					char *text = extract(start, pch);
					if(strcmp(text,"char") == 0) addTk(TYPE_CHAR);
					else if(strcmp(text, "double") ==0) addTk(TYPE_DOUBLE);
					else if(strcmp(text, "else") ==0) addTk(ELSE);
					else if(strcmp(text, "if") ==0) addTk(IF);
					else if(strcmp(text, "int") ==0) addTk(TYPE_INT);
					else if(strcmp(text, "return") ==0) addTk(RETURN);
					else if(strcmp(text, "struct") ==0) addTk(STRUCT);
					else if(strcmp(text, "void") ==0) addTk(VOID);
					else if(strcmp(text, "while") ==0) addTk(WHILE);
					else{
						tk = addTk(ID);
						tk->text = text;
					}
				}
				else if(isdigit(*pch)){
					start = pch;
					for(pch++; isdigit(*pch); pch++){}
					int isDouble = 0;

					if(*pch == '.'){
						if(isdigit(pch[1])){
							isDouble = 1;
							pch++;
							while(isdigit(*pch)) pch++;
						}
						else err("at least one digit required after decimal point");
					}

					if(*pch == 'e' || *pch == 'E'){
						isDouble = 1;
						pch++;
						if(*pch == '+' || *pch == '-') pch++;
						if(isdigit(*pch)){
							while(isdigit(*pch)) pch++;
						}
						else err("at least one digit required in exponent");
					}

					char *text = extract(start, pch);

					if(isDouble){
						tk = addTk(DOUBLE);
						tk->d = atof(text);
					}
					else{
						tk = addTk(INT);
						tk->i = atoi(text);
					}
				}

				else err("invalid char: %c (%d)",*pch,*pch);
		}
	}
}

void showTokens(const Token *tokens){
	for(const Token *tk=tokens; tk; tk=tk->next){
		printf("\n");
		switch(tk->code){
			case 0: printf("ID:%s", tk->text); break;
			case 1: printf("INT:%d", tk->i); break;
			case 2: printf("DOUBLE:%f", tk->d); break;
			case 3: printf("CHAR:%c", tk->c); break;
			case 4: printf("STRING:%s", tk->text); break;
			case 5: printf("TYPE_CHAR"); break;
			case 6: printf("TYPE_DOUBLE"); break;
			case 7: printf("ELSE"); break;
			case 8: printf("IF"); break;
			case 9: printf("TYPE_INT"); break;
			case 10: printf("RETURN"); break;
			case 11: printf("STRUCT"); break;
			case 12: printf("VOID"); break;
			case 13: printf("WHILE"); break;
			case 14: printf("COMMA"); break;
			case 15: printf("SEMICOLON"); break;
			case 16: printf("LPAR"); break;
			case 17: printf("RPAR"); break;
			case 18: printf("LBRACKET"); break;
			case 19: printf("RBRACKET"); break;
			case 20: printf("LACC"); break;
			case 21: printf("RACC"); break;
			case 22: printf("END"); break;
			case 23: printf("ADD"); break;
			case 24: printf("SUB"); break;
			case 25: printf("MUL"); break;
			case 26: printf("DIV"); break;
			case 27: printf("DOT"); break;
			case 28: printf("AND"); break;
			case 29: printf("OR"); break;
			case 30: printf("NOT"); break;
			case 31: printf("ASSIGN"); break;
			case 32: printf("EQUAL"); break;
			case 33: printf("NOTEQ"); break;
			case 34: printf("LESS"); break;
			case 35: printf("LESSEQ"); break;
			case 36: printf("GREATER"); break;
			case 37: printf("GREATEREQ"); break;
			default: printf("unknown token code: %d", tk->code);
		}
	}
	printf("\n");
}
