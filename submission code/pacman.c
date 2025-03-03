/****************************************************
* Pacman For Console V1.3                           *
* By: Mike Billars (michael@gmail.com)              *
* Date: 2014-04-26                                  *
*                                                   *
* Please see file COPYING for details on licensing  *
*       and redistribution of this program          *
*                                                   *
* Adapted by Nir Lipovetzky for COMP20003 (2019)    *
*                                                   *
****************************************************/

/***********
* INCLUDES *
***********/
#include <stdio.h>
#include <curses.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/timeb.h>
#include <assert.h>
#include <locale.h>
#include "pacman.h"
#include "node.h"
#include "ai.h"
#include "utils.h"



/*************
* PROTOTYPES *
*************/
void IntroScreen();                                     //Show introduction screen and menu
void CheckCollision();                                  //See if Pacman and Ghosts collided
void CheckScreenSize();                                 //Make sure resolution is at least 32x29
void CreateWindows(int y, int x, int y0, int x0);       //Make ncurses windows
void Delay();                                           //Slow down game for better control
void DrawWindow();                                      //Refresh display
void ExitProgram(const char *message);                  //Exit and display something
void GetInput();                                        //Get user input
void InitCurses();                                      //Start up ncurses
void LoadLevel(char *levelfile);                        //Load level into memory
void MainLoop();                                        //Main program function
void MoveGhosts();                                      //Update Ghosts' location
void MovePacman();                                      //Update Pacman's location
void PauseGame();                                       //Pause
void update_current_state();                            //sync current game state
void send_action(move_t move);                          //Like getInput but for AI agent

/*********
* MACROS *
*********/
#define TERMINAL_TYPE (strcmp(getenv("TERM"), "xterm") == 0 ? "rxvt" : \
  getenv("TERM"))


/*******************
* GLOBAL VARIABLES *
*******************/
//I know global variables are bad, but it's just sooo easy!

char LevelFile[] = LEVELS_FILE;     //Locations of default levels
int FreeLife = 1000;                //Starting points for free life
int Points = 0;                     //Initial points
int Lives = 3;                      //Number of lives you start with
int SpeedOfGame = 175;              //How much of a delay is in the game

//Windows used by ncurses
WINDOW * win;
WINDOW * status;
SCREEN *mainScreen = NULL;

//For colors
enum { Wall = 1, Normal = 2, Pellet = 3, PowerUp = 4, GhostWall = 5, Ghost1 = 6, Ghost2 = 7, Ghost3 = 8, Ghost4 = 9, BlueGhost = 10, Pacman = 11 };


int Loc[5][2] = { 0 };                    //Location of Ghosts and Pacman (Pacman loc in idx 4)
int Dir[5][2] = { 0 };                    //Direction of Ghosts and Pacman
int StartingPoints[5][2] = { 0 };         //Default location in case Pacman/Ghosts die
int Invincible = 0;                       //Check for invincibility
int Food = 0;                             //Number of pellets left in level
int Level[29][28] = { 0 };                //Main level array
int LevelNumber = 0;                      //What level number are we on?
int GhostsInARow = 0;                     //Keep track of how many points to give for eating ghosts
int tleft = 0;                            //How long left for invincibility

int budget=0; //Max budget expanded nodes Dijkstra
bool ai_run = false; //Run AI
bool ai_pause = false; // To check stats of every action
propagation_t propagation; //Propagation type
state_t current_state; //Contains a copy of the current game state
char ai_stats[500]; //Info printed out about AI algorithm

// Global variables for output.txt!
propagation_t outputPropagation;
int outputBudget = 0;
unsigned maxDepth = 0;
unsigned totalGenerated = 0;
unsigned totalExpanded = 0;
double outPutTime = 0;
unsigned expandedPerSecond = 0;
unsigned outputScore = 0;

//Timekeeping...
clock_t start;
double secs;

void print_usage(){
    printf("To run the AI solver: \n");
    printf("USAGE: ./pacman <[level_#/[level_file_name> <ai/ai_pause> <max/avg> <budget> slow\n");
    printf("or, to play with the keyboard: \n");
    printf("USAGE: ././pacman <[level_#/[level_file_name>\n");
}

