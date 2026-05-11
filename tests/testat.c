struct S{
	int n;
	char text[16];
	};
	
struct S a;
struct S v[10];

void f(char text[],int i,char ch){
	text[i]=ch;
	}

int h(int x,int y){
	if(x>0&&x<y){
		f(v[x].text,y,'#');
		return 1;
		}
	return 0;
	}

void testConditions(){

	// IF - conditia trebuie sa fie scalar
	if(a.n){}
	// if(v){}

	// WHILE - conditia trebuie sa fie scalar
	while(a.n){}
	// while(a.text){}

	// RETURN - functiile void nu pot returna o valoare
	return;
	// return 10;
}

int testReturn(){

	return 10;

	// RETURN - expresia trebuie sa fie scalar
	// return v;

	// RETURN - functiile non-void trebuie sa aiba o expresie returnata
	// return;

	// RETURN - expresia returnata trebuie sa fie convertibila la tipul functiei
	// return a;
}

void testAssign(){
	
	int x;
	x = 'a';

	// Destinatia trebuie sa fie left-value
	// (x + 1) = 10;

	// Destinatia nu trebuie sa fie constanta
	// a.text = x;

	// Ambii operanzi trebuie sa fie scalari
	// x = v;

	// Sursa trebuie sa fie convertibila la destinatie
	// x = a;
}

void testExprOr(){

	int x;
	int y;

	// Ambii operanzi trebuie sa fie scalari si sa nu fie structuri
	// if(v || 1){}
	// if(a || 1){}

	// Rezultatul este un int
	x = (y || 0);
}

void testExprAnd(){

	int x;
	int y;

	// Ambii operanzi trebuie sa fie scalari si sa nu fie structuri
	// if(v && 1){}
	// if(a && 1){}

	// Rezultatul este un int
	x = (y && 1);
}

void testExprEq(){

	int x;
	int y;

	// Ambii operanzi trebuie sa fie scalari si sa nu fie structuri
	// if(v == 0){}
	// if(a != 0){}

	// Rezultatul este un int
	x = (y == 0);
}

void testExprRel(){

	int x;
	int y;

	// Ambii operanzi trebuie sa fie scalari si sa nu fie structuri
	// if(v < 10){}
	// if(a <= 0){}
	// if(v > 10){}
	// if(a >= 0){}

	// Rezultatul este un int
	x = (y > 0);
}

void testExprAdd(){

	int x;
	int y;

	// Ambii operanzi trebuie sa fie scalari si sa nu fie structuri
	x = y + 1;
	// x = v + 1;

	x = h(1,2) - y;
	// x = a - 1;
}

void testExprMul(){

	int x;
	int y;

	// Ambii operanzi trebuie sa fie scalari si sa nu fie structuri
	x = y * 2;
	// x = v * 2;

	x = h(1,2) / 2;
	// x = a / 2;
}

void testExprCast(){

	int x;
	int y;
	x = (int)12.34;

	// Structurile nu se pot converti
	// x = (int)a;

	// Tipul la care se converteste nu poate fi structura
	// x = (struct S)3;

	// Un array se poate converti doar la alt array
	// x = (int)a.text;

	// Un scalar se poate converti doar la alt scalar
	// x = (int[])y;
}

void testExprUnary(){

	int x;

	// Minus unar si Not trebuie sa aiba un operand scalar
	x = -1;
	// x = -v;

	x = !0;
	// x = !a.text;

	// Rezultatul lui Not este un int
	x = !a.n;
}

void testExprPostfix(){

	int x;

	// Doar un array poate fi indexat
	x = a.text[0];
	// x = a.n[0];

	// Indexul in array trebuie sa fie convertibil la int
	x = a.text[x];
	// x = a.text[a];

	// Operatorul de selectie a unui camp de structura se poate aplica doar structurilor
	x = a.n;
	// x = x.n;

	// Campul unei structuri trebuie sa existe
	x = v[0].n;
	// x = v[0].age;
}

void testExprPrimary(){

	int x;

	// ID-ul trebuie sa existe in TS
	x = 1;
	// unknown = 1;

	// Doar functiile pot fi apelate
	f(a.text, 1, '#');
	// x();

	// O functie poate fi doar apelata
	x = h(1,2);
	// x = h;

	// Apelul unei functii trebuie sa aiba acelasi numar de argumente ca si numarul de parametri
	f(a.text, 1, '#');
	// f(a.text, 1);

	h(1,2);
	// h(1,2,3);

	// Tipurile argumentelor de la apelul unei functii trebuie sa fie convertibile la tipurile parametrilor functiei
	f(a.text, 1, '#');
	// f(a, 1, '#');

	x = h(1,2);
	// x = h(a, 2);
}