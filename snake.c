/* Programarea Calculatoarelor, seria CC
 * Tema2 - Snake
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curses.h>
#include <ctype.h>
#include <sys/select.h>
#include <time.h>
#include <term.h>

#define FOREVER 	        1
#define FOREVER_AND_EVER        1
#define INIT_ROW 	       12
#define INIT_COL 	       20
#define MAX_LENGTH    	      100
#define START_LENGTH           10
#define NAME_LENGTH            25
#define NR_OBS                 30
#define MAX_NR_OBS            150
#define DIR_UP         		0
#define DIR_DOWN       		1
#define DIR_LEFT       		2
#define DIR_RIGHT               3
#define SELECT_ERROR	       -1
#define SELECT_EVENT		1
#define SELECT_NO_EVENT		0
#define S_TO_WAIT 		0
#define MILIS_TO_WAIT      170000
#define KEYBOARD		0
#define LINE_UP                 3
#define OPT_NO_OBS              1
#define OPT_OBS                 2
#define OPT_HIGH                3
#define OPT_HELP		4
#define OPT_QUIT                5

/* curses.h este necesar pentru biblioteca grafică ncurses
 * ctype.h este necesar pentru funcția tolower - man tolower pentru detalii
 * Inițial, steluța se va afla pe ecran la coordonatele (INIT_ROW, INIT_COL) 
 */

typedef struct Snake {
    	int x;
    	int y;
}Snake;

typedef struct Obstacle {
	int x;
	int y;
}Obstacle;


void init_COLORS ();
void init_MAIN_MENU (WINDOW *wnd, int nrows, int ncols);
void init_HELP (WINDOW *wnd, int nrows, int ncols);
void init_SCORE_MENU (WINDOW *wnd, int nrows, int ncols, int score);
void draw_LOGO (int nrows, int ncols);
void draw_ICE_CREAM (int nrows, int ncols);

void build_FENCE (int L_UP, int nrows, int ncols);
void draw_SNAKE (Snake snake[], int length);
void draw_OBSTACLE (Obstacle obs[], int nr_obs, int nrows, int ncols, int L_UP);
void draw_FOOD (int *rand_x, int *rand_y, Snake snake[], int length, int nrows, int ncols, Obstacle obs[], int nr_obs, int L_UP);
void highscore (const char* fileName, int *maxim, char *name_HS, int name_Length);