/****************************************************************
* Function:    main()                                           *
* Parameters:  argc, argv (passed from command line)            *
* Returns:     Success                                          *
* Description: Initiate the program and call the subfunctions   *
****************************************************************/
int main(int argc, char *argv[100]) {
    //Delete 2 lines
    start= clock();
    
    /**
     * Parsing command line options
     */
    if( argc > 2 && argc < 5 ){
        print_usage();
        return 0;
    }

    int j = 0;
    srand( (unsigned)time( NULL ) );

    InitCurses();                   //Must be called to start ncurses
    CheckScreenSize();              //Make sure screen is big enough
    CreateWindows(29, 28, 1, 1);    //Create the main and status windows



    if (argc > 2 ) {
        ai_run = true;
        if( strcmp(argv[2],"ai_pause")==0 ){
            ai_pause = true;
        }

        initialize_ai();

        if( strcmp(argv[3],"avg")==0 ){
            propagation = avg;
        }
        else if( strcmp(argv[3],"max")==0 ){
            propagation = max;
        }
        else{
            print_usage();
            return 0;
        }

        sscanf (argv[4],"%d",&budget);

    }

    //If they specified a level to load
    if((argc > 1) && (strlen(argv[1]) > 1)) {
        argv[1][99] = '\0';
        LoadLevel(argv[1]);         //Load it and...
        MainLoop();                 //Start the game
    }

    //If they did not enter a level, display intro screen then use default levels
    else {
        IntroScreen();              //Show intro "movie"
        j = 1;                      //Set initial level to 1

        if(argc > 1) j = argv[1][0] - '0';    //They specified a level on which to start (1-9)

        //Load 9 levels, 1 by 1, if you can beat all 9 levels in a row, you're awesome
        for(LevelNumber = j; LevelNumber < 10; LevelNumber++) {

            //Replace level string underscore with the actual level number (see pacman.h)
            LevelFile[strlen(LevelFile) - 6] = '0';
            LevelFile[strlen(LevelFile) - 5] = LevelNumber + '0';

            LoadLevel(LevelFile);   //Load level into memory
            Invincible = 0;         //Reset invincibility with each new level
            MainLoop();             //Start the level

        }

    }
    //Delete 3 lines
    //Game has ended, deactivate and end program
    ExitProgram(EXIT_MSG);
    
    //This doesn't need to be here, but just to be proper
    return(0);
}

/****************************************************************
* Function:    CheckCollision()                                 *
* Parameters:  none                                             *
* Returns:     none                                             *
* Description: Check and handle if Pacman collided with a ghost *
****************************************************************/
void CheckCollision() {

    //Temporary variable
    int a = 0;

    //Check each ghost, one at a time for collision
    for(a = 0; a < 4; a++) {

        //Ghost X and Y location is equal to Pacman X and Y location (collision)
        if((Loc[a][0] == Loc[4][0]) && (Loc[a][1] == Loc[4][1])) {

            //Pacman is invincible, ghost dies
            if(Invincible == 1) {

                Points = Points + GhostsInARow * 20;    //Increase points by an increasing value

                //Display the points earned for eating the ghost
                mvwprintw(win, Loc[4][0], Loc[4][1] - 1, "%d", (GhostsInARow * 20));
                GhostsInARow *= 2;

                wrefresh(win);      //Update display
                usleep(1000000);    //and pause for a second

                //Reset the ghost's position to the starting location
                Loc[a][0] = StartingPoints[a][0]; Loc[a][1] = StartingPoints[a][1];
            }

            //Pacman is vulnerable, Pacman dies
            else {

                //Display an X in position
                wattron(win, COLOR_PAIR(Pacman));
                mvwprintw(win, Loc[4][0], Loc[4][1], "X");
                wrefresh(win);

                //Subtract one life and pause for 1 second
                Lives--;
                usleep(1000000);

                //If no more lives, game over
                if(Lives == -1) ExitProgram(END_MSG);

                //If NOT game over...

                //Reset Pacman's and ghosts' positions
                for(a = 0; a < 5; a++) {
                    Loc[a][0] = StartingPoints[a][0];
                    Loc[a][1] = StartingPoints[a][1];
                }

                //Reset directions
                Dir[0][0] =  1; Dir[0][1] =  0;
                Dir[1][0] = -1; Dir[1][1] =  0;
                Dir[2][0] =  0; Dir[2][1] = -1;
                Dir[3][0] =  0; Dir[3][1] =  1;
                Dir[4][0] =  0; Dir[4][1] = -1;

                DrawWindow();       //Redraw window
                usleep(1000000);    //Pause for 1 second before continuing game
            }
        }
    }
}

/****************************************************************
* Function:    CheckScreenSize()                                *
* Parameters:  none                                             *
* Returns:     none                                             *
* Description: Make sure the virtual console is big enough      *
****************************************************************/
void CheckScreenSize() {

        //Make sure the window is big enough
        int h, w; getmaxyx(stdscr, h, w);

        if((h < 35) || (w < 29)) {
                endwin();
                delscreen(mainScreen);
                fprintf(stderr, "\nSorry.\n");
                fprintf(stderr, "To play Pacman for Console, your console window must be at least 35x29\n");
                fprintf(stderr, "Please resize your window/resolution and re-run the game.\n\n");
                exit(0);
        }

}

/****************************************************************
* Function:    CreateWindows()                                  *
* Parameters:  y, x, y0, x0 (coords and size of window)         *
* Returns:     none                                             *
* Description: Create the main game windows                     *
****************************************************************/
void CreateWindows(int y, int x, int y0, int x0) {
    //Create two new windows, for status bar and for main level
    win = newwin(y, x, y0, x0);
    status = newwin(16, 130, 29, 1);
}

