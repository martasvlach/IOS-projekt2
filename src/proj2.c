/**
 * @file proj2.c
 * @author Martin Vlach (xvlach18)
 * @brief IOS proj2 - The senate bus problem
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <signal.h>


#define SEMAPHORE_CISLOVANI_RADKU "/xvlach18_ios_proj2_cilovani_radku"
#define SEMAPHORE_ZASTAVKA_PRISTUP "/xvlach18_ios_proj2_zastavka_pristup"
#define SEMAPHORE_ZASTAVKA_POCET "/xvlach18_ios_proj2_zastavka_pocet"
#define SEMAPHORE_BUS_POCET "/xvlach18_ios_proj2_bus_pocet"
#define SEMAPHORE_BUS_PRIJEL "/xvlach18_ios_proj2_bus_prijel"
#define SEMAPHORE_KONEC "/xvlach18_ios_proj2_konec"
#define SEMAPHORE_MAX "/xvlach18_ios_proj2_max"
#define SEMAPHORE_NASTOUPIL "/xvlach18_ios_proj2_nastoupil"
#define SEMAPHORE_BUS_KONCI "/xvlach18_ios_proj2_bus_dokoncil_cestu"
#define SEMAPHORE_ok "/xvlach18_ios_proj2_vystoupenoUZ"

FILE *out;
// Proměná pro přiřazování ID raiderům
int IdRider = 0;
// Počet procesů riders
int R; 
// Kapacita autobusu
int C;
// Maximální doba, po kterou je generován nový proces rider (0 až 1000 milisekund)
int ART;
// Maximální doba, po kterou autobus simuluje jízdu (0 až 1000 milisekund)
int ABT; 

// Sdílená proměnná pro číslování řádků na výstupu
int *cisloRadku = NULL;
int cisloRadkuID = 0;
sem_t *cisloRadkuSEMAFOR = NULL;

// Sdílená proměnná pro počet RIDERS na zastávce
int *pocetZastavka = NULL;
int pocetZastavkaID = 0;
sem_t *pocetZastavkaSEMAFOR = NULL;

// Sdílená proměnná pro počet RIDERS přecedstovaných
int *pocetMAXBus = NULL;
int pocetMAXBusID = 0;
sem_t *pocetMAXBusSEMAFOR = NULL;


sem_t *zastavka_pristupSEMAFOR = NULL;
sem_t *busPrijelSEMAFOR = NULL; // nastupovani bus
sem_t *konecSEMAFOR = NULL;
sem_t *nastoupilJsemSEMAFOR = NULL; // RIDER dava procesu vedet ze nastoupil
sem_t *busKonciSEMAFOR = NULL;
sem_t *okSEMAFOR = NULL;



void otevriSoubor()
{
	if ((out = fopen("proj2.out", "w+")) == NULL) 
		{
    		fprintf(stderr, "CHYBA - proj2.out se nepodařilo otevřít!\n");
    		exit(-2);
 		}
}

/*
	Funkce která mi nastaví sdílené zdroje a semafory
 */