int main(void)
{
	int y = INIT_ROW, x = INIT_COL;
	int nrows, ncols, i;
	
	char c, name_HS[NAME_LENGTH];
	
	/* Directia sarpelui */
	int direction;  
	
	/* Lungimea sarpelui */
	int length = START_LENGTH;
	
	/* Numarul de obstacole */
	int nr_obs = NR_OBS;
	
	/* Numarul liniei de deasupra 
	 * O modific daca vreau ca ecranul sa se lase mai jos */
	int L_UP = LINE_UP;
	
	/* Coordonatele hranei */
	int foodX, foodY;
	
	/* Verific daca am apasat corect vreo tasta sau directia e buna
	 * fara sa intru in gard sau obstacol
         * Se face 1 daca am apasat */
	int ok = 0; 
	
	/* Daca nu apas pe vreo tasta
	 * Verifica daca am intrat pe o directie anume */
	int ok2 = 0;
	
	int nfds, sel;
	
	/* Daca sarpele se "musca" devine 1 */
	int BITTEN = 0;
	
	/* Daca sarpele intra intr-un obstacol devine 1 */
	int IN_OBS = 0;
	
	/* Scorul jucatorului */
	int score = 0;
	
	/* Viteza sarpelui */
	double speed = MILIS_TO_WAIT;
	
	/* Optiunea din meniul de intrare */
	int option = OPT_NO_OBS;
	
	/* Variabila de pauza */
	int pause = 0;
	
	/* Variabila in care retin CEL MAI MARE SCOR */
	int maxHS = 0;
	
	/* Retine daca directia sarpelui e buna */
	int good_direction = 0;

	/* Folosit pentru a scrie si citi intr-un fisier, Il voi folosi la HIGHSCORE */
	FILE *file;
	file = fopen("highscore.txt", "a+");
	
	/* Pentru a salva in maxHS de la inceput cel mai mare scor din turele precedenta */
	/* Fara acest rand maxHS = 0 la fiecare deschidere a programului */
	highscore("highscore.txt", &maxHS, name_HS, NAME_LENGTH);
	
	/* Inchide fisierul pentru a salva schimbarile */
	fclose(file);
	/* Apoi il redeschide */
	file = fopen("highscore.txt", "a+");
	
	fd_set read_descriptors;
	struct timeval timeout;

	Snake snake[MAX_LENGTH];
	
	Obstacle obs[MAX_NR_OBS];

	/* Se inițializează ecranul; initscr se va folosi în mod obligatoriu */
	WINDOW *wnd = initscr();
	
	/* Se initializeaza folosirea culorilor si perechile de culori */
	init_COLORS();
	 
	/* getmaxyx - este un macro, nu o funcție, așă că nu avem adresă la parametri */
	/* Se va reține în nrows și ncols numărul maxim de linii și coloane */
	getmaxyx(wnd, nrows, ncols);

	/* Funcția select va 'asculta' tastatura
	 * La scrierea unui șir de caractere + apăsarea ENTER, 
	 * funcția select va întoarce o valoare > 0
	 * Notă: pentru programul cu ncurses,
	 * select va întoarce o valoare > 0 atunci când se introduce
	 * un singur caracter 
	 */

	/* Funcția select va asculta 'dispozitivele' aflate în mulțimea read_descriptors
	 * Numărul de elemente din mulțime = nfds = 1
	 * Pentru select, tastatura este identificată prin numărul 0
	 */
	nfds = 1;
	
	/* Se curăță mulțimea de lucru pentru select */
	FD_ZERO(&read_descriptors);
	
	/* Se adaugă tastatura la mulțime */
	FD_SET(KEYBOARD, &read_descriptors);

	/* Funcția select va așteapta evenimente de la tastatură - introducere de șiruri urmate de ENTER
	 * Dacă după un timp (timeout) nu au loc evenimente, atunci se iese din funcție,
	 * iar funcția returnează valoarea 0 
	 * În timeout, specificăm câte secunde (tv_sec) și câte milisecunde (tv_usec)
	 * vrem să așteptăm
	 */
	timeout.tv_sec = S_TO_WAIT;
	timeout.tv_usec = MILIS_TO_WAIT;

	/* Initializez ecranul de inceput MAIN MENU */
	init_MAIN_MENU(wnd, nrows, ncols);

	while(FOREVER_AND_EVER) 
	{
		c = getchar();
		
		/* Pentru ca ecranul sa se actualizeze dupa fiecare apasare de tasta 
		 * Altfel se coloreaza fiecare optiune si ramane colorata */
		wbkgd(wnd, COLOR_PAIR(1));
		
		draw_LOGO(nrows, ncols);
		draw_ICE_CREAM(nrows, ncols);
		
		attron(COLOR_PAIR(10));
		mvaddstr((nrows / 2) - 7, (ncols - 19) / 2, "H@ppy");
		attroff(COLOR_PAIR(10));
		attron(COLOR_PAIR(11));
		mvaddstr((nrows / 2) - 7, (ncols - 19) / 2 + 6, "Sn@ke");
		attroff(COLOR_PAIR(11));
		attron(COLOR_PAIR(13));
		mvaddstr((nrows / 2) - 7, (ncols - 19) / 2 + 12, "v1.01");
		attroff(COLOR_PAIR(13));
		
		if(tolower(c) == 's')
		{
			switch(option)
			{
				case OPT_NO_OBS:
					option = OPT_OBS;
					attron(COLOR_PAIR(7));
					mvaddstr((nrows / 2) + 4, (ncols - 30) / 2, "Start game with obstacles");
					attroff(COLOR_PAIR(7));
					break;

				case OPT_OBS:
					option = OPT_HIGH;
					attron(COLOR_PAIR(7));
					mvaddstr((nrows / 2) + 5, (ncols - 30) / 2, "Highscore");
					attroff(COLOR_PAIR(7));
					break;

				case OPT_HIGH:
					option = OPT_HELP;
					attron(COLOR_PAIR(7));
					mvaddstr((nrows / 2) + 6, (ncols - 30) / 2, "Help");
					attroff(COLOR_PAIR(7));
					break;

				case OPT_HELP:
					option = OPT_QUIT;
					attron(COLOR_PAIR(7));
					mvaddstr((nrows / 2) + 7, (ncols - 30) / 2, "Quit");
					attroff(COLOR_PAIR(7));
					break;
				
				case OPT_QUIT:
					option = OPT_NO_OBS;
					attron(COLOR_PAIR(7));
					mvaddstr((nrows / 2) + 3, (ncols - 30) / 2, "Start game without obstacles");
					attroff(COLOR_PAIR(7));
					break;
			}
			
			refresh();
		}

		if(tolower(c) == 'w')
		{
			switch(option)
			{
				case OPT_NO_OBS:
					option = OPT_QUIT;
					attron(COLOR_PAIR(7));
					mvaddstr((nrows / 2) + 7, (ncols - 30) / 2, "Quit");
					attroff(COLOR_PAIR(7));
					break;

				case OPT_OBS:
					option = OPT_NO_OBS;
					attron(COLOR_PAIR(7));
					mvaddstr((nrows / 2) + 3, (ncols - 30) / 2, "Start game without obstacles");
					attroff(COLOR_PAIR(7));
					break;

				case OPT_HIGH:
					option = OPT_OBS;
					attron(COLOR_PAIR(7));
					mvaddstr((nrows / 2) + 4, (ncols - 30) / 2, "Start game with obstacles");
					attroff(COLOR_PAIR(7));
					break;
		
				case OPT_HELP:
					option = OPT_HIGH;
					attron(COLOR_PAIR(7));
					mvaddstr((nrows / 2) + 5, (ncols - 30) / 2, "Highscore");
					attroff(COLOR_PAIR(7));
					break;
				
				case OPT_QUIT:
					option = OPT_HELP;
					attron(COLOR_PAIR(7));
					mvaddstr((nrows / 2) + 6, (ncols - 30) / 2, "Help");
					attroff(COLOR_PAIR(7));
					break;
			}
			
			refresh();
		}
	
		if(tolower(c) == 'y')
		{
			if(option == OPT_QUIT)
				break;
	
			if(option == OPT_HELP)
			{
				init_HELP(wnd, nrows, ncols);

				while(tolower(c) != 'b')
					c = getchar();

				if(tolower(c) == 'b')
				{
					init_MAIN_MENU(wnd, nrows, ncols);
					
					option = OPT_NO_OBS;
				}
			}
			else
			{
				if(option == OPT_HIGH)
				{
					/* Obtin scorul si numele jucatorului cu CEL MAI MARE SCOR */
					highscore("highscore.txt", &maxHS, name_HS, NAME_LENGTH);
					
					attron(COLOR_PAIR(9));
					mvprintw((nrows / 2) + 5, (ncols - 30) / 2 + 12 , "==>>");
					attroff(COLOR_PAIR(9));
					
					attron(COLOR_PAIR(8));
					mvprintw((nrows / 2) + 5, (ncols - 30) / 2 + 18 , "%s : %d", name_HS, maxHS);
					attroff(COLOR_PAIR(8));
					
					attron(COLOR_PAIR(7));
					mvaddstr((nrows / 2) + 5, (ncols - 30) / 2, "Highscore");
					attroff(COLOR_PAIR(7));
					
					refresh();
					
					/* Sterg numele de pe ecran */
					for(i = (ncols - 30) / 2 + 12; i < ncols; i++)
						 mvprintw((nrows / 2) + 5, i, " ");
				}
				else
				{
					/* Am ajuns aici daca am ales sa incep jocul cu/fara obstacole */
					
					initscr();
					
					wbkgd(wnd, COLOR_PAIR(1));
					
					/* Se șterge ecranul */
					clear();
					
					/* Se inhibă afișarea caracterelor introduse de la tastatură */
					noecho();
					
					/* Caracterele introduse sunt citite imediat - fără 'buffering' */
					cbreak();
					
					/* Se ascunde cursorul */	
					curs_set(0);
					
					/* Pentru a salva in maxHS de la inceput cel mai mare scor din turele precedenta */
					/* Fara acest rand maxHS = 0 la fiecare deschidere a programului */
					highscore("highscore.txt", &maxHS, name_HS, NAME_LENGTH);
	
					/* Atribui sarpelui pozitia initiala de pe ecran */
					for (i = 0; i < START_LENGTH; i++)
					{
        					snake[i].x = x;
						snake[i].y = y + i;
   					}
   					
   					/* Incarc sarpele pe ecran */
   					draw_SNAKE(snake, length);
	
					/* Sarpele porneste cu directia "in sus" */
					direction = DIR_UP; 
	
					/* Atributul pentru culoarea "gardului" */
					attron(COLOR_PAIR(2));

					/* Construiesc "gardul" */
					build_FENCE(L_UP, nrows, ncols);
					
					/* Plasez obstacolele pe ecran */
					if(option == OPT_OBS)
						draw_OBSTACLE(obs, nr_obs, nrows, ncols, L_UP);

					/* Opresc atributul */
					attroff(COLOR_PAIR(2));

					/* Atributul pentru culoarea hranei */
					attron(COLOR_PAIR(3));
	
					/* Plasez mancarea pe ecran la o pozitie disponibila 
        		 		 * Retin in foodX si foodY coordonatele hranei */
					draw_FOOD(&foodX, &foodY, snake, length, nrows, ncols, obs, nr_obs, L_UP);

					/* Opresc atributul */
					attroff(COLOR_PAIR(3));

					/* Adaug pe ecran scorul */
					attron(COLOR_PAIR(11));
					mvprintw(1, 3, "SCORE = %d ", score);
					attroff(COLOR_PAIR(11));
					mvprintw(1, 9, "=");
					attron(COLOR_PAIR(11));
					mvprintw(1, 11, "%d ", score);
					attroff(COLOR_PAIR(11));
					attron(COLOR_PAIR(10));
					mvaddstr(1, 17, "Press C to PAUSE the game and then again C to CONTINUE");
					attroff(COLOR_PAIR(10));
					
					/* Se reflectă schimbările pe ecran */
					refresh();

					/* Rămânem în while cat timp sarpele nu "moare" */
					while (FOREVER) 
					{
						sel = select(nfds, &read_descriptors, NULL, NULL, &timeout);
						
						/* Verific daca am apasat pe tastatura
                 				 * Daca da, SELECT_EVENT = 1 si atunci citesc caracterul apasat */
						if (sel == SELECT_EVENT) 
						{
							c = getchar();
							 
							/* pause = 1 doar daca am apasat pe 'c' si devine din nou 0 daca mai apas o data */
							if (tolower(c) == 'c') 
								pause = 1;
					
							/* Pot sa misc sarpele doar daca nu ma aflu in pauza */
							if(pause == 0)
							{
								/* Se determină noua poziție, în funcție de tasta apăsată
		 	 					 * Nu putem depași nrows și ncols, sau linia 1/coloana 1 (GARDUL) .
	 		 					 */
								switch (tolower(c)) 
								{
			    						case 'a':                           			      /* Daca sarpele merge spre dreapta deja, */ 
			        						if (direction != DIR_RIGHT && direction != DIR_LEFT)  /* nu se poate intoarce pe acelasi rand  sau nu poate accelera */
				    							if(x > 1)           			      /* Verific daca noua pozitie a sarpelui este in interiorul ecranului */
				   							{              			
				        							direction = DIR_LEFT;
												x--;
												ok = 1;
			            							}
			        						break;
		
			    						case 'd':                            			      /* Analog celelalte cazuri */
			        						if (direction != DIR_LEFT && direction != DIR_RIGHT) 
				    							if(x + 1 < ncols - 1)
				    							{
												direction = DIR_RIGHT;
												x++;
 												ok = 1;
				    							}
                								break;
		
			    						case 'w':
										if (direction != DIR_DOWN && direction != DIR_UP) 
				    							if(y > L_UP + 1)
				    							{
												direction = DIR_UP;
												y--;
												ok = 1;
				    							}
                								break;

			    						case 's':
										if (direction != DIR_UP && direction != DIR_DOWN) 
				    							if(y + 1 < nrows - 1)
				    							{
												direction = DIR_DOWN;
												y++;
												ok = 1;
				    							}
                								break;
								} /* Pana aici switch-ul pentru taste */
							}
			
							/* Daca directia pe care se afla sarpele e buna, il mut in noua pozitie */
							/* Daca inca sunt in pauza nu intra pe if(ok == 1) pt ca nu am dreptul de a muta sarpele atunci */
							if(ok == 1) 
							{
								/* Verific daca sarpele a ajuns cu capul in dreptul hranei */
								if(x == foodX && y == foodY) 
								{
									/* Cresc sarpele cu o unitate */
									snake[length].x = snake[length - 1].x;
    									snake[length].y = snake[length - 1].y;
									length++;
					
									attron(COLOR_PAIR(3));
									/* Plasez hrana intr-o alta pozitie disponibila */
									draw_FOOD(&foodX, &foodY, snake, length, nrows, ncols, obs, nr_obs, L_UP);
									attroff(COLOR_PAIR(3));
					
									/* Cresc scorul si il actualizez */
									score++;
									attron(COLOR_PAIR(11));
									mvprintw(1, 3, "SCORE = %d ", score);
									attroff(COLOR_PAIR(11));
									mvprintw(1, 9, "=");
									attron(COLOR_PAIR(11));
									mvprintw(1, 11, "%d ", score);
									attroff(COLOR_PAIR(11));
								
									refresh();
					
									/* Cresc viteza sarpelui la fiecare 2 patratele de lungime castigate */
									if(score % 2 == 0 && score <= 28)
										speed -= 10000;
								}
								else
								{	
									/* Verific daca sarpele a intrat intr-un obstacol */
									for(i = 0; i < nr_obs; i++)
										if(snake[0].x == obs[i].x && snake[0].y == obs[i].y)
										{
											IN_OBS = 1; 
											break;
										}

									/* Daca da ies din while(FOREVER) */
									if(IN_OBS == 1)
										break;
									
									/* Verific daca sarpele s-a muscat */
									for(i = 1; i < length; i++)
										if(snake[0].x == snake[i].x && snake[0].y == snake[i].y)
											BITTEN = 1;

									/* Daca da, ies din while(FOREVER) */	
									if(BITTEN == 1)
										break;
									
									/* Sterg coada sarpelui */
									mvaddch(snake[length - 1].y, snake[length - 1].x, ' '); 
    									snake[length - 1].x = -1;  // O scot in afara ecranului 
    									snake[length - 1].y = -1;
								}

								/* aux se comporta ca un auxiliar de la interschimbare */
								Snake aux; 

								/* Mut toate "inelele" sarpelui in pozitia de deasupra/precedenta sa */
    								for (i = length; i > 0; i--) 
								{
        								aux = snake[i - 1];
        								snake[i] = aux;
									refresh();
    								}	
			
								/* Fixez coordonatele capului sarpelui */
								/* Capul este cel ce isi modifica directie, iar corpul il urmeaza */
								snake[0].x = x;    
    								snake[0].y = y;

								/* Incarc sarpele pe ecran */
								draw_SNAKE(snake, length);
		
								ok = 0;
								
								refresh();
							}
								/* Intra pe else daca ok == 0, daca directia nu e buna sau daca sunt in pauza */
							else    /* sau am apasat o tasta gresita sau sarpele iese din ecran */
							{
								/* Daca am apasat o tasta gresita, directia sarpelui nu se modifica */
								if(tolower(c) != 'a' && tolower(c) != 's' && tolower(c) != 'd' && tolower(c) != 'w' && tolower(c) != 'c')
									sel = SELECT_EVENT;
								else 
								{
									/* Daca am apasat corect una din taste, dar directia nu e buna
                                         		 		 * Sarpele nu se poate intoarce pe acelasi rand sau accelera pe acelasi rand
                                         		 		 * Verific mai intai daca nu sunt in pauza */
									if(pause == 0)
									{
										switch(tolower(c)) 
										{
					    						case 'a':
												if(direction == DIR_RIGHT || direction == DIR_LEFT)
												{
						    							sel = SELECT_EVENT;
						    							good_direction = 1;
						    						}
												break;
					
					    						case 'd':
												if(direction == DIR_LEFT || direction == DIR_RIGHT)
						    						{
						    							sel = SELECT_EVENT;
						    							good_direction = 1;
						    						}
												break;

					    						case 'w':
												if(direction == DIR_DOWN || direction == DIR_UP)
						    						{
						    							sel = SELECT_EVENT;
						    							good_direction = 1;
						    						}
												break;
					
					    						case 's':
												if(direction == DIR_UP || direction == DIR_DOWN)
						    						{
						    							sel = SELECT_EVENT;
						    							good_direction = 1;
						    						}
												break;
										}
	
										/* GAME OVER, sarpele a lovit "gardul" */
										if(good_direction == 0)    
										 	break;
										
										good_direction = 0;
									}
									/* Daca ma aflu in pauza, jocul continua doar la apasarea tastei 'c' */
									else
									{
										do {
											c = getchar();

										} while(tolower(c) != 'c');
								
										if(tolower(c) == 'c')
										{
											pause = 0;
											sel = SELECT_NO_EVENT;
										}
									}	
								}
							}
						} /* Aici se inchide if(sel == SELECT_EVENT) */
	
						/* SELECT_NO_EVENT = 0
	  	 				 * Daca nu am apasat pe nicio tasta sau revin din PAUZA sarpele continua pe directia pe care se afla */
						if(sel == SELECT_NO_EVENT)
						{
							switch(direction)
							{
			    					case DIR_LEFT:         /* Verific pe ce directie se afla sarpele */
									if(x > 1)      /* si daca pot sa ajung acolo */
									{
				    						x--;
				    						ok2 = 1;
									}
									break;

			    					case DIR_RIGHT:
									if(x + 1 < ncols - 1)
									{
				    						x++;
				    						ok2 = 1;
									}
									break;

			    					case DIR_UP:
									if(y > L_UP + 1)
									{
				    						y--;
				    						ok2 = 1;
									}
									break;

			    					case DIR_DOWN:
									if(y + 1 < nrows - 1)
									{	
				    						y++;
				    						ok2 = 1;
									}
								break;
							}

							if(ok2 == 1)
							{
								/* Verific daca sarpele a ajuns cu capul in dreptul hranei */
								if(x == foodX && y == foodY) 
								{
									/* Cresc sarpele cu o unitate */
									snake[length].x = snake[length - 1].x;
    									snake[length].y = snake[length - 1].y;
									length++;

									attron(COLOR_PAIR(3));
									/* Plasez hrana intr-o alta pozitie disponibila */
									draw_FOOD(&foodX, &foodY, snake, length, nrows, ncols, obs, nr_obs, L_UP);
									attroff(COLOR_PAIR(3));

									/* Cresc scorul si actualizez scorul si dimensiunea */
									score++;
									attron(COLOR_PAIR(11));
									mvprintw(1, 3, "SCORE = %d ", score);
									attroff(COLOR_PAIR(11));
									mvprintw(1, 9, "=");
									attron(COLOR_PAIR(11));
									mvprintw(1, 11, "%d ", score);
									attroff(COLOR_PAIR(11));
				
									refresh();

									/* Cresc viteza sarpelui la fiecare 2 patratele de lungime castigate */
									if(score % 2 == 0 && score <= 28)
										speed -= 10000;
								}
								else
								{	
									/* Verific daca sarpele a intrat intr-un obstacol */
									for(i = 0; i < nr_obs; i++)
										if(snake[0].x == obs[i].x && snake[0].y == obs[i].y)
										{
											IN_OBS = 1; 
											break;
										}

									/* Daca da ies din while(FOREVER) */
									if(IN_OBS == 1)
										break;
									
									/* Verific daca sarpele s-a muscat */
									for(i = 1; i < length; i++)
										if(snake[0].x == snake[i].x && snake[0].y == snake[i].y)
											BITTEN = 1;
					
									/* Daca da, ies din while(FOREVER) */	
									if(BITTEN == 1)
										break;
									
									/* Sterg coada sarpelui */
									mvaddch(snake[length - 1].y, snake[length - 1].x, ' '); 
    									snake[length - 1].x = -1;  // ca sa iasa din ecran
    									snake[length - 1].y = -1;
								}

								/* aux se comporta ca un auxiliar de la interschimbare */
								Snake aux; 

								/* Mut toate "inelele" sarpelui in pozitia de deasupra/precedenta sa */
    								for (i = length; i > 0; i--) 
								{
        								aux = snake[i - 1];
        								snake[i] = aux;
									refresh();
    								}	
			
								/* Fixez coordonatele capului sarpelui */
								/* Capul este cel ce isi modifica directie, iar corpul il urmeaza */
								snake[0].x = x;    
    								snake[0].y = y;

								/* Incarc sarpele pe ecran */
								draw_SNAKE(snake, length);
								
								ok2 = 0;
								
								refresh();
							}	
							else
							{
								/*GAME OVER, sarpele a lovit "gardul" */
								break; 
							}

							/* Funcția select modifică read_descriptors și timeout
		 		 			 * Așadar, aceste variabile trebuie reinițializate
		 		 			 */
							FD_SET(KEYBOARD, &read_descriptors);
		
							timeout.tv_sec = S_TO_WAIT;
							timeout.tv_usec = speed;
							
						}/* Aici se inchide if(sel = SELECT_NO_EVENT) */
						
					}/* Aici se inchide while(FOREVER) */
					
					/* Ajung aici daca sarpele s-a muscat sau a intrat intr-un obstacol sau in gard
					 * Reinitializez ecranul de la inceput , cel cu meniul pentru a alege un joc nou sau iesire */
					
					/* Daca am obtinut un nou HIGHSCORE il inregistrez */
					if(score > maxHS)
					{
						/* Initializez ecranul pentru a scrie pentru inregistrarea HIGHSCORE-ului */
						init_SCORE_MENU(wnd, nrows, ncols, score);
						
						/* Citesc numele jucatorului */
						attron(COLOR_PAIR(11));
						mvscanw(nrows / 2, (ncols - 26) / 2 + 26, "%s", name_HS);
						attroff(COLOR_PAIR(11));
						
						/* Adaug in fisier numele si scorul jucatorului */
						fprintf(file,"%s %d\n", name_HS, score);
						
						/* Sterg numele de pe ecran */
						for(i = (ncols - 26) / 2; i < ncols; i++)
							mvaddstr(nrows / 2, i, " ");
						
						mvaddstr((nrows / 2) + 5, (ncols - 21) / 2, "Highscore UPDATED !");
						mvaddstr(nrows - 2, (ncols - 33) / 2, "Press B to go back to MAIN MENU");
						
						refresh();
						
						/* Inainte a fost activat echo() pentru a dezinhiba aparitia
						 * caracterelor introduse de la tastatura, cu scopul de a aparea pe ecran
						 * numele jucatorului in timp ce acesta il scrie 
						 * Revine la inhibarea caracterelor introduse */
						noecho();
						
						while(tolower(c) != 'b')
							c = getchar();
					}
					
					/* Initializeaza ecranul de la inceput MAIN MENU */
					init_MAIN_MENU(wnd, nrows, ncols);

					/* Reinitializarea variabilelor de joc */
					y = INIT_ROW;
					x = INIT_COL;
					direction = DIR_UP;
					length = START_LENGTH;
					BITTEN = 0;
					IN_OBS = 0;
					score = 0;
					pause = 0;
					speed = MILIS_TO_WAIT;
					option = OPT_NO_OBS;
					for(i = 0; i < nr_obs; i++)
					{
						obs[i].x = 0;
						obs[i].y = 0;
					}
					
					/* Inchide fisierul pentru a salva schimbarile */
					fclose(file);
					/* Apoi il redeschide */
					file = fopen("highscore.txt", "a+");

					refresh();	
				}  /*                                              */
				   /* Aici se inchid cele 2 else-uri de la optiuni */		
			}          /*                                              */
		}/* Aici se inchide if(tolower(c) == 'y') */
		
	}/* Aici se inchide while(FOREVER_AND_EVER) */
	
	/* Se închide fereastra ncurses */	
	endwin();
	
	return 0;
}