/****************************************************************
* Function:    Delay()                                          *
* Parameters:  none                                             *
* Returns:     none                                             *
* Description: Pause the game and still get keyboard input      *
****************************************************************/
void Delay() {

    //Needed to get time
    struct timeb t_start, t_current;
    ftime(&t_start);

    //Slow down the game a little bit
    do {
        if(ai_run == false)
            GetInput();                 //Still get the input from keyboard
        ftime(&t_current);          //Update time and check if enough time has overlapped
    } while (abs(t_start.millitm - t_current.millitm) < SpeedOfGame);
}

/****************************************************************
* Function:    DrawWindow()                                     *
* Parameters:  none                                             *
* Returns:     none                                             *
* Description: Redraw each window to update the screen          *
****************************************************************/
void DrawWindow() {

    //Temporary variables
    int a = 0; int b = 0;
    //Variable used to display certain characters
    chtype chr = ' ';
    //Variable that displays normal/bold character
    chtype attr = 0;

    //Display level array
    for(a = 0; a < 29; a++) for(b = 0; b < 28; b++) {
        //Determine which character is in location and display it
        switch(Level[a][b]) {
        case 0: chr = ' '; attr = A_NORMAL; wattron(win, COLOR_PAIR(Normal));    break;
        case 1: chr = ' '; attr = A_NORMAL; wattron(win, COLOR_PAIR(Wall));      break;
        case 2: chr = '.'; attr = A_NORMAL; wattron(win, COLOR_PAIR(Pellet));    break;
        case 3: chr = '*'; attr = A_BOLD;   wattron(win, COLOR_PAIR(PowerUp));   break;
        case 4: chr = ' '; attr = A_NORMAL; wattron(win, COLOR_PAIR(GhostWall)); break;
        }

        //Display character. "attr" is ORed with chr as documented for mvwaddch,
        //    this will make the character bold/normal
        //assert(mvwaddch(win, a, b, chr | attr) != ERR);
        mvwaddch(win, a, b, chr | attr);
    }

    //Display number of lives, score, and level

    attr = A_NORMAL;                //Reset attribute in case it's set to "BOLD"

    //Switch window to status, and change color
    wmove(status, 1, 1);
    wattron(status, COLOR_PAIR(Pacman));

    //Display number of lives
    for(a = 0; a < Lives; a++)
        wprintw(status, "C ");
    wprintw(status, "  ");

    //Display level number and score
    wattron(status, COLOR_PAIR(Normal));
    mvwprintw(status, 2, 2, "Level: %d     Score: %d ", LevelNumber, Points);

    mvwprintw(status, 3, 2, "%s", ai_stats);

    wrefresh(status);               //Update status window

    //Display ghosts if Pacman is not invincible
    if(Invincible == 0) {
        wattron(win, COLOR_PAIR(Ghost1)); mvwaddch(win, Loc[0][0], Loc[0][1], '&');
        wattron(win, COLOR_PAIR(Ghost2)); mvwaddch(win, Loc[1][0], Loc[1][1], '&');
        wattron(win, COLOR_PAIR(Ghost3)); mvwaddch(win, Loc[2][0], Loc[2][1], '&');
        wattron(win, COLOR_PAIR(Ghost4)); mvwaddch(win, Loc[3][0], Loc[3][1], '&');
    }

    //OR display vulnerable ghosts
    else {
        wattron(win, COLOR_PAIR(BlueGhost));
        mvwaddch(win, Loc[0][0], Loc[0][1], tleft + '0');
        mvwaddch(win, Loc[1][0], Loc[1][1], tleft + '0');
        mvwaddch(win, Loc[2][0], Loc[2][1], tleft + '0');
        mvwaddch(win, Loc[3][0], Loc[3][1], tleft + '0');
    }

    //Display Pacman
    wattron(win, COLOR_PAIR(Pacman)); mvwaddch(win, Loc[4][0], Loc[4][1], 'C' | A_NORMAL);


    wrefresh(win);                  //Update main window
}

