#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
//#include <string.h>

// Define	costanti
#define	maxNodi	5500			// Massimo numero	di nodi	inseribili nel grafo
#define	nBuddiesMedio	2.9		// Massimo numero	di nodi	buddies
#define	maxTot (int)(maxNodi+maxNodi*nBuddiesMedio)
#define	maxPeso	90000			// 90.000	=	simula l'infinito.
#define	maxLoop	30000			// Massimo numero	di loop	dell'algoritmo (evito	cicli	infiniti)
#define	maxSvolte	2000	 	//	Massimo	numero di	svolte consentite	(archi diversi)	nel	percorso risultante
struct elemento
{
	long nodo;
	float peso;
	long arco_id;
	long codicevia;
	long imp;
	long larghezza;
	struct elemento *next;
};

struct divieti
{
	long node;
	long pred;
	struct divieti *next;
};

struct sbarramento
{
	long node;
	struct sbarramento *next;
};

struct doppiosenso
{
	int node;
	int estratto;
	struct doppiosenso *next;
};

struct coda
{
	float peso;
	long pred;
	long arco_id;
	long codicevia;
	long boolean;
};
// 1=estratto;	0=non estratto.

struct svolte
{
	long arcid;
	long codvia;
};

// Variabili Globali
struct elemento *Auto[maxTot ], *Ped[maxNodi];
// A rappresenta il	grafo	automobilistico, Ped quello	pedonale. Per	il grafo pedonale	non	servono	i	nodi buddies
struct coda B[maxTot ];						//	B	contiene il	cammino	minimo.
struct doppiosenso *bisenso[maxNodi];				//lista	di adiacenze delle strade	a	doppio senso di	circolazione
struct divieti *buddies[maxNodi], *altroArrivo = NULL;
struct sbarramento *sbarrato[maxNodi];
struct svolte percorso[maxSvolte * 4];
int array[maxNodi], statistica[10], cicli[4], fine = 0, var = 0, fs = 0, fs1 = 0;//,esisteSbarramento[maxNodi]					 //	ci dice	per	ogni nodo	quanti sono	i	suoi buddies
long R[maxTot ], struttura[maxTot ], maxbuddies = 0, r = 1, max = 0, ntot = 0, altrapartenza = 0, cont = 0;
//	R contiene i nodi ancora da processare per calcolare il peso	minimo.
float peso[4];
char *Out = "", *outEsteso = "", *forbidden = "", *toOpen = "", forbidden1[30] = "", *stats = "";

FILE *fp1, *fp, *fpi, *fpinv;

int __stdcall calcolaPercorso(long fnode1, long fnode2, long tnode1, long tnode2, int f, int reset, int tipo);
void __stdcall modificaImp(long fnode, long tnode, long arcId, long impedenza, int reset);
long __stdcall impArco(long tnode, long arcId, int reset);
void __stdcall verificaImp(int reset);
void __stdcall ricaricaGrafi(char *toOpen1, char* Out1, char *forbidden1);						//ricarica i grafi
		/*void __stdcall impostaDir	(char	*input,char	*output,char *divieti);*/
void Init(int reset);

//	Crea un	nuovo elemento della lista di adiacienza di	un nodo.
BOOL WINAPI DLLEntryPoit(HINSTANCE hDLL, DWORD dwReason, LPVOID Reserved)
{
	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
			break;
		case DLL_PROCESS_DETACH:
			break;
	}
	return TRUE;
}

struct elemento * CREA_ELEMENTO(long nodo, float peso, long arco_id, long codicevia, long imp, long larghezza)
{
	struct elemento *head = NULL;

	if ((head = (struct elemento*) malloc(sizeof(*head))) == NULL)
	{
		printf("\n MEMORIA INSUFFICIENTE \n");
		exit(1);
	}
	head->next = NULL;
	head->nodo = nodo;
	head->peso = peso;						 //	il peso	è	dato dalla lunghezza
	head->codicevia = codicevia;
	head->arco_id = arco_id;
	head->imp = imp;
	head->larghezza = larghezza;
	return head;
}

struct divieti * CREA_ELEMENTO2(long node, long pred)
{
	struct divieti *head;
	head = NULL;
	if ((head = (struct divieti*) malloc(sizeof(*head))) == NULL)
	{
		printf("\nMEMORIA	INSUFFICIENTE\n");
		exit(1);
	}
	head->next = NULL;
	head->node = node;
	head->pred = pred;
	return head;
}

struct doppiosenso * CREA_ELEMENTO3(long node)
{
	struct doppiosenso *head;
	head = NULL;
	if ((head = (struct doppiosenso*) malloc(sizeof(*head))) == NULL)
	{
		printf("\n MEMORIA INSUFFICIENTE \n");
		exit(1);
	}
	head->next = NULL;
	head->node = node;			// il	peso è dato	dalla	lunghezza
	head->estratto = 0;
	return head;
}

struct sbarramento * CREA_ELEMENTO4(long node)
{
	struct sbarramento *head;
	head = NULL;
	if ((head = (struct sbarramento*) malloc(sizeof(*head))) == NULL)
	{
		printf("\n MEMORIA INSUFFICIENTE \n");
		exit(1);
	}
	head->next = NULL;
	head->node = node;
	return head;
}

// Appende un	nuovo	elemento alla	lista	di adiacenza di	un nodo.
//PERCHE'	NON	GESTISCE anche PED ????
// POSSO METTERE CREA_L_A	e	CREA_L_A2	insieme	,	basta	gestire	max	nel	caso di	L_A
void CREA_L_A(struct elemento *head, long nodo)
{
	struct elemento *temp = NULL;
	if (nodo > max)
		max = nodo;

	if (Auto[nodo] == NULL)				 //	accesso	diretto
		Auto[nodo] = head;
	else
	{
		temp = Auto[nodo]->next;	// attacca	in	testa	 un	nuovo
		Auto[nodo]->next = head;	// nodo	alla lista di	adiacenza
		head->next = temp;
	}
	return;
}

void CREA_L_A2(struct elemento *head, long nodo)
{
	struct elemento *temp = NULL;

	if (Ped[nodo] == NULL)				// accesso diretto
		Ped[nodo] = head;
	else
	{
		temp = Ped[nodo]->next;	 //	attacca	 in	 testa	un nuovo
		Ped[nodo]->next = head;	 //	nodo alla	lista	di adiacenza
		head->next = temp;		 //
	}
	return;
}

void CREA_L_A3(struct doppiosenso *head, long node)
{
	struct doppiosenso *temp = NULL;

	if (bisenso[node] == NULL)				// accesso diretto
		bisenso[node] = head;
	else
	{
		temp = bisenso[node]->next;	 //	attacca	 in	 testa	un nuovo
		bisenso[node]->next = head;	 //	nodo alla	lista	di adiacenza
		head->next = temp;		 //
	}
	return;
}

void CREA_L_A4(struct sbarramento *head, long node)
{
	struct sbarramento *temp = NULL;

	if (sbarrato[node] == NULL)				 //	accesso	diretto
		sbarrato[node] = head;
	else
	{
		temp = sbarrato[node]->next;	// attacca	in	testa	 un	nuovo
		sbarrato[node]->next = head;	// nodo	alla lista di	adiacenza
		head->next = temp;		 //
	}
	return;
}

// Inizializza la	Coda per il	Cammino	Minimo.
//	x	rappresenta	la sorgente	cioè il	nodo di	partenza (iss=initialize single	source)
void I_S_S(long x)
{
	long i;

	for (i = 1; i < maxTot ; i++)
	{
		B[i].peso = maxPeso;
		B[i].codicevia = 0;
		B[i].arco_id = 0;
		B[i].boolean = 0;
		B[i].pred = 0;
		R[i] = 0;					// tutti i nodi	sono ancora	da processare
	}
	B[x].peso = 0;					// la	sorgente ha	peso 0
	R[1] = x;	// il	nodo da	cui	parto	(nodo	numero 1)	è	la sorgente
}