/* 
 *   Functie de initializare a CULORILOR
 */
void init_COLORS()
{
	/* Se initializeaza folosirea culorilor */
	start_color();
	
	/* Folosita pentru fundalul ecranului */
	init_pair(1, COLOR_WHITE, COLOR_BLACK);
	
	/* Folosita pentru "gard" si obstacole */
	init_pair(2, COLOR_WHITE, COLOR_WHITE);
	
	/* Folosita pentru hrana */
	init_pair(3, COLOR_YELLOW, COLOR_YELLOW);
	
	/* Folosita pentru capul sarpelui */
	init_pair(4, COLOR_CYAN, COLOR_CYAN);
	
	/* Folosita pentru corpul sarpelui */
	init_pair(5, COLOR_BLUE, COLOR_BLUE);
	
	/* Folosita pentru optiuni */
	init_pair(7, COLOR_WHITE, COLOR_BLUE);
	
	/* Folosite pentru scrisul HIGHSCORULUI */
	init_pair(8, COLOR_YELLOW, COLOR_BLACK);
	init_pair(9, COLOR_CYAN, COLOR_BLACK);
	
	/* Folosite pentru LOGO, culoarea sarpelui din MAIN MENU */
	init_pair(10, COLOR_GREEN, COLOR_BLACK);
	init_pair(11, COLOR_YELLOW, COLOR_BLACK);
	init_pair(12, COLOR_RED, COLOR_BLACK);
	init_pair(13, COLOR_CYAN, COLOR_BLACK);
	init_pair(14, COLOR_MAGENTA, COLOR_BLACK);
	init_pair(15, COLOR_BLUE, COLOR_BLACK);
	init_pair(16, COLOR_WHITE, COLOR_BLACK);
}