int nastavZdroje()
{
	int err_code = 0;
//Inicializace proměnné pro číslování řádků	
 	if ((cisloRadkuID = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) == -1)
    {
        err_code = 1; 
    }
    if ((cisloRadku = shmat(cisloRadkuID, NULL, 0)) == NULL)
    {
        err_code = 1;
    }
    *cisloRadku = 0; // počáteční hodnota je jedna 

//Inicializace proměnné pro počet RIDERS na zastávce

 	if ((pocetZastavkaID = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) == -1)
    {
        err_code = 1;
    }
    if ((pocetZastavka = shmat(pocetZastavkaID, NULL, 0)) == NULL)
    {
        err_code = 1;
    }
    *pocetZastavka = 0; // počáteční hodnota je jedna 



    if ((pocetMAXBusID = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) == -1)
    {
        err_code = 1; 
    }
    if ((pocetMAXBus = shmat(pocetMAXBusID, NULL, 0)) == NULL)
    {
        err_code = 1;
    }
    *pocetMAXBus = 0; // počáteční hodnota je jedna 


	//Semafor pro uzamykání sdílené proměnné číslo řádku

	if ((cisloRadkuSEMAFOR = sem_open(SEMAPHORE_CISLOVANI_RADKU, O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED)
    {    
    	err_code = 1;
	}	

	//Semafor pro uzamykání zastávky

	if ((zastavka_pristupSEMAFOR = sem_open(SEMAPHORE_ZASTAVKA_PRISTUP, O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
    {    
    	err_code = 1;
	}
	//Semafor pro uzamykání proměnné počet RIDERS na zastávce	

	if ((pocetZastavkaSEMAFOR = sem_open(SEMAPHORE_ZASTAVKA_POCET, O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED)
    {    
    	err_code = 1;
	}
	
	// Semafor signalizujici prijezd BUS

	if ((busPrijelSEMAFOR = sem_open(SEMAPHORE_BUS_PRIJEL, O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
    {    
    	err_code = 1;
	}


	// Semafor signalizující že se může ukončit hlavní process

	if ((konecSEMAFOR = sem_open(SEMAPHORE_KONEC, O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
    {    
    	err_code = 1;
	}

	// Semafor signalizující nastoupení RIDERA

	if ((nastoupilJsemSEMAFOR = sem_open(SEMAPHORE_NASTOUPIL, O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
    {    
    	err_code = 1;
	}	

	// Semafor signalizující ukončení cesty BUSu

	if ((busKonciSEMAFOR  = sem_open(SEMAPHORE_BUS_KONCI, O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
    {    
    	err_code = 1;
	}	


	// Semafor pro sdílenou proměnnou počet odcedstovaných riders
	if ((pocetMAXBusSEMAFOR = sem_open(SEMAPHORE_MAX, O_CREAT | O_EXCL, 0666, 1)) == SEM_FAILED)
    {    
    	err_code = 1;
	}

	// Semafor počet odcedstovaných riders
	if ((okSEMAFOR = sem_open(SEMAPHORE_ok, O_CREAT | O_EXCL, 0666, 0)) == SEM_FAILED)
    {    
    	err_code = 1;
	}

	return err_code;
}

void uvolniZdroje()
{
	fclose(out); // zavri soubor

	// smazání sdílené proměnné pro číslování řádků
	shmctl(cisloRadkuID, IPC_RMID, NULL);

	// smazání sdílené proměnné pro počet RIDERS na zastávce
	shmctl(pocetZastavkaID, IPC_RMID, NULL);


	// smazání sdílené proměnné pro maxriders
	shmctl(pocetMAXBusID, IPC_RMID, NULL);

	// Ukončení a odlinkování semaforu pro uzamykání proměnné pro číslování řádků	
	sem_close(cisloRadkuSEMAFOR);
	sem_unlink(SEMAPHORE_CISLOVANI_RADKU);	

	// Ukončení a odlinkování semaforu pro zamykání zastávky
	sem_close(zastavka_pristupSEMAFOR);
	sem_unlink(SEMAPHORE_ZASTAVKA_PRISTUP);

	// Ukončení a odlinkování semaforu pro zápis počtu RIDERS na zastávce
	sem_close(pocetZastavkaSEMAFOR);
	sem_unlink(SEMAPHORE_ZASTAVKA_POCET);

	// Ukončení a odlinkování semaforu který signalizuje příjezd busu
	sem_close(busPrijelSEMAFOR);
	sem_unlink(SEMAPHORE_BUS_PRIJEL);

	// Ukončení a odlinkování semaforu který signalizuje že se může ukončit hlavní process
	sem_close(konecSEMAFOR);
	sem_unlink(SEMAPHORE_KONEC);

	// Ukončení a odlinkování semaforu který signalizuje konec jízdy
	sem_close(busKonciSEMAFOR);
	sem_unlink(SEMAPHORE_BUS_KONCI);

	// Semafor pro sdílenou proměnnou počet odcedstovaných
	sem_close(pocetMAXBusSEMAFOR);
	sem_unlink(SEMAPHORE_MAX);

	// Ukončení a odlinkování semaforu který signalizuje převzetí signílu o nastoupení
	sem_close(nastoupilJsemSEMAFOR);
	sem_unlink(SEMAPHORE_NASTOUPIL);

	// Ukončení a odlinkování semaforu který signalizuje převzetí signálu s koncem jízdy
	sem_close(okSEMAFOR);
	sem_unlink(SEMAPHORE_ok);	
}


/**
 * @brief Funkce projde znaky v rětězci a ověří zda se jedná a číslo
 * @param argument Řětězec z pole argv (argument)
 */
void numberCheck(char *argument)
{
	int delka = strlen(argument);

	for (int i = 0; i < delka; i++)
	{
		if (!isdigit(argument[i]))
		{
		fprintf(stderr, "CHYBA : Některý argument není celé číslo\n");
        exit (1);
		}
	}

}

/**
 * @brief Funkce ověří počet argumetů a následně se volá ošetření jednotlivých položek
 * @param argv Pole argumentů
 * @param argc Počet argumentů v poli
 */
void argumentsCheck(char *argv[], int argc)
{
    if (argc != 5)
    {
        fprintf(stderr, "CHYBA : Nebyl zadán požadovaný počet argumentů (4) \n");
        exit (1);
    }

	for (int i =1; i < argc; i++)
	{
		numberCheck(argv[i]);
    }
}


/**
 * @brief Funkce přiřadí do proměnných prvku z pole argumentů a ověří jejich platné hodnoty
 * @param argv Pole argumentů
 */
void setGlobVariables(char *argv[])
{
	R = atoi(argv[1]);
	C = atoi(argv[2]);
	ART= atoi(argv[3]);
	ABT= atoi(argv[4]);

	if ((ABT < 0 || ABT > 1000) || (ART < 0 || ART > 1000) || (R < 0 ) || (C<0))
	{
		fprintf(stderr, "CHYBA : Některý z argumetů není v povoleném rozsahu \n (ABT >= 0 && ABT <= 1000) \n (ART >= 0 && ART <= 1000)\n");
		exit (1);
	}
}


/**
 * @brief Funkce se semaforem na příčítání sdílenné proměnné s počtem RIDERS na zastávce
*/ 
void zCount()
{
	sem_wait(pocetZastavkaSEMAFOR);
	(*pocetZastavka)++ ;
	sem_post(pocetZastavkaSEMAFOR);
}

/*
	Funkce ve které autobus zamyká zastávku a přijíždí
 */

void bus_arrive()
{
	sem_trywait(zastavka_pristupSEMAFOR);
	sem_wait(cisloRadkuSEMAFOR);
	(*cisloRadku)++ ;
	fprintf(out,"%d\t: BUS \t: arrival\n", *cisloRadku);
	sem_post(cisloRadkuSEMAFOR);
	sem_trywait(busKonciSEMAFOR);
}

/*
	Funkce ve které autobus odemyká zastávku a odjíždí
 */
void bus_depart()
{
	sem_post(zastavka_pristupSEMAFOR);
	sem_wait(cisloRadkuSEMAFOR);
	(*cisloRadku)++ ;
	fprintf(out,"%d\t: BUS \t: depart\n", *cisloRadku);
	sem_post(cisloRadkuSEMAFOR);
}

/*
 Funkce pro zjištění kolik RID má BUS nabrat pokud je počet čekajích větší nebo stejný jako
 kapacita, vrátí se kapacita, jinak se vrátí počet RID kteří mají nastoupit do busu
 */

int maxRiders()
{
	if ((*pocetZastavka) >= C)
	{
		return C;
	}

	 return (*pocetZastavka);
}

/**
 * @brief Funkce generuje náhodné číslo (čas) z intervalu 0 až maxTime
 * @param Vrchní strop invervalu pro generování
 * @return Náhodné číslo z intervalu nebo 0	
 */
int timeGenerator(int maxTime)
{
	if ( maxTime == 0)
	{
		return 0;
	}
	int randomTime = (rand() % maxTime);
	return randomTime*1000;
}

/*
 * RIDER PROCESS
 */
void RIDER_process()
{	
	// Raider se vygeneruje a vypíše, že začal při čemž čeká na přístup k sdílenné proměnné na "číslování" řádků

	sem_wait(cisloRadkuSEMAFOR);
	(*cisloRadku)++ ;
	fprintf(out,"%d\t: RID %d \t: start\n", *cisloRadku, IdRider);
	sem_post(cisloRadkuSEMAFOR);


	// Raider se pokusí vstoupit na zastávku, pokud se na ní nenachází autobus vypíše ENTER a počet čekajích na zastávce včetně něj
	// Pokud se na zastávce autobus čeká než odjede

	sem_wait(zastavka_pristupSEMAFOR);
	zCount();
	sem_wait(cisloRadkuSEMAFOR);
	(*cisloRadku)++ ;
	fprintf(out,"%d\t: RID %d \t: enter : %d \n", *cisloRadku, IdRider, *pocetZastavka);
	sem_post(cisloRadkuSEMAFOR);
	sem_post(zastavka_pristupSEMAFOR);

	// Čeká na zastávce dokud autobus nepřijede, pokusí se nastoupit pokud je ještě místo v buse 
	// Pokud se do busu dostane, dá procesu BUS věděť že nastoupil a vypíše proces RID nastoupil

	sem_wait(busPrijelSEMAFOR);
	sem_wait(cisloRadkuSEMAFOR);
	(*cisloRadku)++ ;
	fprintf(out,"%d\t: RID %d \t: boarding\n", *cisloRadku, IdRider);
	sem_post(cisloRadkuSEMAFOR);
	sem_post(nastoupilJsemSEMAFOR);

	// Proces čeká na ukončení jízdy autobusu dá mu věděť že toto zasnamenal, posílá signál a umírá
	// Vypisuje process RID finish

	sem_wait(busKonciSEMAFOR);
	sem_wait(cisloRadkuSEMAFOR);
	(*cisloRadku)++ ;
	fprintf(out,"%d\t: RID %d \t: finish\n", *cisloRadku, IdRider);
	sem_post(cisloRadkuSEMAFOR);
	sem_post(okSEMAFOR);
	exit(0);

}

/*
 * BUS PROCESS - Převáží RIDERS dokud je všechny nepřeveze
 */	
void BUS_process()
{	
	// BUS začíná a vypisuje že začal 

	sem_wait(cisloRadkuSEMAFOR);
	(*cisloRadku)++ ;
	fprintf(out,"%d\t: BUS \t: start\n", *cisloRadku);
	sem_post(cisloRadkuSEMAFOR);


	// BUS jezdí do doby než převeze všechny RIDERS kteří se mají vygenerovat

	while((*pocetMAXBus) < R) 
	 {
		bus_arrive();
		int max = maxRiders(); // zeptá se kolik RIDERS má nabrat
		if (max != 0) // Pokud na zastavce nekdo ceka tak je naberu
			{ 

	 			sem_wait(cisloRadkuSEMAFOR);
				(*cisloRadku)++ ;
				fprintf(out,"%d\t: BUS \t: start boarding: %d \n", *cisloRadku, *pocetZastavka); // Bus vypíše že začal nabírat RIDERS a jejich momentální počet na zastávce
				sem_post(cisloRadkuSEMAFOR);

				 for (int i = 0; i < max; i++) // BUS nabírá RIDERS dokud nenabere všechny na zastávce, nebo dokud nenaplní kapacitu
	 				{
	 					sem_post(busPrijelSEMAFOR);
	 					sem_wait(nastoupilJsemSEMAFOR);
	 					sem_wait(pocetZastavkaSEMAFOR);
	 					(*pocetZastavka)--;				// Čeká dokud RIDER nanastoupí, a poté sníží čekajících na zastávce
	 					sem_post(pocetZastavkaSEMAFOR);
	 					sem_wait(pocetMAXBusSEMAFOR);
						(*pocetMAXBus)++;				// Zvýší počet lidí, kteří již cestují/vali
						sem_post(pocetMAXBusSEMAFOR);
	 				}

		 		sem_wait(cisloRadkuSEMAFOR);
				(*cisloRadku)++ ;
				fprintf(out,"%d\t: BUS \t: end boarding: %d \n", *cisloRadku, *pocetZastavka); // BUS vypíše, že končí nastupování
				sem_post(cisloRadkuSEMAFOR);
			}
		bus_depart(); // Autobus odjizdi 

		if (ABT != 0)	// Simulace jizdy
			{
				usleep(timeGenerator(ABT)); // bus se nemá uspat pokud je ATB 0
			}

	// Autobus dokončuje jízdu a tiskne, dává možnost se ukončit procesům které jsou v něm	

		sem_wait(cisloRadkuSEMAFOR);
		(*cisloRadku)++ ;
		fprintf(out,"%d\t: BUS \t: end\n", *cisloRadku);
		sem_post(cisloRadkuSEMAFOR);
		for (int n = 0; n < max; n++)
		{
			sem_post(busKonciSEMAFOR); // rekni procesum ze se monou ukoncit
			sem_wait(okSEMAFOR);
		}	
	} // Konec cyklu

	// Autobus již převezl přechny RIDERS a ukončuje se 		

	sem_wait(cisloRadkuSEMAFOR);
	(*cisloRadku)++ ;
	fprintf(out,"%d\t: BUS \t: finish\n", *cisloRadku);
	sem_post(cisloRadkuSEMAFOR);

	// Autobus končí a povoluje semafor v hlavním cyklu

	sem_post(konecSEMAFOR);
	exit (0);
}	

/*
 * Generator PROCESS - generuje processy RIDERS dokud jich nevygeneruje potřebné R
 */	

void GENERATOR_process()
{
	for (int i = 0; i < R; i++ ) 
		{
			IdRider++; 			 // Inkrementace glibální nesdílené proměnné, která mi značí index RID 
			pid_t RIDER = fork();

			if (RIDER < 0)
			{
				fprintf(stderr, "CHYBA : Fork() na process RIDER se nepodařil\n");
				uvolniZdroje();
				exit (-2);
			}

			if ( RIDER == 0 )
			{
				RIDER_process(); // v potomovi poběží RIDER_funkce
			}

			if (ART != 0 )	   // RIDERS se mají generovat v náhodném čase, pokud ART není nula, v tom případě se vygenerují okamžitě
			{
				usleep(timeGenerator(ART)); 
			}	
		}

 	exit(0);
}


int main(int argc, char *argv[]) {
	argumentsCheck(argv, argc); // Zkontrolují se zadané argumenty
	setGlobVariables(argv);		// Přiřadí se hodnoty do globálních proměnných se zadaných argumentů
	otevriSoubor();
	if (nastavZdroje()) 		// Nastaví se zdoje (semafory, sdílená paměť, soubor pro zápis .. )
	{
		fprintf(stderr, "CHYBA : nastala chyba pri alokovani zdroju\n");
		uvolniZdroje();
		exit (-2);
	}				
	setbuf(out, NULL);

	/*
	 	Pokud se jedná o dětský process přesunu se do funkce BUS_process()
	 	Pokud je PID záporné nastává handling_error() - Vypíše se chybová hlášení, uvolní se alokované zdroje a ukončí se 
	 	Pokud je PID kladné jedná se o rodičkovský process následně se forkuje na generator
	*/

	pid_t BUS = fork();

	if ( BUS == 0 )
		{
			BUS_process(); 	
		}

	else if ( BUS < 0 )
		{
			fprintf(stderr, "CHYBA : Fork() na process BUS se nepodařil\n");
			uvolniZdroje();
			return (-1);
		}	

	else 
		{
			pid_t GENERATOR = fork();

			if ( GENERATOR == 0 )
				{
					GENERATOR_process();
				}	

			else if ( GENERATOR < 0 )
				{
					fprintf(stderr, "CHYBA : Fork() na process GENERATOR se nepodařil \n");
					uvolniZdroje();
					return (-1);
				}
		}

	sem_wait(konecSEMAFOR); // Čekání na semafor který se procne těsně před ukončením programu
	uvolniZdroje();			// Uvolnění alokovaných zdrojů
	return 0;
}	