// Ritorna il	peso dell'arco(x,y).
//	f	determina	il modo	di calcolare il	peso
float PESO(long x, long y, long f, struct elemento *s[])
{
	struct elemento *temp = NULL;
	float peso = 0;

	temp = s[x];									// accesso diretto
	while (temp->nodo != y)
		temp = temp->next;

	if (f == 0)
		peso = temp->peso;
	else if (f == 1)
		peso = temp->peso + temp->imp;
	else if (f == 2)
		peso = (temp->peso / temp->larghezza) + temp->imp;

	return peso;
}

// Funzione	di rilassamento	dei	nodi.
void RELAX(long u, long v, long f, struct elemento *s[], long x)
{
	float z = 0;
	struct elemento *temp = NULL;
	z = PESO(u, v, f, s);
	if (B[v].peso > B[u].peso + z)
	{
		B[v].peso = B[u].peso + z;
		B[v].pred = u;
		temp = s[u];				// accesso diretto
		while (temp->nodo != v)
			temp = temp->next;
		B[v].arco_id = temp->arco_id;
		B[v].codicevia = temp->codicevia;
	}

}

// Estrae	il minimo	dalla	coda B usando	R.

long E_M()			//selezione	il vertice u appartenente	a	V-S	con
{					//la minima	stima	di cammino minimo	ed inserisce
	long i, x = 0;			//u	in S
	float min = maxPeso;
	int trovato = 0;

	for (i = 1; i < r; i++)
	{
		if (B[R[i]].peso < min && B[R[i]].boolean == 0)
		{
			min = B[R[i]].peso;
			x = R[i];
		}
	}
	B[x].boolean = 1;					//estratto
	return x;	 //	se x=0 la	coda è vuota.
}

//PERCHé non C'e'	+	CONTA???
// ricerca uno degli indici	corrispondenti a num
long ricerca(long num)
{
	long i;

	for (i = 1; i <= maxTot ; i++)
		if (struttura[i] == num)
			return i;

	return -1;
}

struct elemento *trovaArco(long nodef, long nodet)
{
	struct elemento *temp = NULL;

	if (nodet >= maxNodi)
		nodet = ricerca(struttura[nodet]);

	temp = Ped[nodef];	 // il	trova	arco può lavorare	sempre sul grafo pedonale
	while (temp->nodo != nodet)
		temp = temp->next;

	return temp;
}

struct elemento *trovaArcoAuto(long nodef, long nodet)
{
	struct elemento *temp = NULL;

	if (nodet >= maxNodi)
		nodet = ricerca(struttura[nodet]);	//e	se ritorna -1	che	succede??
	temp = Auto[nodef];	//	il trova arco	può	lavorare sempre	sul	grafo	pedonale

	while (temp != NULL)
	{
		if (temp->nodo == nodet)
			break;
		else
			temp = temp->next;
	}

	return temp;
}

int verifica(long u, struct divieti *l) //	u	nodo trovato dall'alg. y nodo	richiesto	da ext.	ritorna	0	se insuccesso
{
	while (l != NULL)
		if (l->node == u)
			return 1;
		else
			l = l->next;
	return 0;
}

void cancella(long from, long to)
{
	typedef struct elemento *l;
	l *pred; //*l1;
	struct elemento *t, *temp = NULL; //,*p;

	t = Auto[from];	//oppure	t=(*l1);	//puntatore	al nodo	corrente di	l1
	pred = &(Auto[from]);			//indirizzo	del	campo	del	predecessore
	while (t != NULL)
	{
		if (t->nodo == to)
		{
			*pred = t->next; //va	fatta	eventuale	free di	mem
			break;
		}
		else
			pred = &(t->next);
		t = t->next;
	}
}

void modifica(long var1, long nBuddies, long oldDest, long newDest)
{
	struct elemento *t;
	struct divieti *p;
	int find = 0, i;

	p = buddies[var1];

	for (i = 1; i <= nBuddies; i++)
	{
		t = Auto[p->node];
		while (t != NULL)
		{
			if (t->nodo == oldDest)
			{
				t->nodo = newDest;
				find = 1;
				break;
			}
			else
				t = t->next;
		}
		p = p->next;	//buddy	successivo
	}
}

int notInList(struct divieti *d, long nodo)
{// mi	devo riportare ai	nodi del grafo iniziale	(0...maxNodi): il	divieto	di svolta	è	valido anche per un	buddy
	if (nodo >= maxNodi)
		nodo = ricerca(struttura[nodo]);

	while (d != NULL)
		if (d->node == nodo)
			return 0;
		else
			d = d->next;

	return 1;
}

//PRIMA	SI CHIAMAVA	DUPLICESENSO
//torna	1	(+ effetto collaterale)	se ricerca con successo	(cioè	se l'arco	nodef	nodet	è	a	doppio senso)	else 0
int belong(int fnode, int tonode)
{
	struct doppiosenso *temp;

	temp = bisenso[fnode];
	while (temp != NULL)
	{
		if (temp->node == tonode)
		{
			temp->estratto = 1;
			return 1;
		}
		temp = temp->next;
	}
	return 0;
}

long DIJKSTRA(long ris1, struct divieti *lista, long f, struct elemento *s[])
{
	long u = 0, i = 0, x, y;
	struct elemento *temp = NULL;
	struct divieti *tmp;	//,*t;
	//ricerca	valore associato al	nodo di	start	(ris)	e	fine (ris2)	tramite	il mapping iniziale

	if (var == 1)
	{
		x = ris1;
		printf("\n NODO	INIZIALE X=%d	corrisponde	a	%d ", x, struttura[ris1]);
		fprintf(fpi, "\n	NODO INIZIALE	X=%d corrisponde a %d	", x, struttura[ris1]);
	}
	else
	{
		x = ricerca(ris1);
		printf("\n NODO	INIZIALE X=%d	corrisponde	a	%d ", x, ris1);
		fprintf(fpi, "\n	NODO INIZIALE	X=%d corrisponde a %d	", x, ris1);
	}
	if (lista == NULL)
		fprintf(fpi, "\nlista null\n");

	printf("\n NODO/I	FINALE/I	Y=%d corrisponde a %d	\n", lista->node, struttura[lista->node]);
	fprintf(fpi, "\n	NODO/I FINALE/I	 Y=%d	corrisponde	a	%d \n", lista->node, struttura[lista->node]);
	tmp = lista;

	while (tmp != NULL)
	{
		y = struttura[tmp->node];
		if (x == -1 || y == -1)
		{
			printf("\nERRORE:	il nodo	di partenza	o	quello di	arrivo non appartengono	al mio database\n");
			exit(1);
		}
		tmp = tmp->next;
	}

	I_S_S(x);					//inizializzazione (single source)
	u = E_M();						// il	nodo più vicino	alla sorgente
	r++;

	while ((verifica(u, lista) == 0) && i != maxLoop)
	{
		temp = s[u];
		while (temp != NULL)
		{
			R[r] = temp->nodo;
			r++;
			RELAX(u, temp->nodo, f, s, x);						//rilasso	tutti	gli	archi	uscenti	da u
			temp = temp->next;
		}
		u = E_M();	 //	la minima	stima	di cammino minimo	ed inserisce
					 //	u	in S che è l'ins.	dei	vertici	il cui peso	di
					 //	camm min dalla sorg	s	è	stato	già	determinato
		i++;
	}

	cicli[cont] = i;
	if (i == maxLoop)
		i = -1;
	else
		i = 1;

	fine = u;
	return i;
}