/* 
 *   Functie de initializare a MENIULUI PRINCIPAL
 */
void init_MAIN_MENU(WINDOW *wnd, int nrows, int ncols)
{
	initscr();
	/* Se șterge ecranul */
	clear();
	/* Se inhibă afișarea caracterelor introduse de la tastatură */
	noecho();
	/* Caracterele introduse sunt citite imediat - fără 'buffering' */
	cbreak();
	/* Se ascunde cursorul */	
	curs_set(0);
	/* Seteaza fundalul ecranului */
	wbkgd(wnd, COLOR_PAIR(1));
	
	draw_LOGO(nrows, ncols);
	draw_ICE_CREAM(nrows, ncols);
	
	attron(COLOR_PAIR(10));
	mvaddstr((nrows / 2) - 7, (ncols - 19) / 2, "H@ppy");
	attroff(COLOR_PAIR(10));
	attron(COLOR_PAIR(11));
	mvaddstr((nrows / 2) - 7, (ncols - 19) / 2 + 6, "Sn@ke");
	attroff(COLOR_PAIR(11));
	attron(COLOR_PAIR(13));
	mvaddstr((nrows / 2) - 7, (ncols - 19) / 2 + 12, "v1.01");
	attroff(COLOR_PAIR(13));
	attron(COLOR_PAIR(7));
	mvaddstr((nrows / 2) + 3, (ncols - 30) / 2, "Start game without obstacles");      
	attroff(COLOR_PAIR(7));
	mvaddstr((nrows / 2) + 4, (ncols - 30) / 2, "Start game with obstacles");
	mvaddstr((nrows / 2) + 5, (ncols - 30) / 2, "Highscore");
	mvaddstr((nrows / 2) + 6, (ncols - 30) / 2, "Help");
	mvaddstr((nrows / 2) + 7, (ncols - 30) / 2, "Quit");
	mvaddstr(nrows - 2, (ncols - 40) / 2, "Use W and S to NAVIGATE and Y to ENTER");

	refresh();
}