/****************************************************************
* Function:    DrawWindowState(state_t state)                   *
* Parameters:  none                                             *
* Returns:     none                                             *
* Description: Redraw each window given a game state            *
* to update the screen                                          *
****************************************************************/
void DrawWindowState(state_t state) {

    //Temporary variables
    int a = 0; int b = 0;
    chtype chr = ' ';                 //Variable used to display certain characters
    chtype attr = 0;                       //Variable that displays normal/bold character

    //Display level array
    for(a = 0; a < 29; a++) for(b = 0; b < 28; b++) {
        //Determine which character is in location and display it
        switch(state.Level[a][b]) {
        case 0: chr = ' '; attr = A_NORMAL; wattron(win, COLOR_PAIR(Normal));     break;
        case 1: chr = ' '; attr = A_NORMAL; wattron(win, COLOR_PAIR(Wall));       break;
        case 2: chr = '.'; attr = A_NORMAL; wattron(win, COLOR_PAIR(Pellet));     break;
        case 3: chr = '*'; attr = A_BOLD;   wattron(win, COLOR_PAIR(PowerUp));    break;
        case 4: chr = ' '; attr = A_NORMAL; wattron(win, COLOR_PAIR(GhostWall));  break;
        }

        //Display character. "attr" is ORed with chr as documented for mvwaddch,
        //    this will make the character bold/normal
        mvwaddch(win, a, b, chr | attr);
    }

    //Display number of lives, score, and level

    attr = A_NORMAL;                //Reset attribute in case it's set to "BOLD"

    //Switch window to status, and change color
    wmove(status, 1, 1);
    wattron(status, COLOR_PAIR(Pacman));

    //Display number of lives
    for(a = 0; a < state.Lives; a++)
        wprintw(status, "C ");
    wprintw(status, "  ");

    //Display level number and score
    wattron(status, COLOR_PAIR(Normal));
    mvwprintw(status, 2, 2, "Level: %d     Score: %d ", state.LevelNumber, state.Points);

    wrefresh(status);               //Update status window

    //Display ghosts if Pacman is not invincible
    if(state.Invincible == 0) {
        wattron(win, COLOR_PAIR(Ghost1)); mvwaddch(win, state.Loc[0][0], state.Loc[0][1], '&');
        wattron(win, COLOR_PAIR(Ghost2)); mvwaddch(win, state.Loc[1][0], state.Loc[1][1], '&');
        wattron(win, COLOR_PAIR(Ghost3)); mvwaddch(win, state.Loc[2][0], state.Loc[2][1], '&');
        wattron(win, COLOR_PAIR(Ghost4)); mvwaddch(win, state.Loc[3][0], state.Loc[3][1], '&');
    }

    //OR display vulnerable ghosts
    else {
        wattron(win, COLOR_PAIR(BlueGhost));
        mvwaddch(win, state.Loc[0][0], state.Loc[0][1], tleft + '0');
        mvwaddch(win, state.Loc[1][0], state.Loc[1][1], tleft + '0');
        mvwaddch(win, state.Loc[2][0], state.Loc[2][1], tleft + '0');
        mvwaddch(win, state.Loc[3][0], state.Loc[3][1], tleft + '0');
    }

    //Display Pacman
    wattron(win, COLOR_PAIR(Pacman));
    chr = 'C';
    mvwaddch(win, state.Loc[4][0], state.Loc[4][1], chr | A_NORMAL);

    wrefresh(win);                  //Update main window
}

/****************************************************************
* Function:    ExitProgram()                                    *
* Parameters:  message: text to display before exiting          *
* Returns:     none                                             *
* Description: Gracefully end program                           *
****************************************************************/
void ExitProgram(const char *message) {
    endwin();                       //Uninitialize ncurses and destroy windows
    delscreen(mainScreen);
    printf("%s\n", message);        //Display message
    // Why not
    outputScore = Points;
    printf("Final Score: %d\n",Points);
    
    //Time to write the correct output to output.txt
    FILE *f = fopen("output.txt", "w");
    assert(f);
    
    //Timekeeping...
    secs= ((double) (clock() - start)) / CLOCKS_PER_SEC;
    
    //For comma formatting between large numbers
    setlocale(LC_NUMERIC, "");
    
    //Print all the required stuff into output.txt!
    if(outputPropagation == 0){
        fprintf(f, "Propagation = Max\n");
    }
    else{
        fprintf(f, "Propagation = Avg\n");
    }
    fprintf(f, "Budget = %d\n", outputBudget);
    fprintf(f, "MaxDepth = %d\n", maxDepth);
    fprintf(f, "TotalGenerated = %'d\n", totalGenerated);
    fprintf(f, "TotalExpanded = %'d\n", totalExpanded);
    fprintf(f, "Time = %.2f seconds\n", secs);
    fprintf(f, "Expanded/Second = %'d\n", (int)(totalExpanded/secs));
    fprintf(f, "Score = %d\n", outputScore);
    //Close the file pointer!
    fclose(f);
    exit(0);                        //End program, return 0
}