// Ritorna il	percorso minimo	(P_M) e	il suo peso.
float P_M(long x, struct divieti *lista, int f, struct elemento *s[], int ii)
{
	long y, z, i = 0;
	int start;

	start = (ii - 1) * maxSvolte;			// cont	dice il	numero di	soluzioni	alternative	trovate	(da	1	a	4)
	i = DIJKSTRA(x, lista, f, s);
	y = fine;
	z = y;

	if (i == 1)
	{
		while (y != x && y != 0)
		{
			if (B[y].arco_id != 0)	//evita	che	mi stampi	arco_id=0
			{
				percorso[start].arcid = B[y].arco_id;
				percorso[start].codvia = B[y].codicevia;
				start++;
				if (fprintf(fpi, "\"y=%d arc_id=%d\",cod_via=%d\n", y, B[y].arco_id, B[y].codicevia) == 0)
				{
					printf("\n ERRORE	nella	scrittura	del	file di	output\n");
					exit(1);
				}
				printf("\"y=%d arc_id=%d\",cod_via=%d\n", y, B[y].arco_id, B[y].codicevia);
			}
			y = B[y].pred;
		}
		peso[ii - 1] = B[z].peso;
		printf("peso[%d]=%f\n", ii - 1, peso[ii - 1]);
		if (fprintf(fpi, "peso:\n	%f\n\n", B[z].peso) == 0)
		{
			printf("\n ERRORE	nella	scrittura	del	file di	output\n");
			exit(1);
		}
		printf("PESO PERCORSO:\n %f\n", B[z].peso);
		cont++;
		return B[z].peso;	//attenzione va	bene solo	se le	cifre	decimali sono	effettivamente trascurabili
	}
	else //i=-1
	{
		printf("\nImpossibile	calcolare	il percorso	minimo.\n");
		fprintf(fpi, "\nImpossibile calcolare il	percorso minimo.\n");
		cont++;
		return -1;
	}
}

// Riempe il grafo (la struttura Auto) dal file grafo.txt.
void LEGGI_GRAFO()
{
	struct elemento *h;
	struct doppiosenso *h3;
	struct sbarramento *h4;
	long fromnode, tonode, arco_id, codicevia, larghezza, percorrenza, impedenza, indice = 0, conta = 1, var1 = 0, var2 = 0, fnode, tnode;
	float lunghezza = 0;

	if ((fp = fopen(toOpen, "r")) == NULL)
	{
		printf("\nERRORE NELL'APERTURA DEL FILE	DI INGRESSO\n");
		exit(1);
	}

	while ((fs1 = fscanf(fp, "%d;%d;%f;%d;%d;%d;%d;%d", &fromnode, &tonode, &lunghezza, &arco_id, &codicevia, &larghezza, &percorrenza, &impedenza)) != EOF)
		if (fs1 != 8)
		{
			printf("\nERRORE:	nel	file con il	grafo	in input e'	presente una riga	con	formato	errato\n");
			exit(1);
		}
		else
		{
			var1 = ricerca(fromnode);
			if (var1 == -1)	 //non è presente
			{
				struttura[conta] = fromnode;
				ntot++;
				var1 = conta;
				conta++;
			}
			var2 = ricerca(tonode);
			if (var2 == -1)
			{
				struttura[conta] = tonode;
				ntot++;
				var2 = conta;
				conta++;
			}
			fnode = var1;
			tnode = var2;

			if (conta > maxNodi)
			{
				printf("\nERRORE:	nel	file di	ingresso sono	presenti troppi	nodi,	il max è %d\n", maxNodi - 1);
				fclose(fpi);
				exit(1);
			}

			switch (percorrenza)
			{
				case 1:	// strada	senso	unico	da nodeto	a	nodefrom (ok)
				{
					h = CREA_ELEMENTO(fnode, lunghezza, arco_id, codicevia, impedenza, larghezza);
					CREA_L_A(h, tnode);
					h = CREA_ELEMENTO(tnode, lunghezza, arco_id, codicevia, impedenza, larghezza);
					// è un	nodo diverso dal precedente. Non posso usare lo	stesso
					CREA_L_A2(h, fnode);	// grafo pedonale
					h = CREA_ELEMENTO(fnode, lunghezza, arco_id, codicevia, impedenza, larghezza);
					CREA_L_A2(h, tnode);	// grafo pedonale
					break;
				}
				case 2:		// strada	senso	unico	da nodefrom	a	nodeto (ok)
				{
					h = CREA_ELEMENTO(tnode, lunghezza, arco_id, codicevia, impedenza, larghezza);
					CREA_L_A(h, fnode);
					h = CREA_ELEMENTO(tnode, lunghezza, arco_id, codicevia, impedenza, larghezza);
					CREA_L_A2(h, fnode);	// grafo pedonale
					h = CREA_ELEMENTO(fnode, lunghezza, arco_id, codicevia, impedenza, larghezza);
					CREA_L_A2(h, tnode);	// grafo pedonale
					break;
				}
				case 3:					 //	strada doppio	senso	(ok)
				{
					h = CREA_ELEMENTO(fnode, lunghezza, arco_id, codicevia, impedenza, larghezza);
					CREA_L_A(h, tnode);
					h = CREA_ELEMENTO(fnode, lunghezza, arco_id, codicevia, impedenza, larghezza);
					CREA_L_A2(h, tnode);	// grafo pedonale

					h = CREA_ELEMENTO(tnode, lunghezza, arco_id, codicevia, impedenza, larghezza);
					CREA_L_A(h, fnode);
					h = CREA_ELEMENTO(tnode, lunghezza, arco_id, codicevia, impedenza, larghezza);
					CREA_L_A2(h, fnode);	// grafo pedonale

					// memorizzo in	un array di	adiacenze	tutte	le strade	bisex	per	poi	costruire	i	divieti	di inversione	ad u
					h3 = CREA_ELEMENTO3(tnode);
					CREA_L_A3(h3, fnode);
					h3 = CREA_ELEMENTO3(fnode);
					CREA_L_A3(h3, tnode);
					break;
				}
				case 4:	// strada	chiusa nel nodo	from =>	è	a	senso	unico	da to	a	from
				{
					h = CREA_ELEMENTO(fnode, lunghezza, arco_id, codicevia, impedenza, larghezza);
					CREA_L_A(h, tnode);
					h = CREA_ELEMENTO(tnode, lunghezza, arco_id, codicevia, impedenza, larghezza);// è un	nodo diverso dal precedente. Non posso usare lo	stesso
					CREA_L_A2(h, fnode);	// grafo pedonale	devo creare	entrambi gli archi
					h = CREA_ELEMENTO(fnode, lunghezza, arco_id, codicevia, impedenza, larghezza);
					CREA_L_A2(h, tnode);	// grafo pedonale	devo creare	entrambi gli archi

					h4 = CREA_ELEMENTO4(tnode);
					CREA_L_A4(h4, fnode);
					break;
				}
				case 5:	// strada	chiusa nel nodo	to =>	è	a	senso	unico	da nodef a nodet
				{
					h = CREA_ELEMENTO(tnode, lunghezza, arco_id, codicevia, impedenza, larghezza);
					CREA_L_A(h, fnode);
					h = CREA_ELEMENTO(tnode, lunghezza, arco_id, codicevia, impedenza, larghezza);
					CREA_L_A2(h, fnode);	// grafo pedonale	devo creare	entrambi gli archi
					h = CREA_ELEMENTO(fnode, lunghezza, arco_id, codicevia, impedenza, larghezza);
					CREA_L_A2(h, tnode);	// grafo pedonale	devo creare	entrambi gli archi

					h4 = CREA_ELEMENTO4(fnode);
					CREA_L_A4(h4, tnode);
					break;
				}
				case 6:			// strada	chiusa da	entrambi i lati
				{
					// creo	solo l'arco	a	doppio senso per grafo pedonale. Non creo	arco per grafo automobilistico
					h = CREA_ELEMENTO(fnode, lunghezza, arco_id, codicevia, impedenza, larghezza);
					CREA_L_A2(h, tnode);	//grafo	pedonale
					h = CREA_ELEMENTO(tnode, lunghezza, arco_id, codicevia, impedenza, larghezza);
					CREA_L_A2(h, fnode);	 //	grafo	pedonale
					h4 = CREA_ELEMENTO4(fnode);
					CREA_L_A4(h4, tnode);
					h4 = CREA_ELEMENTO4(tnode);
					CREA_L_A4(h4, fnode);
					break;
				}
				default:
				{
					printf("\nERRORE:	nel	file di	ingresso il	parametro	XASTA	assume un	valore non valido\n");
					exit(1);
				}
			}	//end	switch (catturare	il caso	rimanente	=> errore
		}	//end	while
	fclose(fp);

}