/* 
 *   Functie de initializare a ecranului HELP
 */
void init_HELP(WINDOW *wnd, int nrows, int ncols)
{
	initscr();
	/* Se șterge ecranul */
	clear();
	/* Se inhibă afișarea caracterelor introduse de la tastatură */
	noecho();
	/* Caracterele introduse sunt citite imediat - fără 'buffering' */
	cbreak();
	/* Se ascunde cursorul */	
	curs_set(0);
	/* Seteaza fundalul ecranului */
	wbkgd(wnd, COLOR_PAIR(1));
				
	attron(COLOR_PAIR(7));
	mvaddstr(2, 5, "IN GAME & MAIN MENU CONTROLS");
	attroff(COLOR_PAIR(7));
	mvaddstr(3, 5, "- W for UP and NAVIGATE");
	mvaddstr(4, 5, "- S for DOWN and NAVIGATE");
	mvaddstr(5, 5, "- A for LEFT");
	mvaddstr(6, 5, "- D for RIGHT");
	mvaddstr(7, 5, "- C for PAUSE / CONTINUE in game");
	mvaddstr(8, 5, "- B for BACK");
	attron(COLOR_PAIR(7));
	mvaddstr(9, 5, "COLORS IN GAME");
	attroff(COLOR_PAIR(7));
	mvaddstr(10, 5, "FENCE / OBSTACLES");
	attron(COLOR_PAIR(2));
	mvaddstr(10, 24, "   ");
	attroff(COLOR_PAIR(2));
	mvaddstr(11, 5, "FOOD");
	attron(COLOR_PAIR(3));
	mvaddstr(11, 24, "   ");
	attroff(COLOR_PAIR(3));
	mvaddstr(12, 5, "SNAKE's HEAD");
	attron(COLOR_PAIR(4));
	mvaddstr(12, 24, "   ");
	attroff(COLOR_PAIR(4));
	mvaddstr(13, 5, "SNAKE's BODY");
	attron(COLOR_PAIR(5));
	mvaddstr(13, 24, "   ");
	attroff(COLOR_PAIR(5));				
	attron(COLOR_PAIR(7));
	mvaddstr(14, 5, "OBJECTIVE");
	attroff(COLOR_PAIR(7));
	mvaddstr(15, 5, "Collect the FOOD to get snake LONGER avoiding the FENCE & OBSTACLES");
	mvaddstr(16, 5, "- Every piece of FOOD eaten, the SCORE and LENGTH grow by 1");
	mvaddstr(17, 5, "- Every 2 points achieved, the snake's SPEED increases");
	mvaddstr(18, 5, "- You can chose to play with/without OBSTACLES");

	mvaddstr(nrows - 2, (ncols - 33) / 2, "Press B to go back to MAIN MENU");
	
	refresh();
}