/****************************************************************
* Function:    GetInput()                                       *
* Parameters:  none                                             *
* Returns:     none                                             *
* Description: Get input from user and take appropriate action  *
****************************************************************/
void GetInput() {

    int ch;                         //Key pushed
    static int chtmp;               //Buffered key
    int tmp;

    ch = getch();                   //Figure out which key is pressed

    //If they are not pressing something, use previous input
    if(ch == ERR) ch = chtmp;
    chtmp = ch;

    //Determine which button is pushed
    switch (ch) {
    case KEY_UP:    case 'w': case 'W':            //Move pacman up
        if(Loc[4][0] <= 0) tmp = LEVEL_HEIGHT - 1;
        else tmp = Loc[4][0] - 1;
        if((Level[tmp][Loc[4][1]] != 1)
        && (Level[tmp][Loc[4][1]] != 4))
            { Dir[4][0] = -1; Dir[4][1] =  0; }
        break;

    case KEY_DOWN:  case 's': case 'S':            //Move pacman down
        if(Loc[4][0] >= 28) tmp = 0;
        else tmp = Loc[4][0] + 1;
        if((Level[tmp][Loc[4][1]] != 1)
        && (Level[tmp][Loc[4][1]] != 4))
            { Dir[4][0] =  1; Dir[4][1] =  0; }
        break;

    case KEY_LEFT:  case 'a': case 'A':            //Move pacman left
        if(Loc[4][1] <= 0) tmp = LEVEL_WIDTH - 1;
        else tmp = Loc[4][1] - 1;
        if((Level[Loc[4][0]][tmp] != 1)
        && (Level[Loc[4][0]][tmp] != 4))
            { Dir[4][0] =  0; Dir[4][1] = -1; }
        break;

    case KEY_RIGHT: case 'd': case 'D':            //Move pacman right
        if(Loc[4][1] >= 27) tmp = 0;
        else tmp = Loc[4][1] + 1;
        if((Level[Loc[4][0]][tmp] != 1)
        && (Level[Loc[4][0]][tmp] != 4))
            { Dir[4][0] =  0; Dir[4][1] =  1; }
        break;

    case 'p': case 'P':                            //Pause game
        PauseGame();
        chtmp = getch();            //Update buffered input
        break;

    case 'q': case 'Q':                            //End program
        ExitProgram(QUIT_MSG);
        break;

    }
}

void send_action(move_t move) {

    int ch;                         //Key pushed
    static int chtmp;               //Buffered key
    int tmp;

    ch = getch();                   //Figure out which key is pressed

    //If they are not pressing something, use previous input
    if(ch == ERR) ch = chtmp;
    chtmp = ch;

    //Determine which button is pushed
    switch (move) {
    case up:          //Move pacman up
        if(Loc[4][0] <= 0) tmp = LEVEL_HEIGHT - 1;
        else tmp = Loc[4][0] - 1;
        if((Level[tmp][Loc[4][1]] != 1)
        && (Level[tmp][Loc[4][1]] != 4))
            { Dir[4][0] = -1; Dir[4][1] =  0; }
        break;

    case down:         //Move pacman down
        if(Loc[4][0] >= 28) tmp = 0;
        else tmp = Loc[4][0] + 1;
        if((Level[tmp][Loc[4][1]] != 1)
        && (Level[tmp][Loc[4][1]] != 4))
            { Dir[4][0] =  1; Dir[4][1] =  0; }
        break;

    case left:         //Move pacman left
        if(Loc[4][1] <= 0) tmp = LEVEL_WIDTH - 1;
        else tmp = Loc[4][1] - 1;
        if((Level[Loc[4][0]][tmp] != 1)
        && (Level[Loc[4][0]][tmp] != 4))
            { Dir[4][0] =  0; Dir[4][1] = -1; }
        break;

    case right:          //Move pacman right
        if(Loc[4][1] >= 27) tmp = 0;
        else tmp = Loc[4][1] + 1;
        if((Level[Loc[4][0]][tmp] != 1)
        && (Level[Loc[4][0]][tmp] != 4))
            { Dir[4][0] =  0; Dir[4][1] =  1; }
        break;



    }
}

/****************************************************************
* Function:    InitCurses()                                     *
* Parameters:  none                                             *
* Returns:     none                                             *
* Description: Initialize ncurses and set defined colors        *
****************************************************************/
void InitCurses() {

    //initscr();                      //Needed for ncurses windows
    mainScreen = newterm(TERMINAL_TYPE, stdout, stdin);
    set_term(mainScreen);
    start_color();                  //Activate colors in console
    curs_set(0);                    //    Don't remember
    keypad(stdscr, TRUE);           //Activate arrow keys
    nodelay(stdscr, TRUE);          //Allow getch to work without pausing
    nonl();                         //    Don't remember
    cbreak();                       //    Don't remember
    noecho();                       //Don't display input key

    //Make custom text colors
    init_pair(Normal,    COLOR_WHITE,   COLOR_BLACK);
    init_pair(Wall,      COLOR_WHITE,   COLOR_WHITE);
    init_pair(Pellet,    COLOR_WHITE,   COLOR_BLACK);
    init_pair(PowerUp,   COLOR_BLUE,    COLOR_BLACK);
    init_pair(GhostWall, COLOR_WHITE,   COLOR_CYAN);
    init_pair(Ghost1,    COLOR_RED,     COLOR_BLACK);
    init_pair(Ghost2,    COLOR_CYAN,    COLOR_BLACK);
    init_pair(Ghost3,    COLOR_MAGENTA, COLOR_BLACK);
    init_pair(Ghost4,    COLOR_YELLOW,  COLOR_BLACK);
    init_pair(BlueGhost, COLOR_BLUE,    COLOR_RED);
    init_pair(Pacman,    COLOR_YELLOW,  COLOR_BLACK);
}