void LEGGI_DIVIETI()
{
	struct divieti *d = NULL, *tmp = NULL, *h2;
	struct elemento *p, *temp = NULL, *h, *p1 = NULL;
	struct doppiosenso *temp1 = NULL;
	struct sbarramento *temp2 = NULL;
	char resto[100] = "", numero[10] = "", s[4] = "n", app2[150] = "", lista[150] = "";
	int i = 0, ind, value = 0, indice = 0, g = 0;
	long fromnode, tonode, var1 = 0, var2 = 0, var3 = 0, ret = 0, val, pos = maxNodi;

	if ((fp = fopen(forbidden, "r")) == NULL)
	{
		printf("\nERRORE NELL'APERTURA FILE	DIVIETI	DI SVOLTA	%s\n", forbidden1);
		exit(1);
	}

	while (s[0] != 'n' && s[0] != 'N' && s[0] != 's' && s[0] != 'S')
	{
		printf("vuoi consentire	le inversioni	ad u (s/n) ? ");
		gets(s);
	}
	if ((s[0] == 'n') || (s[0] == 'N'))
	{
		printf("non	ha consentito	le inversioni	ad U\n");
		ind = 0;

		while (forbidden[ind] != '.')
		{
			forbidden1[ind] = forbidden[ind];
			ind++;
		}
		forbidden1[ind] = '\0';
		strcat(forbidden1, "a.txt");
		printf("sto	scrivendo	il file	%s\n", forbidden1);

		if ((fp1 = fopen(forbidden1, "w+")) == NULL)
		{
			printf("\nERRORE NELL'APERTURA FILE	DIVIETI	DI SVOLTA	%s\n", forbidden1);
			exit(1);
		}

		while ((fs = fscanf(fp, "%d;%d;%s", &fromnode, &tonode, &resto)) != EOF)
			if (fs != 3)
			{
				printf("\nERRORE nella lettura del file	dei	divieti	di svolta	%s fs=%d\n", forbidden1, fs);
				exit(1);
			}
			else
			{
				var1 = ricerca(fromnode);
				var2 = ricerca(tonode);
				if (belong(var1, var2))	// side	effect.	va a porre il	campo	estratto=1.	Ha successo	se l'arco	è	a	doppio senso
				{
					if ((fprintf(fp1, "%d;%d;%s;%d\n", fromnode, tonode, resto, fromnode)) == 0)
					{
						printf("\n ERRORE	nella	scrittura	del	file dei divieti %s\n", forbidden1);
						exit(1);
					}
				}
				else if ((fprintf(fp1, "%d;%d;%s\n", fromnode, tonode, resto)) == 0)
				{
					printf("\n ERRORE	nella	scrittura	del	file dei divieti %s\n", forbidden1);
					exit(1);
				}
			}

		for (i = 1; i < maxNodi; i++)
		{	// per le	strade doppio	senso	che	sono rimaste fuori
			temp1 = bisenso[i];
			while (temp1 != NULL)
			{
				if (temp1->estratto == 0)
				{
					if ((fprintf(fp1, "%d;%d;%d\n", struttura[i], struttura[temp1->node], struttura[i])) == 0)
					{
						printf("\n ERRORE	nella	scrittura	del	file dei divieti %s\n", forbidden1);
						exit(1);
					}
				}
				temp1 = temp1->next;
			}	//endwhile

			// per le	strade sbarrate
			temp2 = sbarrato[i]; //	i	è	il nodo	di "arrivo"	in cui è presente	lo sbarramento
			while (temp2 != NULL)
			{
				val = temp2->node;
				temp = Auto[i];	 //	val	è	il nodo	di arrivo
				lista[0] = '\0';
				app2[0] = '\0';
				while (temp != NULL)
				{
					sprintf(app2, "%d", struttura[temp->nodo]);	//Oss.	il valore	ritornato	dalla	sprintf	non	contiene il	terminatore
					strcat(lista, app2);
					temp = temp->next;
					if (temp != NULL)
						strcat(lista, ";");
				}
				lista[strlen(lista)] = '\0';
				if (lista[0] != '\0')	//se non ho	nodi uscenti non devo	aggiungere divieti
					if ((fprintf(fp1, "%d;%d;%s\n", struttura[val], struttura[i], lista)) == 0)
					{
						printf("\n ERRORE	nella	scrittura	del	file dei divieti %s\n", forbidden1);
						exit(1);
					}
				temp2 = temp2->next;
			}	//endwhile
		}	// for

		if (fseek(fp1, 0, SEEK_SET) != 0)
		{
			printf("\nERRORE nell'accesso	al file	delle	inversioni ad	U\n");
			exit(1);
		}
		fp = fp1;
	}	//endif

	// inizia	qui	la fase	di creazione del nuovo grafo con sdoppiamento	nodi (leggo	dal	nuovo	file appena	creato)

	while ((fs = fscanf(fp, "%d;%d;%s", &fromnode, &tonode, &resto)) != EOF)
	{
		if (fs != 3)
		{
			printf("\nERRORE nella lettura del file	dei	divieti	di svolta	%s fs=%d\n", forbidden1, fs);
			exit(1);
		}

		var1 = ricerca(fromnode);
		var2 = ricerca(tonode);
		if (var1 == -1 || var2 == -1)
		{
			printf("\nERRORE:	ricontrollare	il file	dei	divieti, sono	presenti nodi	sconosciuti	%d - %d\n", var1, var2);
			exit(1);
		}

		indice = 0;
		d = NULL;

		while (resto[indice] != '\0')
		{
			g = 0;
			while (resto[indice] != ';' && resto[indice] != '\0')
			{
				numero[g] = resto[indice];
				g = g + 1;
				indice = indice + 1;
			}
			if (resto[indice] != '\0')
				indice = indice + 1;

			numero[g] = '\0';
			g = atoi(numero);	//ho estratto	una	svolta vietata
			var3 = ricerca(g);
			if (var3 == -1)
			{
				printf("\nERRORE: ricontrollare il	file dei divieti,	sono presenti	nodi sconosciuti=%d\n", g);
				exit(1);
			}

			//	costruisco in	d	la lista dei nodi	che	non	posso	essere
			//	raggiunti	considerando la	regola(=riga)	di divieto che sto leggendo	ora.
			//	La lista viene creata	considerando il	grafo	iniziale (var3:	0	...	maxNodi)
			h2 = CREA_ELEMENTO2(var3, 0);
			if (d == NULL)
				d = h2;
			else
			{
				tmp = d->next;
				d->next = h2;
				h2->next = tmp;
			}
		}	 //end while

		array[var2]++;

		// aggiornamento statistiche
		statistica[array[var2]]++;

		if (array[var2] > maxbuddies)
			maxbuddies = array[var2];

		struttura[pos] = tonode;	 //pongo	il nuovo buddy nella prima posizione libera

		p = trovaArco(var1, var2);
		p1 = trovaArcoAuto(var1, var2);
		if (p1 == NULL)
			h2 = CREA_ELEMENTO2(pos, var1);
		else
		{
			h2 = CREA_ELEMENTO2(pos, 0);
			h = CREA_ELEMENTO(pos, p->peso, p->arco_id, p->codicevia, p->imp, p->larghezza);
			CREA_L_A(h, var1);
		}

		if (buddies[var2] == NULL)
			buddies[var2] = h2;
		else
		{
			tmp = buddies[var2]->next;
			buddies[var2]->next = h2;
			h2->next = tmp;
		}	 //	aggiungo il	nuovo	buddy	alla lista (di buddies)

		cancella(var1, var2);
		pos++;

		if (pos > maxTot)
		{
			printf("\nERRORE: sono	stati	creati troppi	nodi buddies (pos=%d). Il	max	e':%d\n", pos - maxNodi,
					maxTot - maxNodi);
			exit(1);
		}

		temp = Auto[var2];	 //grafo automobilistico
		while (temp != NULL)// ricopio per il	nuovo	nodo la	lista	di adiacenze di	quello originale tranne	i	divieti	di svolta
		{
			if (notInList(d, temp->nodo))
			{
				p = trovaArco(var2, temp->nodo);
				h = CREA_ELEMENTO(temp->nodo, p->peso, p->arco_id, p->codicevia, p->imp, p->larghezza);
				CREA_L_A(h, pos - 1);
			}
			temp = temp->next;
		}

		if (array[var1] > 0)
			modifica(var1, array[var1], var2, pos - 1);

	}	//endwhile
	fclose(fp);
}