/* 
 *   Functie ecranul GAME OVER dupa obtinerea unui nou Highscore
 */
void init_SCORE_MENU(WINDOW *wnd, int nrows, int ncols, int score)
{
	initscr();
	/* Se șterge ecranul */
	clear();
	/* Se dezinhibă afișarea caracterelor introduse de la tastatură */
	echo();
	/* Caracterele introduse sunt citite imediat - fără 'buffering' */
	cbreak();
	/* Se ascunde cursorul */	
	curs_set(0);
	/* Seteaza fundalul ecranului */
	wbkgd(wnd, COLOR_PAIR(1));
	
	attron(COLOR_PAIR(10));
	mvprintw((nrows / 2) - 2, (ncols - 48) / 2, "CONGRATULATIONS !");
	attroff(COLOR_PAIR(10));
	mvprintw((nrows / 2) - 2, (ncols - 48) / 2 + 18, "You made a new");
	attron(COLOR_PAIR(13));
	mvprintw((nrows / 2) - 2, (ncols - 48) / 2 + 33, "HIGHSCORE");
	attroff(COLOR_PAIR(13));
	mvprintw((nrows / 2) - 2, (ncols - 48) / 2 + 43, "=>");
	attron(COLOR_PAIR(13));
	mvprintw((nrows / 2) - 2, (ncols - 48) / 2 + 46, "%d", score);
	attroff(COLOR_PAIR(13));
	mvaddstr(nrows / 2, (ncols - 26) / 2, "Please WRITE your NAME :");
	
	refresh();
}