/****************************************************************
* Function:    IntroScreen()                                    *
* Parameters:  none                                             *
* Returns:     none                                             *
* Description: Display the introduction animation               *
****************************************************************/
void IntroScreen() {
    int a = 0;
    int b = 23;

    a=getch(); a=getch(); a=getch();//Clear the buffer

    mvwprintw(win, 20, 8, "Press any key...");

    //Scroll Pacman to middle of screen
    for(a = 0; a < 13; a++) {
        if(getch()!=ERR) return;
        wattron(win, COLOR_PAIR(Pacman));
        mvwprintw(win, 8, a, " C");
        wrefresh(win);
        usleep(100000);
    }

    //Show "Pacman"
    wattron(win, COLOR_PAIR(Pacman));
    mvwprintw(win, 8, 12, "PACMAN");
    wrefresh(win);
    usleep(1000000);

    //Ghosts chase Pacman
    for(a = 0; a < 23; a++) {
        if(getch()!=ERR) return;
        wattron(win, COLOR_PAIR(Pellet)); mvwprintw(win, 13, 23, "*");
        wattron(win, COLOR_PAIR(Pacman)); mvwprintw(win, 13, a, " C");
        wattron(win, COLOR_PAIR(Ghost1)); mvwprintw(win, 13, a-3, " &");
        wattron(win, COLOR_PAIR(Ghost3)); mvwprintw(win, 13, a-5, " &");
        wattron(win, COLOR_PAIR(Ghost2)); mvwprintw(win, 13, a-7, " &");
        wattron(win, COLOR_PAIR(Ghost4)); mvwprintw(win, 13, a-9, " &");
        wrefresh(win);
        usleep(100000);
    }

    usleep(150000);

    //Pacman eats powerup and chases Ghosts
    for(a = 25; a > 2; a--) {
        if(getch()!=ERR) return;
        wattron(win, COLOR_PAIR(Pellet)); mvwprintw(win, 13, 23, " ");

        //Make ghosts half as fast so Pacman can catch them
        if(a%2) b--;

        wattron(win, COLOR_PAIR(BlueGhost)); mvwprintw(win, 13, b-9, "& & & &");

        //Erase the old positions of ghosts
        wattron(win, COLOR_PAIR(Pacman)); mvwprintw(win, 13, b-9+1, " ");
        wattron(win, COLOR_PAIR(Pacman)); mvwprintw(win, 13, b-9+3, " ");
        wattron(win, COLOR_PAIR(Pacman)); mvwprintw(win, 13, b-9+5, " ");
        wattron(win, COLOR_PAIR(Pacman)); mvwprintw(win, 13, b-9+7, " ");

        wattron(win, COLOR_PAIR(Pacman)); mvwprintw(win, 13, a-3, "C          ");

        wattron(win, COLOR_PAIR(Pellet)); mvwprintw(win, 13, 23, " ");
        wrefresh(win);
        usleep(100000);
    }

}

/****************************************************************
* Function:    LoadLevel()                                      *
* Parameters:  levelfile: name of file to load                  *
* Returns:     none                                             *
* Description: Open level file and load it into memory          *
****************************************************************/
void LoadLevel(char levelfile[100]) {

    int a = 0; int b = 0;
    size_t l;
    char error[sizeof(LEVEL_ERR)+255] = LEVEL_ERR;    //Let's assume an error
    FILE *fin;                      //New file variable
    Food = 0;                       //Used to count how many food pellets must be eaten

    //Reset defaults
    Dir[0][0] =  1; Dir[0][1] =  0;
    Dir[1][0] = -1; Dir[1][1] =  0;
    Dir[2][0] =  0; Dir[2][1] = -1;
    Dir[3][0] =  0; Dir[3][1] =  1;
    Dir[4][0] =  0; Dir[4][1] = -1;

    //Open file
    fin = fopen(levelfile, "r");

    //Make sure it didn't fail
    if(!(fin)) {
        l = sizeof(error)-strlen(error)-1;
        strncat(error, levelfile, l);
        if(strlen(levelfile) > l) {
            error[sizeof(error)-2] = '.';
            error[sizeof(error)-3] = '.';
            error[sizeof(error)-4] = '.';
        }
        ExitProgram(error);
    }

    //Open file and load the level into the array
    for(a = 0; a < 29; a++) {
            for(b = 0; b < 28; b++) {
                    fscanf(fin, "%d", &Level[a][b]);    //Get character from file
        if(Level[a][b] == 2) { Food++; }    //If it's a pellet, increase the pellet count

        //Store ghosts' and Pacman's locations
        if(Level[a][b] == 5) { Loc[0][0] = a; Loc[0][1] = b; Level[a][b] = 0;}
        if(Level[a][b] == 6) { Loc[1][0] = a; Loc[1][1] = b; Level[a][b] = 0;}
        if(Level[a][b] == 7) { Loc[2][0] = a; Loc[2][1] = b; Level[a][b] = 0;}
        if(Level[a][b] == 8) { Loc[3][0] = a; Loc[3][1] = b; Level[a][b] = 0;}
        if(Level[a][b] == 9) { Loc[4][0] = a; Loc[4][1] = b; Level[a][b] = 0;}
            }
    }

    //All done, now get the difficulty level (AKA level number)
    fscanf(fin, "%d", &LevelNumber);

    //Save initial character positions in case Pacman or Ghosts die
    for(a = 0; a < 5; a++) {
        StartingPoints[a][0] = Loc[a][0]; StartingPoints[a][1] = Loc[a][1];
    }

}