void __stdcall verificaImp(long reset)
{
	long i = 0;
	struct elemento *temp = NULL;
	FILE * fp;

	Init(reset);
	fp = fopen("c:/progetto/imp.txt", "w");

	for (i = 1; i < maxNodi; i++)
	{
		temp = Ped[i];
		while (temp != NULL)
		{
			if (temp->imp != 0)
				fprintf(fp, "\"%d\"\n", temp->arco_id);
			temp = temp->next;
		}
	}
	fclose(fp);
}

// Cambia	(dinammicamente) il	peso dell'arco . se	reset=0	-> ricarica	la struttura .
// Devo	farlo	in entrambi	i	grafi
// Viene quindi	ad esserci una inconsistenza con il	file grafo.txt (!!!???)	NO,	poichè
// poi posso fare	salva	grafo	(!?).
void __stdcall modificaImp(long fnode, long tnode, long arcId, long impedenza, int reset)
{
	struct elemento *temp = NULL;
	struct divieti *t1, *t2;

	Init(reset);
	temp = Ped[ricerca(tnode)];
	while (temp != NULL)
	{
		if (temp->arco_id == arcId)
		{
			temp->imp = impedenza;
			break;
		}
		temp = temp->next;
	}

	if (temp != NULL)
	{
		temp = Ped[temp->nodo];
		while (temp != NULL)
		{
			if (struttura[temp->nodo] == tnode)
				temp->imp = impedenza;

			temp = temp->next;
		}
	}

	temp = Auto[ricerca(tnode)];
	while (temp != NULL)
	{
		if (temp->arco_id == arcId)
		{
			temp->imp = impedenza;
			break;
		}
		temp = temp->next;
	}

	if (temp != NULL)
	{
		temp = Auto[temp->nodo];
		while (temp != NULL)
		{
			if (struttura[temp->nodo] = tnode)
				temp->imp = impedenza;

			temp = temp->next;
		}
	}

	t1 = buddies[(ricerca(tnode))];
	while (t1 != NULL)
	{
		temp = Auto[t1->node];
		while (temp != NULL)
		{
			if (temp->arco_id == arcId)
			{
				temp->imp = impedenza;
				break;
			}
			temp = temp->next;
		}

		if (temp != NULL)
		{
			temp = Auto[temp->nodo];
			while (temp != NULL)
			{
				if (struttura[temp->nodo] = tnode)
					temp->imp = impedenza;
				temp = temp->next;
			}
		}
		t1 = t1->next;

	}
	temp = Auto[ricerca(fnode)];
	while (temp != NULL)
	{
		if (temp->arco_id == arcId)
		{
			temp->imp = impedenza;
			break;
		}
		temp = temp->next;
	}

	if (temp != NULL)
	{
		temp = Auto[temp->nodo];
		while (temp != NULL)
		{
			if (struttura[temp->nodo] == fnode)
				temp->imp = impedenza;
			temp = temp->next;
		}
	}

	t2 = buddies[(ricerca(fnode))];
	while (t2 != NULL)
	{
		temp = Auto[t2->node];
		while (temp != NULL)
		{
			if (temp->arco_id == arcId)
			{
				temp->imp = impedenza;
				break;
			}
			temp = temp->next;
		}

		if (temp != NULL)
		{
			temp = Auto[temp->nodo];
			while (temp != NULL)
			{
				if (struttura[temp->nodo] = fnode)
					temp->imp = impedenza;

				temp = temp->next;
			}
		}
		t2 = t2->next;
	}
}

// Ritorna l'impedenza dell'arco specificato da	arcId	e	tnode. Eventualmente (reset==0)	ricarica i grafi
// oss.	l'impedenza	viene	letta	dal	grafo	pedonale poichè	contiene tutti gli archi e comunque
// se	un arco	è	presente in	entrambi i grafi (quasi	tutti) l'impedenza è la	stessa.
long __stdcall impArco(long tnode, long arcId, int reset)
{
	struct elemento *temp = NULL;
	long i = -1;

	Init(reset);	 //	eventualmente	ricarico la	struttura

	temp = Ped[ricerca(tnode)];
	while (temp != NULL)
	{
		if (temp->arco_id == arcId)
		{
			i = temp->imp;
			break;
		}
		temp = temp->next;
	}
	return i;
}

void calcolaStatistiche()
{
	int res = 0, i = 7;
	float g = 0, h = 0;
	FILE *stat;

	printf("\n STO STAMPANDO LE	STATISTICHE	SUL	FILE %s\n", stats);
	stat = fopen(stats, "w");
	while (i > 0)
	{
		fprintf(stat, "I	nodi sdoppiati %d	volte	sono %d\n", i, statistica[i] - res);
		h = h + (statistica[i] - res) * i;
		g = statistica[i] + g - res;
		res = statistica[i];
		i--;
	}
	fprintf(stat, "I	nodi sdoppiati 0 volte sono	%d\n", (ntot - ((int) g)));
	fprintf(stat, "nodi tot sdoppiati almeno	una	volta	=%.2f\n", g);
	fprintf(stat, "nodi tot compresi	buddies=%.2f invece	di %d\n", h + ntot, maxTot);
	fprintf(stat, "nodi buddies=%.3f\n", h);
	h = h / maxNodi;
	fprintf(stat, "in media ho	%.4f buddies per ogni	nodo\n", h);
	fprintf(stat, "il max numero	di buddies di	un nodo	e' %d	\n", maxbuddies);
	fprintf(stat, "max	indice usato=%d\n", max);
	fprintf(stat, "i	nodi tot sono	%d\n\n", ntot);
	for (i = 0; i < 4; i++)
		fprintf(stat, "i	cicli	effettuati per trovare la	sol	nro	%d sono	%d\n", i + 1, cicli[i]);
	fclose(stat);
}