/* 
 *   Functie de construire a GARDULUI
 */
void build_FENCE(int L_UP, int nrows, int ncols)
{
	int i, j;
	
	for(i = L_UP; i < nrows; i++)
		for(j = 0; j < ncols; j++)
		{
			mvaddch(i, 0, '#');           /* Partea din stanga  */
			mvaddch(nrows - 1, j, '#');   /* Partea de jos      */
			mvaddch(i, ncols - 1, '#');   /* Partea din dreapta */
			mvaddch(L_UP, j, '#');        /* Partea de sus      */
		}
}

/*
 *   Functie de desenare a sarpelui 
 */
void draw_SNAKE(Snake snake[], int length)
{
	int i;
	
	for (i = 0; i < length; i++) 
	{
       		if (snake[i].y != -1 && snake[i].x != -1) 
		{
			if(i == 0)
			{
				attron(COLOR_PAIR(4));
        			mvaddch(snake[i].y, snake[i].x, '0'); // capul
				attroff(COLOR_PAIR(4));
			}
			else
			{
				attron(COLOR_PAIR(5));
				mvaddch(snake[i].y, snake[i].x, '*'); // coada
				attroff(COLOR_PAIR(5));
			}
        	}
    	}
}

/* 
 *   Functie de plasare a obstacolelor 
 */
void draw_OBSTACLE (Obstacle obs[], int nr_obs, int nrows, int ncols, int L_UP)
{
	int i;
	
	srand(time(NULL));
	
	/* Plasez la intamplare nr_obs de obstacole in interiorul ecranului */
	for(i = 0; i < nr_obs; i++)
	{
		do
		{
			obs[i].x = rand() % ncols;
			obs[i].y = rand() % nrows;

		/* Verific daca obstacolul se afla pe gard sau a fost pus deasupra altuia */
		} while(obs[i].x == 0 || obs[i].x == ncols - 1 || obs[i].y <= L_UP || obs[i].y == nrows - 1 || (obs[i].x == obs[i-1].x && obs[i].y == obs[i-1].y && i > 0 ) );
	}

	/* Adaug obstacolele pe ecran */
	for(i = 0; i < nr_obs; i++)
		mvaddch(obs[i].y, obs[i].x, '#');
	
	refresh();
}

/* 
 *   Functie de plasare a hranei 
 */
void draw_FOOD (int *rand_x, int *rand_y, Snake snake[], int length, int nrows, int ncols, Obstacle obs[], int nr_obs, int L_UP)
{
	int i, k, x = -1, j = -1;
	
	srand(time(NULL));
	
	/* Plasez la intamplare o posibila pozitie a hranei, daca aceasta nu se afla pe "gard" */
	do
	{
		*rand_x = rand() % ncols;
		*rand_y = rand() % nrows;

	} while(*rand_x == 0 || *rand_x == ncols - 1 || *rand_y <= L_UP || *rand_y == nrows - 1);

	for(i = 0; i < length; i++)
	{
		for(k = 0; k < nr_obs; k++)
			/* Verific daca hrana va fi pusa pe sarpe sau pe un obstacol 
			 * In acest caz caut o pozitie libera */
			if( (snake[i].x == (*rand_x) && snake[i].y == (*rand_y)) || (obs[k].x == (*rand_x) && obs[k].y == (*rand_y)) )
			{
				do
				{
					do
					{
						*rand_x = rand() % ncols;
						*rand_y = rand() % nrows;

					} while(*rand_x == 0 || *rand_x == ncols - 1 || *rand_y <= L_UP || *rand_y == nrows - 1);

					j++; 
					x++;

				} while( (snake[j].x == (*rand_x) && snake[j].y == (*rand_y) && j < length) || (obs[x].x == (*rand_x) && obs[x].y == (*rand_y) && x < nr_obs) );
			}
	}
	
	/* Adaug hrana*/
	mvaddch((*rand_y), (*rand_x), 'H');
	
	refresh();	
}