void update_current_state(){

    //Location of Ghosts and Pacman
    memcpy( current_state.Loc, Loc, 5*2*sizeof(int) );

    //Direction of Ghosts and Pacman
    memcpy( current_state.Dir, Dir, 5*2*sizeof(int) );

    //Default location in case Pacman/Ghosts die
    memcpy( current_state.StartingPoints, StartingPoints, 5*2*sizeof(int) );

    //Check for invincibility
    current_state.Invincible = Invincible;

    //Number of pellets left in level
    current_state.Food = Food;

    //Main level array
    memcpy( current_state.Level, Level, 29*28*sizeof(int) );

    //What level number are we on?
    current_state.LevelNumber=LevelNumber;

    //Keep track of how many points to give for eating ghosts
    current_state.GhostsInARow = GhostsInARow;

    //How long left for invincibility
    current_state.tleft = tleft;

    //Initial points
    current_state.Points = Points;

    //Remiaining Lives
    current_state.Lives = Lives;

}


/****************************************************************
* Function:    MainLoop()                                       *
* Parameters:  none                                             *
* Returns:     none                                             *
* Description: Control the main execution of the game           *
****************************************************************/
void MainLoop() {

    DrawWindow();                    //Draw the screen
    wrefresh(win); wrefresh(status); //Refresh it just to make sure
    usleep(1000000);                 //Pause for a second so they know they're about to play

    /* Move Pacman. Move ghosts. Check for extra life awarded
    from points. Pause for a brief moment. Repeat until all pellets are eaten */
    do {

        MovePacman();    DrawWindow();    CheckCollision();
        MoveGhosts();    DrawWindow();    CheckCollision();
        if(Points > FreeLife) { Lives++; FreeLife *= 2;}

         /**
         * AI execution mode
         */
        if(ai_run){

            update_current_state();

            /**
             * ****** HERE IS WHERE YOUR SOLVER IS CALLED
             */
            move_t selected_move = get_next_move( current_state, budget, propagation, ai_stats );

            /**
             * Execute the selected action
             */
            send_action(selected_move);

            if(ai_pause){
                DrawWindow();
                PauseGame();
            }


        }

        Delay();


    } while (Food > 0);

    DrawWindow();                   //Redraw window and...
    usleep(1000000);                //Pause, level complete

}

/****************************************************************
* Function:    MoveGhosts()                                     *
* Parameters:  none                                             *
* Returns:     none                                             *
* Description: Move all ghosts and check for wall collisions    *
****************************************************************/
void MoveGhosts() {

    //Set up some temporary variables
    int a = 0; int b = 0; int c = 0;
    int checksides[] = { 0, 0, 0, 0, 0, 0 };
    static int SlowerGhosts = 0;
    int tmp;

    /* Move ghosts slower when they are vulnerable. This will allow
    the ghosts to move only x times for every y times Pacman moves */
    if(Invincible == 1) {
        SlowerGhosts++;
        if(SlowerGhosts > HOW_SLOW)
            SlowerGhosts = 0;
    }

    //If ghosts are not vulnerable OR the ghosts can move now
    if((Invincible == 0) || SlowerGhosts < HOW_SLOW)

    //Loop through each ghost, one at a time
    for(a = 0; a < 4; a++) {

        //Switch sides? (Transport to other side of screen)
             if((Loc[a][0] ==  0) && (Dir[a][0] == -1)) Loc[a][0] = 28;
        else if((Loc[a][0] == 28) && (Dir[a][0] ==  1)) Loc[a][0] =  0;
        else if((Loc[a][1] ==  0) && (Dir[a][1] == -1)) Loc[a][1] = 27;
        else if((Loc[a][1] == 27) && (Dir[a][1] ==  1)) Loc[a][1] =  0;
        else {

        //Determine which directions we can go
        for(b = 0; b < 4; b++) checksides[b] = 0;
        if(Loc[a][0] == 28) tmp = 0;
        else tmp = Loc[a][0] + 1;
        if(Level[tmp][Loc[a][1]] != 1) checksides[0] = 1;
        if(Loc[a][0] == 0) tmp = 28;
        else tmp = Loc[a][0] - 1;
        if(Level[tmp][Loc[a][1]] != 1) checksides[1] = 1;
        if(Loc[a][1] == 27) tmp = 0;
        else tmp = Loc[a][1] + 1;
        if(Level[Loc[a][0]][tmp] != 1) checksides[2] = 1;
        if(Loc[a][1] == 0) tmp = 27;
        else tmp = Loc[a][1] - 1;
        if(Level[Loc[a][0]][tmp] != 1) checksides[3] = 1;

        //Don't do 180 unless we have to
        c = 0; for(b = 0; b < 4; b++) if(checksides[b] == 1) c++;
        if(c > 1) {
                 if(Dir[a][0] ==  1) checksides[1] = 0;
            else if(Dir[a][0] == -1) checksides[0] = 0;
            else if(Dir[a][1] ==  1) checksides[3] = 0;
            else if(Dir[a][1] == -1) checksides[2] = 0;
        }

        c = 0;
        do {
            //Decide direction, based somewhat-randomly
            b = (int)(rand() / (1625000000 / 4));

            /* Tend to mostly chase Pacman if he is vulnerable
            or run away when he is invincible */
            if(checksides[b] == 1) {
                     if(b == 0) { Dir[a][0] =  1; Dir[a][1] =  0; }
                else if(b == 1) { Dir[a][0] = -1; Dir[a][1] =  0; }
                else if(b == 2) { Dir[a][0] =  0; Dir[a][1] =  1; }
                else if(b == 3) { Dir[a][0] =  0; Dir[a][1] = -1; }
            }
            else {
                if(Invincible == 0) {
                //Chase Pacman
                     if((Loc[4][0] > Loc[a][0]) && (checksides[0] == 1)) { Dir[a][0] =  1; Dir[a][1] =  0; c = 1; }
                else if((Loc[4][0] < Loc[a][0]) && (checksides[1] == 1)) { Dir[a][0] = -1; Dir[a][1] =  0; c = 1; }
                else if((Loc[4][1] > Loc[a][1]) && (checksides[2] == 1)) { Dir[a][0] =  0; Dir[a][1] =  1; c = 1; }
                else if((Loc[4][1] < Loc[a][1]) && (checksides[3] == 1)) { Dir[a][0] =  0; Dir[a][1] = -1; c = 1; }
                }

                else {
                //Run away from Pacman
                     if((Loc[4][0] > Loc[a][0]) && (checksides[1] == 1)) { Dir[a][0] = -1; Dir[a][1] =  0; c = 1; }
                else if((Loc[4][0] < Loc[a][0]) && (checksides[0] == 1)) { Dir[a][0] =  1; Dir[a][1] =  0; c = 1; }
                else if((Loc[4][1] > Loc[a][1]) && (checksides[3] == 1)) { Dir[a][0] =  0; Dir[a][1] = -1; c = 1; }
                else if((Loc[4][1] < Loc[a][1]) && (checksides[2] == 1)) { Dir[a][0] =  0; Dir[a][1] =  1; c = 1; }
                }
            }

        } while ((checksides[b] == 0) && (c == 0));

        //Move Ghost
        Loc[a][0] += Dir[a][0];
        Loc[a][1] += Dir[a][1];
        }
    }
}

