# ParalelGeneticAlgorithm
[EN]

C algorithm that solves a backpack problem (trying to fit as much objects as you can). This algorithm contains a selection method which selects the best chromosomes, a crossover function which combines chromosomes for the next generation and a mutation method which switches bits.
The algorithm returns the last generation created which is the best way of including stuff into the backpack.
The algorothm contains paralelism using Pthreds. Methods like crossover and selection can be done in paralel using syncronasation elements like barrier (this ensures the integrity of the parallelization).

[RO]

~ Descriere pe scurt:

Pentru paralelizarea algoritmului am refacut functia run_genetic_algorithm astfel incat aceasta sa fie executata pe N threaduri. Mi-am creat un typedef \_th cu ajutorul
caruia am trimis adrese ale variabilelor necesare in functia de thread create. Dupa pornirea threadurilor m-am ajutat de o bariera care sa opreasca threadurile atunci 
cand intreaga generatie este nevoita sa primeasca o modificare (de exemplu la sortari, la crossover sau atunci cand aflu fitnessul). Ca algoritmul sa scaleze am folosit 
merge sort dupa algoritmul facut la laborator (in care am utilizat aceasi bariera exemplificata mai sus).

~ Observatii:
	- am mutat in mainul scriptului tema1_par.c initializarea generatie curente si cea urmatoare + o noua generatie copie utilizata pentru mergesort, este de mentionat
		ca am marit capacitatera generatiilor pana la urmatoarea putere a lui 2 mai mare ca object count(conditie necesara pentru merge sort).
	- am adaugat in functia read_input si variabila nr_of_threads pe care o citesc.
	- am modificat pe langa run_genetic_algorithm si functia compute_finess_function (care mai nou calculeaza doar pentru un anumit nr de val din generatie,
		mai exact de la start pana la end).
	- principiul principal de paralelizare folosit este de a impartii munca structurilor repetitive pe fiecare thread de la o valoare start pana la o valoare end,
		calculate in main.

~Comentarii pe baza codului:

	genetic_algorithm.h: (am adaugat structura de date transmisa ca parametru pentru functia de creare a threadurilor in functia de executat). liniile[10-23].
	
	typedef struct _th
       {
	sack_object *objects;     \
	int *object_count;         \ variabilele din schelet (atribuite prin adresa).
	int *generations_count;    /
	int *sack_capacity;       /
	long start;                 //indexul de start in for al fiecarui thread
	long end;		    //indexul de end in for al fiecarui thread
	int index;	            //indexul threadului
	int *nr_of_threads;	    //nr de threaduri utilizate
	int *power_of_two;	    //2 la puterea x ai x^2 >= object_count && x cat de mic
	individual *current_generation;		//generatia curenta
	individual *next_generation;		//generatia urmatoare
	individual *copy_generation;		//copie generatie utila in mergesort
	pthread_barrier_t *barrier;		//bariera utilizata pentru sincronizare
       } thread_input;

	tema1_par.c: 
		*adaug declaratia lui nr_of_threads = 0 si power_of_two = 1;
		linia[31]: formez power_of_two dupa regula de mai sus;
		liniile[33-35]: declar current gen next gen si copy gen;
		liniile[37-43]: declar bariera, o initializez; declar un vector de threaduri si un vector de thread_input (typedeful de mai sus);
		liniile[45-60]: dau valori elementelor vectorului de structura pentru fiecare thread in parte;
		liniile[62-70]: dau create iar apoi join ce cele nr_of_thread threaduri;

	genetic_algorithm.c:
		
		~read_input (..): linia[35] dau valoarea citita lui nr_of_threads. parametru pe care il introduc in antetul functiei;
		
		~comute_fitness_function(..) : linia[96] modific structura repetitiva sa mearca din Start ----> End ,deoarece fiecare thread va apela aceasta functie si va
			calcula doar un procent din total. Evident antetul functiei este modificat (adaug start,end si scot object_count).

		~merge_sort(individual *generation, int start, int mid, int end, individual *copy_generation) : functie necesara care resorteaza vectorul , copiindul in copy_generation.
		
		~run_genetic_algorithm(void*arg) : 
			liniile[216-228]: setarea gen init se face in paralel de la start->end pe fiecare thread;
			struct repetitica pentru nr de gen:
				linia[237]: se apeleaza comute_fitness_function(..) explicata mai sus.
				linia[238]: se asteapta pana cand toate threadurile fac aceasta operatie (este vital pentru urmatoarea operatie).
				liniile[240-258]: merge-sort paralel + modificarea start-end pentru parcurgerea unei puteri ale lui 2.
				liniile[260-300]: se face copierea si mutarea generatiei; toate operatiunile din for-uri se pot paraleliza cu start->end avand grija
						sa transformam aceste valori pentru a parcurge doar 20 sau 30 % din totalul de elemente.
				liniile[303-311]: se face crossoverul in paralel, cu urmatoarea specificatie, daca dupa impartirea in start->end valoarea end este impara
						atunci valoarea start impara se incrementeaza ca intervalele de imperechere sa fie pare (astfel evitam ca un membru al unei
						generatii sa ramana pe afara chiar daca are pereche). DUPA ACEASTA OPERATIE DE CROSSOVER SE PUNE BARIERA, deoarece in liniile
						[313-330] se schimba generatia (operatie tot paralela).
			liniile[336-353]: merge-sort paralel final.		 