long esisteDivieto(long fnode1, long fnode2)	 //ritorna il	vero nodo	di partenza
{
	FILE * fp;
	int indice, trovato = 0, val;
	char resto[70] = "", numero[10] = "";
	long fromnode, tonode, maxN;
	struct elemento *temp;
	struct divieti *b;

	printf("esiste un	divieto	andando	da %d	a	%d ? ", fnode1, fnode2);
	if ((fp = fopen(forbidden1, "r")) == NULL)
	{
		printf("\nERRORE	NELL'APERTURA	FILE DIVIETI DI	SVOLTA %s\n", forbidden1);
		exit(1);
	}
	while (fscanf(fp, "%d;%d;%s", &fromnode, &tonode, &resto) == 3)
	{
		indice = 0;
		if (fromnode == fnode1 && tonode == fnode2)
		{
			printf("si\n");
			val = ricerca(fnode1);	 //val <=maxNodi
			temp = Auto[val];
			while (temp != NULL)
			{
				maxN = maxNodi;
				b = buddies[ricerca(fnode2)];
				while (b != NULL)
				{
					if (b->node == temp->nodo)
					{
						var = 1;
						return b->node;
					}
					else
					{
						printf("b->pred=%d e ricerca(fnode1)=%d	\n", b->pred, ricerca(fnode1));
						if (b->pred == ricerca(fnode1))
						{
							var = 1;
							return b->node;
						}
					}
					b = b->next;
				}	//endwhile
				temp = temp->next;
			}	//endwhile
		}	//	endif

	}	//endwhile
	fclose(fp);
	return fnode2;
}

struct divieti * esisteCollegamento(long tnode1, long tnode2)	//ritorna	lista	di possibili nodi	di arrivo
{
	long n;
	struct elemento *temp;
	struct divieti *b, *tmp = NULL, *h, *lista = NULL;

	//faccio un	ciclo	per	tutti	i	buddies	di tnode1
	n = ricerca(tnode1);
	temp = Auto[n];
	while (temp != NULL)
	{
		if (struttura[temp->nodo] == tnode2)
		{
			h = CREA_ELEMENTO2(n, 0);
			if (lista == NULL)
				lista = h;
			else
			{
				tmp = lista->next;
				lista->next = h;
				h->next = tmp;
			}
		}
		temp = temp->next;
	}

	b = buddies[ricerca(tnode1)];
	while (b != NULL)
	{
		n = b->node;
		temp = Auto[n];
		while (temp != NULL)
		{
			if (struttura[temp->nodo] == tnode2)
			{
				h = CREA_ELEMENTO2(n, 0);
				if (lista == NULL)
					lista = h;
				else
				{
					tmp = lista->next;
					lista->next = h;
					h->next = tmp;
				}
			}
			temp = temp->next;
		}	//endwhile
		b = b->next;
	}	//endwhile

	/*printf("\n LISTA DI	POSSIBILI	NODI FINALI: \n");
	 tmp=lista;
	 while	(tmp !=	NULL)
	 {	printf("%d=%d	",tmp->node,struttura[tmp->node]);
	 tmp=tmp->next;
	 }
	 printf("\n");*/

	return lista;
}

int ricercaSbarramento(long in, long from)
{
	struct sbarramento *temp = NULL;

	fprintf(fpi, "esiste	uno	sbarr	del	tipo %d	%d ", in, from);
	temp = sbarrato[ricerca(in)];
	while (temp != NULL)
		if (temp->node == ricerca(from))
			return 1;
		else
			temp = temp->next;
	fprintf(fpi, " non esiste!");
	return 0;
}

int nonEsistonoArchiUscenti(long fnode, long altro)
{
	struct elemento *temp;
	int ret = 1;

	temp = Auto[ricerca(fnode)];
	while (temp != NULL)
	{
		if (struttura[temp->nodo] != altro)
		{
			ret = 0;
			break;
		}
		temp = temp->next;
	}
	printf("\n	nones(%d,%d) ret=%d\n", fnode, altro, ret);
	return ret;
}

long calcolafnode(long fnode1, long fnode2)	//calcolo il	nodo reale di	partenza dati	i	due	possibli
{
	struct elemento *temp;
	int trovato = 0;
	long ret;
	// fnode1	e	2	come presi da	file grafo di	ingresso
	printf("possibili	nodi di	start: fnode1=%d e fnode2=%d", fnode1, fnode2);

	/*	if (fnode1==fnode2)
	 {	if (nonEsistonoArchiUscenti(fnode1,fnode2))
	 {	temp=Auto[ricerca(fnode1)];
	 if (temp !=	NULL)
	 ret=temp->nodo;

	 else
	 {	printf("\nERRORE:	nodo isolato\n");
	 exit(1);
	 }

	 }
	 else return	fnode1;
	 }*/
	//fnode1 !=	fnode2
// nel caso	in cui ho	una	strada sbarrata	da entrambe	le parti ritorno il	civico più vicino
	/*if (ricercaSbarramento(fnode1,fnode2)	&& ricercaSbarramento(fnode2,fnode1))
	 {	if (Auto[ricerca(fnode1)]==NULL)
	 return fnode2;
	 else return	fnode1;
	 //altrapartenza=esisteDivieto(fnode1,fnode2);
	 }*/
	//else
	if (nonEsistonoArchiUscenti(fnode1, fnode2))
	{
		ret = fnode2;
		if (ricercaSbarramento(fnode2, fnode1))
			return fnode2;
	}
	else if (nonEsistonoArchiUscenti(fnode2, fnode1))
	{
		ret = fnode1;
		if (ricercaSbarramento(fnode1, fnode2))
			return fnode1;
	}
	else //non considero il	caso in	cui	non	esistono nodi	uscenti	da entrambi	i	nodi (arco isolato)
	{
		temp = Auto[ricerca(fnode2)];
		while (temp != NULL)
		{	//printf ("	 el	corrente=%d	\n",temp->nodo);
			if (struttura[temp->nodo] == fnode1)
			{
				trovato = 1;
				break;
			}
			else
				temp = temp->next;
		}	 //endwhile

		if (trovato == 1)	 //fnode1	appartiene ad	Auto[fnode2]
		{
			printf("%d appartiene	ad Auto[%d]\n", fnode1, fnode2);
			if (ricercaSbarramento(fnode1, fnode2))
				ret = fnode2;
			else
			{
				ret = fnode1;
				temp = Auto[ricerca(fnode1)];
				while (temp != NULL)
				{
					if (struttura[temp->nodo] == fnode2)//strada	a	doppio senso	=> provo a partire da	entrambi i nodi
						altrapartenza = esisteDivieto(fnode1, fnode2);
					temp = temp->next;
				}
			}
		}
		else
		{
			printf("strada senso unico o con sbarramento");
			if (ricercaSbarramento(fnode2, fnode1))
				ret = fnode1;
			else
				ret = fnode2;
		}
	}
	printf(" scelgo:%d\n", ret);
	if (ret == fnode1)
		ret = esisteDivieto(fnode2, fnode1);
	else
		ret = esisteDivieto(fnode1, fnode2);
	return ret;
}

struct divieti *calcolatnode(long tnode1, long tnode2)	 // calcolo il	nodo reale di	arrivo
{
	struct elemento *temp;
	struct divieti *h, *lista = NULL, *b, *tmp;
	int trovato = 0;
	long ret, n;

	if (ricerca(tnode1) == -1 || ricerca(tnode2) == -1)
	{
		printf("\n ERRORE: uno o entrambi	i	nodi di	arrivo non appartengono	al mio grafo iniziale.\n");
		exit(1);
	}

	/*	if (tnode1==tnode2)		 //il	nodo di	arrivo è già vincolato.	Posso	arrivare anche in	uno	qualsiasi	dei	buddies
	 {	/*r=ricerca(tnode1);
	 h=CREA_ELEMENTO2(r,0);
	 lista=h;
	 return lista;//	*/
	/*		if (nonEsistonoArchiUscenti(tnode1,tnode2))
	 {	temp=Auto[ricerca(tnode1)];
	 if (temp !=	NULL)
	 ret=temp->nodo;
	 }
	 else
	 {	r=ricerca(tnode1);
	 h=CREA_ELEMENTO2(r,0);
	 lista=h;
	 b=buddies[r];
	 while	(b !=	NULL)
	 {	h2=CREA_ELEMENTO2(b->node,0);
	 tmp=lista->next;
	 lista->next=h2;
	 h2->next=tmp;
	 b=b->next;
	 }

	 //stampa
	 tmp=lista;
	 printf("caso tnode1=tnode2 lista possibili nodi	finali:");
	 while	(tmp !=	NULL)
	 {	printf("%d=%d	",tmp->node,struttura[tmp->node]);
	 tmp=tmp->next;
	 }
	 printf("\n");
	 return lista;
	 }
	 }*/