/****************************************************************
* Function:    MovePacman()                                     *
* Parameters:  none                                             *
* Returns:     none                                             *
* Description: Move Pacman and check for wall collisions        *
****************************************************************/
void MovePacman() {

    static int itime = 0;

    //Switch sides? (Transport to other side of screen)
         if((Loc[4][0] ==  0) && (Dir[4][0] == -1)) Loc[4][0] = 28;
    else if((Loc[4][0] == 28) && (Dir[4][0] ==  1)) Loc[4][0] =  0;
    else if((Loc[4][1] ==  0) && (Dir[4][1] == -1)) Loc[4][1] = 27;
    else if((Loc[4][1] == 27) && (Dir[4][1] ==  1)) Loc[4][1] =  0;

    //Or
    else {
        //Move Pacman
        Loc[4][0] += Dir[4][0];
        Loc[4][1] += Dir[4][1];

        //If he hit a wall, move back
        if((Level[Loc[4][0]][Loc[4][1]] == 1) || (Level[Loc[4][0]][Loc[4][1]] == 4)) {
            Loc[4][0] -= Dir[4][0];    Loc[4][1] -= Dir[4][1];
        }
    }

    //What is Pacman eating?
    switch (Level[Loc[4][0]][Loc[4][1]]) {
        case 2:    //Pellet
            Level[Loc[4][0]][Loc[4][1]] = 0;
            Points++;
            Food--;
            break;
        case 3:    //PowerUp
            Level[Loc[4][0]][Loc[4][1]] = 0;
            Invincible = 1;
            if(GhostsInARow == 0) GhostsInARow = 1;
            itime = time(0);
            break;
    }

    //Is he invincible?
    if(Invincible == 1)  tleft = (11 - LevelNumber - time(0) + itime);

    //Is invincibility up yet?
    if(tleft < 0) { Invincible = 0; GhostsInARow = 0; tleft = 0; }

}

/****************************************************************
* Function:    PauseGame()                                      *
* Parameters:  none                                             *
* Returns:     none                                             *
* Description: Pause game until user presses a button           *
****************************************************************/
void PauseGame() {

    int chtmp;

    //Display pause dialog
    wattron(win, COLOR_PAIR(Pacman));
    mvwprintw(win, 12, 10, "********");
    mvwprintw(win, 13, 10, "*PAUSED*");
    mvwprintw(win, 14, 10, "********");
    wrefresh(win);

    //And wait until key is pressed
    do {
        chtmp = getch();            //Get input
    } while (chtmp == ERR);

}