/* 
 *   Functie de calculare a highscore-ului
 */
void highscore(const char* fileName, int *maxim, char *name_HS, int name_Length)
{
	FILE *file = fopen(fileName, "r");
	int nr;
	char name[name_Length];
   
  	while(!feof(file))
    	{  
      		fscanf(file, "%s%d\n", name, &nr);
		
		if(nr > (*maxim))
		{
			*maxim = nr; 
			strncpy(name_HS, name, strlen(name) + 1);
		}   
    	}       
}

void draw_LOGO(int nrows, int ncols)
{
	attron(COLOR_PAIR(10));
	mvaddstr((nrows / 2) - 11, (ncols - 50) / 2, "   ,-(|)--(|)-.");
	attroff(COLOR_PAIR(10));
	attron(COLOR_PAIR(11));
	mvaddstr((nrows / 2) - 10, (ncols - 50) / 2, "   \\_   ..   _/");
	attroff(COLOR_PAIR(11));
	attron(COLOR_PAIR(12));
	mvaddstr((nrows / 2) - 9,  (ncols - 50) / 2, "     \\______/  ");
	attroff(COLOR_PAIR(12));
	attron(COLOR_PAIR(13));
	mvaddstr((nrows / 2) - 8,  (ncols - 50) / 2, "       V  V                                  ____");
	attroff(COLOR_PAIR(13));
	attron(COLOR_PAIR(14));
	mvaddstr((nrows / 2) - 7,  (ncols - 50) / 2, "       `.^^`.                               /^,--`");
	attroff(COLOR_PAIR(14));
	attron(COLOR_PAIR(15));
	mvaddstr((nrows / 2) - 6,  (ncols - 50) / 2, "         \\^^^\\                             (^^\\ ");
	attroff(COLOR_PAIR(15));
	attron(COLOR_PAIR(16));
	mvaddstr((nrows / 2) - 5,  (ncols - 50) / 2, "         |^^^|                  _,-._       \\^^\\ ");
	attroff(COLOR_PAIR(16));
	attron(COLOR_PAIR(10));
	mvaddstr((nrows / 2) - 4,  (ncols - 50) / 2, "        (^^^^\\      __      _,-'^^^^^`.    _,'^^)");
	attroff(COLOR_PAIR(10));
	attron(COLOR_PAIR(11));
	mvaddstr((nrows / 2) - 3,  (ncols - 50) / 2, "         \\^^^^`._,-'^^`-._.'^^^^__^^^^ `--'^^^_/");
	attroff(COLOR_PAIR(11));
	attron(COLOR_PAIR(12));
	mvaddstr((nrows / 2) - 2,  (ncols - 50) / 2, "          \\^^^^^ ^^^_^^^^^^^_,-'  `.^^^^^^^^_/ ");
	attroff(COLOR_PAIR(12));
	attron(COLOR_PAIR(13));
	mvaddstr((nrows / 2) - 1,  (ncols - 50) / 2, "           `.____,-' `-.__.'        `-.___.' ");  
	attroff(COLOR_PAIR(13));
	
	refresh();
}

void draw_ICE_CREAM(int nrows, int ncols)
{
	attron(COLOR_PAIR(11));
	mvaddstr((nrows / 2) - 11, (ncols - 75) / 2, "     . ,");
	mvaddstr((nrows / 2) - 10, (ncols - 75) / 2, "      *    ,");
	mvaddstr((nrows / 2) - 9 , (ncols - 75) / 2, " ` *~.|,~* '");
	mvaddstr((nrows / 2) - 8 , (ncols - 75) / 2, " '  ,~*~~* `     _");
	mvaddstr((nrows / 2) - 7 , (ncols - 75) / 2, "  ,* / \\`* '    ");
	attroff(COLOR_PAIR(11));
	attron(COLOR_PAIR(13));
	mvaddstr((nrows / 2) - 7 , (ncols - 75) / 2 + 16, "//");
	attroff(COLOR_PAIR(13));
	attron(COLOR_PAIR(11));
	mvaddstr((nrows / 2) - 6 , (ncols - 75) / 2, "   ,* ; \\");
	attroff(COLOR_PAIR(11));
	attron(COLOR_PAIR(16));
	mvaddstr((nrows / 2) - 6 , (ncols - 75) / 2 + 9, ",O.");
	attroff(COLOR_PAIR(16));
	attron(COLOR_PAIR(13));
	mvaddstr((nrows / 2) - 6 , (ncols - 75) / 2 + 15, "//");
	attroff(COLOR_PAIR(13));
	attron(COLOR_PAIR(16));
	mvaddstr((nrows / 2) - 5 , (ncols - 75) / 2, "       ,(:::)=");
	attroff(COLOR_PAIR(16));
	attron(COLOR_PAIR(13));
	mvaddstr((nrows / 2) - 5 , (ncols - 75) / 2 + 14, "//");
	attroff(COLOR_PAIR(13));
	attron(COLOR_PAIR(12));
	mvaddstr((nrows / 2) - 4 , (ncols - 75) / 2, "      (  `~(###)");
	attroff(COLOR_PAIR(12));
	attron(COLOR_PAIR(10));
	mvaddstr((nrows / 2) - 3 , (ncols - 75) / 2, "       %----'`""%");
	attroff(COLOR_PAIR(10));
	attron(COLOR_PAIR(13));
	mvaddstr((nrows / 2) - 2 , (ncols - 75) / 2, "        \\    /");
	mvaddstr((nrows / 2) - 1 , (ncols - 75) / 2, "         \\  /");
	mvaddstr((nrows / 2)     , (ncols - 75) / 2, "        __)(__");  
	attroff(COLOR_PAIR(13));
	attron(COLOR_PAIR(11));
	mvaddstr((nrows / 2) + 1 , (ncols - 75) / 2, "       '------`");
	attroff(COLOR_PAIR(11));
	
	refresh();
}