	/*if (ricercaSbarramento(tnode1,tnode2)	&& ricercaSbarramento(tnode2,tnode1))
	 {		h=CREA_ELEMENTO2(ricerca(tnode1),0);
	 lista=h;
	 h2=CREA_ELEMENTO2(ricerca(tnode2),0);
	 altroArrivo=h2;
	 tmp=lista->next;
	 lista->next=h2;
	 h2->next=tmp;
	 return lista;
	 }*/
	//questo caso	è	da rivedere	così non va	bene si	devono considerare i buddy e capire	bene quello	che	succede	se la	strada è sbarrata	da entrambe	le parti
	//else
	if (nonEsistonoArchiUscenti(tnode1, tnode2))
	{
		ret = tnode2;
		lista = esisteCollegamento(tnode2, tnode1);
		if (lista == NULL)
		{
			printf("\nlista=NULL\n");
			h = CREA_ELEMENTO2(ricerca(tnode2), 0);
			lista = h;
			b = buddies[ricerca(tnode2)];
			while (b != NULL)
			{
				n = b->node;
				//printf("buddy	corrente %d	",n);
				temp = Auto[n];
				while (temp != NULL)
				{
					h = CREA_ELEMENTO2(n, 0);
					tmp = lista->next;
					lista->next = h;
					h->next = tmp;
					temp = temp->next;
				}	//endwhile

				b = b->next;
			}	//endwhile

			// stampa
			tmp = lista;
			while (tmp != NULL)
			{
				printf("%d=%d	", tmp->node, struttura[tmp->node]);
				tmp = tmp->next;
			}
			printf("\n");
			return lista;
		}	//endif
	}
	else if (nonEsistonoArchiUscenti(tnode2, tnode1))
	{
		ret = tnode1;
		lista = esisteCollegamento(tnode1, tnode2);
		if (lista == NULL)
		{
			printf("\n lista=NULL\n");
			h = CREA_ELEMENTO2(ricerca(tnode1), 0);
			lista = h;
			b = buddies[ricerca(tnode1)];
			while (b != NULL)
			{
				n = b->node;
				//printf("buddy	corrente %d	",n);
				temp = Auto[n];
				while (temp != NULL)
				{
					h = CREA_ELEMENTO2(n, 0);
					tmp = lista->next;
					lista->next = h;
					h->next = tmp;
					temp = temp->next;
				}	//endwhile
				b = b->next;
			}	//endwhile
			// stampa
			tmp = lista;
			while (tmp != NULL)
			{
				printf("%d=%d	", tmp->node, struttura[tmp->node]);
				tmp = tmp->next;
			}
			printf("\n");
			return lista;

		}				//endif
	}
	else
	{
		temp = Auto[ricerca(tnode1)];
		while (temp != NULL)
		{
			if (struttura[temp->nodo] == tnode2)
			{
				trovato = 1;
				break;
			}
			else
				temp = temp->next;
		}
		if (trovato == 1)
		{
			ret = tnode1;
			temp = Auto[ricerca(tnode2)];
			while (temp != NULL)
			{
				if (struttura[temp->nodo] == tnode1)
					altroArrivo = esisteCollegamento(tnode2, tnode1);
				temp = temp->next;
			}
		}
		else
			//	è	senso	unico	da tnode2	a	tnode1 =>non esiste	sbarramento	(esistono	solo nei bisensi)
			ret = tnode2;
	}
	printf(" \nscelgo:%d\n", ret);

	if (ret == tnode1)
		lista = esisteCollegamento(tnode1, tnode2);
	else
		lista = esisteCollegamento(tnode2, tnode1);
	return lista;
}

int minimo(float i1, float i2, float i3, float i4)
{
	FILE * fp;
	float mina = 0, minb = 0, min = 0;
	int start, y = 0;

	printf(" minimo	fra	%d %d	%d %d\n", i1, i2, i3, i4);
	if (i1 <= i2 && i1 >= 0)
		mina = i1;
	else if (i2 >= 0)
		mina = i2;
	else
		mina = i1;
	if (i3 <= i4 && i3 >= 0)
		minb = i3;
	else if (i4 >= 0)
		minb = i4;
	else
		minb = i3;
	printf(" mina	%d minb	%d \n", mina, minb);
	if (mina <= minb && mina >= 0)
		min = mina;
	else if (minb >= 0)
		min = minb;
	else
		min = mina;
	if (min == i1)
		start = 0;
	else if (min == i2)
		start = maxSvolte;
	else if (min == i3)
		start = maxSvolte * 2;
	else if (min == i4)
		start = maxSvolte * 3;

	printf("start=%d", start);

	fpinv = fopen("c:/progetto/outInv.txt", "w");

	fprintf(fpi, "start=%d\n", start);
	y = start;
	if ((fp = fopen(Out, "w")) == NULL)
	{
		printf("\nERRORE nella creazione file	di output	%s\n", Out);
		exit(1);
	}
	while (y < start + maxSvolte)
	{
		if (percorso[y].arcid != 0)
		{
			if ((fprintf(fp, "\"%d\",%d\n", percorso[y].arcid, percorso[y].codvia)) == 0)
			{
				printf("\nERRORE nella scrittura del file	di output\n");
				exit(1);
			}
		}
		y++;
	}
	if ((fprintf(fp, "peso\n %f\n", peso[start / maxSvolte])) == 0)	// o lo	elimino	o	faccio il	controllo	di eventuali errori	in scrittura (come sopra)
	{
		printf("\nERRORE nella scrittura del file	di output\n");
		exit(1);
	}

	y = start + maxSvolte;
	while (y >= start)
	{
		if (percorso[y].arcid != 0)
		{
			fprintf(fpinv, "\"%d\",%d\n", percorso[y].arcid, percorso[y].codvia);
		}
		y--;

	}

	fprintf(fpinv, "peso\n	%f\n", peso[start / maxSvolte]);
	fclose(fp);
	fclose(fpinv);
	return (int) min;
}

void inizializzaVariabiliGlobali()
{
	cont = 0;
	var = 0;
	altrapartenza = 0;
	//ntot=0;
	max = 0;
	fs = 0;
	fs1 = 0;
	maxbuddies = 0;
	r = 1;
	fine = 0;
	//stats="";
	outEsteso = "";
	altroArrivo = NULL;
}

void inizializzaArray()
{
	int i;

	for (i = 0; i < maxSvolte * 4; i++)
	{
		percorso[i].arcid = 0;
		percorso[i].codvia = 0;
	}

	for (i = 0; i < maxNodi; i++)
		array[i] = 0;

	for (i = 0; i < 4; i++)
	{
		cicli[i] = 0;
		peso[i] = 0;
	}
}

void inizializzaArrayReset()
{
	int i;

	for (i = 0; i < 10; i++)
		statistica[i] = 0;

	for (i = 0; i < maxNodi; i++)
	{
		if (Ped[i] != NULL)
			Ped[i] = NULL;

		if (bisenso[i] != NULL)
			bisenso[i] = NULL;

		if (buddies[i] != NULL)
			buddies[i] = NULL;

		if (sbarrato[i] != NULL)
			sbarrato[i] = NULL;
	}

	for (i = 0; i < maxTot ; i++)
	{
		if (Auto[i] != NULL)
			Auto[i] = NULL;

		R[i] = 0;

		B[i].peso = maxPeso;
		B[i].pred = 0;
		B[i].boolean = 0;

		struttura[i] = 0;
	}
	ntot = 0;
}

