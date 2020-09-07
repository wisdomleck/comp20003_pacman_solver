// Some variables that you may want to change

#ifndef DATAROOTDIR
#    define DATAROOTDIR "."
#endif
#define LEVELS_FILE DATAROOTDIR "/Levels/level__.dat"

#include "utils.h"

#define EXIT_MSG "Good bye!"
#define END_MSG "Game Over"
#define QUIT_MSG "Bye"
#define LEVEL_ERR "Cannot find level file: "
#define LEVEL_WIDTH 28
#define LEVEL_HEIGHT 29
#define HOW_SLOW 3                     //How slow vulnerable ghost move

// To include these global variables
extern propagation_t outputPropagation;
extern int outputBudget;
extern unsigned maxDepth;
extern unsigned totalGenerated;
extern unsigned totalExpanded;
extern double outPutTime;
extern unsigned expandedPerSecond;
extern unsigned outputScore;

void DrawWindowState(state_t state);                         //Refresh display

