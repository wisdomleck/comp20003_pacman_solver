#ifndef __AI__
#define __AI__

#include <stdint.h>
#include <unistd.h>
#include "node.h"
#include "priority_queue.h"


void initialize_ai();

move_t get_next_move( state_t init_state, int budget, propagation_t propagation, char* stats );

float get_reward( node_t* n );
int compareNodes(node_t* node1, node_t* node2);
int hasBeenExplored(node_t* exploredNodes[], int numExplored, node_t* node);
void initExploredNodes(node_t** exploredNodes, int size);
void maxPropagate(node_t* node, float best_action_score[]);
void avgPropagate(node_t* node, float best_action_score[]);

#endif