void __stdcall ricaricaGrafi(char *toOpen1, char* Out1, char* forbidden1)
{
	toOpen = toOpen1;
	Out = Out1;
	forbidden = forbidden1;
	inizializzaArrayReset();
	LEGGI_GRAFO();
	LEGGI_DIVIETI();
	inizializzaVariabiliGlobali();
	inizializzaArray();
}

void Init(int reset)
{
	if (reset == 0)				// devo	ricaricare i due grafi
	{
		inizializzaArrayReset();
		printf("sto	creando	il grafo pedonale...");
		LEGGI_GRAFO();
		printf(" ... creato	grafo	pedonale\n");
		printf("sto	creando	il grafo automobilistico...\n");
		LEGGI_DIVIETI();
		printf(" ...creato grafo automobilistico\n");
	}
}

void ricavaRadice(char *Out)
{
	int ind;
	ind = strlen(Out);

	while (Out[ind] != '/')
		ind--;

	outEsteso = (char *) malloc((ind + 1) * (sizeof(*outEsteso)));
	outEsteso[ind + 1] = '\0';
	while (ind >= 0)
	{
		outEsteso[ind] = Out[ind];
		ind--;
	}
	stats = strdup(outEsteso);
	strcat(stats, "stat.txt");
	strcat(outEsteso, "outEsteso.txt");

}

int __stdcall calcolaPercorso(long fnode1, long fnode2, long tnode1, long tnode2, long f, int reset, int tipo, char *toOpen1, char* Out1, char *forbidden1)
{
	long fnode;
	int i = 0;
	float i1 = -1, i2 = -1, i3 = -1, i4 = -1;
	struct divieti *retNodet = NULL, *lista = NULL;

	toOpen = toOpen1;
	Out = Out1;
	forbidden = forbidden1;
	ricavaRadice(Out);

	fpi = fopen(outEsteso, "w");
	Init(reset);

	inizializzaVariabiliGlobali();
	inizializzaArray();

	if (tipo == 0)	// percorso	automobilistico
	{
		fnode = calcolafnode(fnode1, fnode2); //fnode1	e	2	sono i valori	come presenti	nel	file grafo.txt
		retNodet = calcolatnode(tnode1, tnode2);
		printf("\n PERCORSO	AUTOMOBILISTICO: \n");
		if ((retNodet == NULL) || (fnode == NULL))
		{
			fprintf(fpi, "retnode o fnode è null");
			exit(1);
		}

		i1 = P_M(fnode, retNodet, f, Auto, 1);
		if (altroArrivo != NULL)
		{
			printf("\nElaborazione percorso	con	arrivo alternativo\n");
			i2 = P_M(fnode, altroArrivo, f, Auto, 2);
		}
		if (altrapartenza != 0)
		{
			printf("\nElaborazione percorso	con	partenza alternativa\n");
			i3 = P_M(altrapartenza, retNodet, f, Auto, 3);
			if (altroArrivo != NULL)
			{
				printf("\nElaborazione percorso	con	arrivo e partenza	alternativi\n");
				i4 = P_M(altrapartenza, altroArrivo, f, Auto, 4);
			}
		}
		i = minimo(i1, i2, i3, i4);

		if (i != -1)
			printf("\n	Il percorso	minimo ha	costo	%d\n", i);
		else
			printf("\nNon esiste	un collegamento	fra	i	due	punti\n");

	}
	else
	{
		printf("creato grafo \n");	//tipo==1	(pedonale)
		printf("\n PERCORSO	PEDONALE:\n");
		lista = CREA_ELEMENTO2(ricerca(tnode1), 0);
		i1 = P_M(fnode1, lista, f, Ped, 1);	//nel grafo pedonale	lo faccio	arrivare e partire dal nodo	più	vicino
		i = minimo(i1, i2, i3, i4);
	}

	//le statistiche vengono aggiornate	solo se	c'è	il reset che mi
	// ricrea	i	grafi	altrimenti otterrei	dati inconsistenti
	fprintf(fpi, "\n	dopo reset=%d\n", reset);

	if (reset == 0)
		calcolaStatistiche();

	fclose(fpi);

	if (i != -1)
		return 1;
	else
		return i;
}

/*long	main(int argc, char	*argv[])
 {long	fnode,fnode1,fnode2,tnode1,tnode2,reset; //,tnode
 int f,tipo;
 int i=0;
 float i1=-1,i2=-1,i3=-1,i4=-1;
 struct	divieti	*retNodet=NULL,*lista=NULL;//*tmp,*h
 char	*toOpen1,*Out1,*forbidden1;


 fnode1=atoi(argv[1]);
 fnode2=atoi(argv[2]);
 tnode1=atoi(argv[3]);
 tnode2=atoi(argv[4]);
 f=atoi(argv[5]);
 reset=atoi(argv[6]);
 tipo=atoi(argv[7]);
 toOpen1=argv[8];
 Out1=argv[9];
 forbidden1=argv[10];

 toOpen=toOpen1;
 Out=Out1;
 forbidden=forbidden1;
 ricavaRadice(Out);
 fp2	=	fopen("c:/progetto/output2.txt","w");
 fpi=fopen(outEsteso,"w");
 fp9=fopen("c:/progetto/output3.txt","w");
 fprintf(fp9,"\n	prima	init input=%s	output=%s	divieti=%s reset=%d\n",toOpen, Out,	forbidden,reset);

 Init(reset);

 inizializzaVariabiliGlobali();
 inizializzaArray();
 // a questo	punto	ho caricato	i	due	grafi

 if (tipo==0)	// percorso	automobilistico
 {	fnode=calcolafnode(fnode1,fnode2); //fnode1	e	2	sono i valori	come presenti	nel	file grafo.txt
 retNodet=calcolatnode(tnode1,tnode2);
 printf("\n PERCORSO	AUTOMOBILISTICO: \n");
 if (retNodet==NULL)
 fprintf(fpi,"retnode è null");
 else fprintf(fpi,"retnode	non	è	null");
 i1=P_M(fnode,retNodet,f,Auto,1);
 if (altroArrivo	!= NULL)
 {	printf("\nElaborazione percorso	con	arrivo alternativo\n");
 i2=P_M(fnode,altroArrivo,f,Auto,2);
 }
 if (altrapartenza	!= 0)
 {	printf("\nElaborazione percorso	con	partenza alternativa\n");
 i3=P_M(altrapartenza,retNodet,f,Auto,3);
 if (altroArrivo	!= NULL)
 {	printf("\nElaborazione percorso	con	arrivo e partenza	alternativi\n");
 i4=P_M(altrapartenza,altroArrivo,f,Auto,4);
 }
 }
 i=minimo(i1,i2,i3,i4);

 if (i	!= -1) printf("\n	Il percorso	minimo ha	costo	%d\n",i);
 else printf("\nNon esiste	un collegamento	fra	i	due	punti\n");

 }
 else
 {	printf("creato grafo \n");	//tipo==1
 printf("\n PERCORSO	PEDONALE:\n");
 lista=CREA_ELEMENTO2(ricerca(tnode1),0);
 i1=P_M(fnode1,lista,f,Ped,1);	 //nel grafo pedonale	lo faccio	arrivare e partire dal nodo	più	vicino
 i=minimo(i1,i2,i3,i4);
 }

 //le statistiche vengono aggiornate	solo se	c'è	il reset che mi
 // ricrea	i	grafi	altrimenti otterrei	dati inconsistenti
 fprintf(fpi,"\n	dopo reset=%d\n",reset);
 fprintf(fp9,"\n	dopo reset=%d\n",reset);
 printf("\nstat=%s\n",stats);
 if (reset	== 0)
 calcolaStatistiche();
 fclose(fp2);
 fclose(fpi);
 fclose(fp9);
 if (i	!= -1) return	1;
 else
 return i;

 }*/

