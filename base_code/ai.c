#include <time.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <assert.h>

#include "ai.h"
#include "utils.h"
#include "priority_queue.h"
#include "pacman.h"

/* Optimised ai.c, has better performance than the base-line requirements of
   the assignment. Baseline assignment named baseai.c
*/

struct heap h;

/**
 * Function called by pacman.c
*/

void initialize_ai(){
	heap_init(&h);
}

/**
 * function to copy a src into a dst state
*/
void copy_state(state_t* dst, state_t* src){
	//Location of Ghosts and Pacman
	memcpy( dst->Loc, src->Loc, 5*2*sizeof(int) );

    //Direction of Ghosts and Pacman
	memcpy( dst->Dir, src->Dir, 5*2*sizeof(int) );

    //Default location in case Pacman/Ghosts die
	memcpy( dst->StartingPoints, src->StartingPoints, 5*2*sizeof(int) );

    //Check for invincibility
    dst->Invincible = src->Invincible;

    //Number of pellets left in level
    dst->Food = src->Food;

    //Main level array
	memcpy( dst->Level, src->Level, 29*28*sizeof(int) );

    //What level number are we on?
    dst->LevelNumber = src->LevelNumber;

    //Keep track of how many points to give for eating ghosts
    dst->GhostsInARow = src->GhostsInARow;

    //How long left for invincibility
    dst->tleft = src->tleft;

    //Initial points
    dst->Points = src->Points;

    //Remiaining Lives
    dst->Lives = src->Lives;

}

// Creates the initial state
node_t* create_init_node( state_t* init_state ){
	node_t * new_n = (node_t *) malloc(sizeof(node_t));
	new_n->parent = NULL;
	new_n->priority = 0;
	new_n->depth = 0;
	new_n->num_childs = 0;
	copy_state(&(new_n->state), init_state);
	new_n->acc_reward =  get_reward( new_n );
    //New variable
    new_n->current_sum = 0;
	return new_n;

}


// Calculates the heuristic value
float heuristic( node_t* n ){
	float h = 0;
	float i = 0;
    float l = 0;
    float g = 0;

    // Special case of root node, as it has no parent
    if(n->parent == NULL){
        return 0;
    }

    // If a life is lost, a node's parent will have +1 lives than current node
    if((n->parent->state.Lives) > (n->state.Lives)){
        l += 20;
    }

    // If just reached a game over state, then g = 100
    if((n->parent->state.Lives == 0) && ((n->state.Lives) == -1)){
        g += 100;
    }

    // If pacman is just ate a fruit and becomes invincible, i = 10
    if(!(n->parent->state.Invincible) && (n->state.Invincible)){
        i += 10;
    }

    h = i - l - g;

	return h;
}

// Gets the reward value of a node
float get_reward ( node_t* n ){
	float reward = 0;

    // If parent is NULL, the root node's points is 0;
    if(n->parent == NULL){
        reward = 0;
    }

    else{
        reward = heuristic(n) + (n->state.Points - n->parent->state.Points);
    }

    // Changed from 0.99, as avg propagation has better performance. Weights deep moves much less.
	float discount = pow(0.75,n->depth);

	return discount * reward;
}

/**
 * Apply an action to node n and return a new node resulting from executing the action
*/
bool applyAction(node_t* n, node_t** new_node, move_t action ){

	bool changed_dir = false;

    // We want to transfer node n's state into new_node, with an action taken
    copy_state(&((*new_node)->state), &(n->state));

    // Now assign this new node's parent
    (*new_node)->parent = n;

    // Update the new_node with the action chosen
    changed_dir = execute_move_t( &((*new_node)->state), action );
    (*new_node)->move = action;

    // Doesnt matter to run stages 3-6 or not, we delete the node if we can't execute a move
    if(changed_dir){
        // 3.
        // Now, need to find the priority(depth) of the node
        // Essentially depth of parent, +1 as it is a new state now
        (*new_node)->depth = ((*new_node)->parent->depth) + 1;
        (*new_node)->priority = (-1) * (*new_node)->depth;

        // 4, 5.
        // Update the reward
        float reward;
        reward = get_reward(*new_node);
        (*new_node)->acc_reward = (*new_node)->parent->acc_reward + reward;

        // 6.
        // Fill in the rest of the auxiliary data
        (*new_node)->parent->num_childs += 1;
        (*new_node)->num_childs = 0;
        // New attribute
        (*new_node)->current_sum = 0;
    }

	return changed_dir;
}

/**
 * Find best action by building all possible paths up to budget
 * and back propagate using either max or avg
 */

move_t get_next_move( state_t init_state, int budget, propagation_t propagation, char* stats ){
    // We need these for our output!
    //outputBudget = budget;
    //outputPropagation = propagation;

    // This doesn't really matter, but we'll keep it for memories' sake
	move_t best_action = rand() % 4;

    // First assume that every decision is the worst possible, then try improve them
	float best_action_score[4];
	for(unsigned i = 0; i < 4; i++)
	    best_action_score[i] = INT_MIN;

	unsigned generated_nodes = 0;
	unsigned expanded_nodes = 0;
	unsigned max_depth = 0;

	//FILL IN THE GRAPH ALGORITHM
	/* ***************************************************************************************************************** */
    // Create the initial node
    node_t* node = create_init_node( &init_state );
    // Create explored nodes array
    node_t* exploredNodes[5 * budget];
    int numExplored = 0;
    initExploredNodes(exploredNodes, 5 * budget);

    // Make the heap object
    struct heap* heap = (struct heap*)malloc(sizeof(struct heap));
    heap_init(heap);

    // Add the first element in
    heap_push(heap, node);

    // While frontier is not empty
    while(heap->count > 0){
        // Pop off the highest priority element
        node = heap_delete(heap);
        generated_nodes++;
        //totalGenerated++;

        // We've popped off a node to investigate, so add it in
        exploredNodes[numExplored] = node;
        numExplored++;

        // Only consider more nodes if our budget allows it
         if(numExplored < budget){
            // For each possible action,
            for(int i = 0; i < 4; i ++){
                // Make the new node before applying an action
                node_t* newnode = (node_t*)malloc(sizeof(node_t));
                assert(newnode);
                // If the node isn't in explored yet,
                // Issue: If we loop, then we have already visited a node, so everything will be INT_MIN, pick random.
                // If we ever need to run back because of a ghost, we should
                // checking if a node has been explored fucks avg propagation up..?
                if(applyAction(node, &newnode, i) && !hasBeenExplored(exploredNodes, numExplored, newnode)){

                    // Find the max depth
                    if(newnode != NULL && newnode->depth > max_depth){
                        max_depth = newnode->depth;
                    }

                    //Keep track of the global max depth
                    //if(max_depth > maxDepth){
                    //    maxDepth = max_depth;
                    //}

                    // Can propagate up here
                    if(propagation == max){
                        maxPropagate(newnode, best_action_score);
                    }
                    else if(propagation == avg){
                        avgPropagate(newnode, best_action_score);
                    }

                    // If the state before had more lives, means we just lost a life. Delete state
                    if((newnode->parent->state.Lives) > (newnode->state.Lives)){
                        free(newnode);
                    }

                    else{
                        heap_push(heap, newnode);
                    }
                }

                else{
                    //The move can't be made, or is redundant
                    free(newnode);
                }
            }
            expanded_nodes++;
            //totalExpanded++;
        }
    }

    // Add 1 to account for 0th element, or root node
    expanded_nodes += 1;

    // Check if the next node has a food or not. If it does, add value to that move. Removes oscillations
    // Probe if the next square has food or not. Can tell if food on init_state > applyAction food levels
    // Stops the oscillating, as it persuades pacman to start eating the nearest food
    for(int i = 0; i < 4; i ++){
        state_t tempState;
        copy_state(&tempState, &init_state);
        int legitMove = execute_move_t(&tempState, i);
        if(legitMove && init_state.Food > tempState.Food){
            best_action_score[i] += 1;
        }
    }


     // finds the maximum value of the best action first.
    float maxReward = INT_MIN;
    int currBestAction = rand() % 4;
    for(int j = 0; j < 4; j++){
        if(best_action_score[j] > maxReward){
            maxReward = best_action_score[j];
            currBestAction = j;
        }
    }

    // loop through to see if there are any ties, if there are, randomly select one. Fails for 2+ ties
    for(int k = 0; k < 4; k++){
        if(best_action_score[currBestAction] - best_action_score[k] < 0.000001){
            //Equal, so flip for winner
            if(k != currBestAction){
                if((rand() % 10) >= 5){
                    currBestAction = k;
                }
            }
        }
    }

    // Found the best move!
    best_action = currBestAction;

    // Free the explored array
    for(int i = 0; i < numExplored; i++){
        free(exploredNodes[i]);
    }

    //We're done with the heap, so free it properly
    free(heap->heaparr);
    free(heap);


    /* ***************************************************************************************************************** */

	sprintf(stats, "Max Depth: %d Expanded nodes: %d  Generated nodes: %d\n",max_depth,expanded_nodes,generated_nodes);

	if(best_action == left)
		sprintf(stats, "%sSelected action: Left, Value: %lf\n",stats, best_action_score[left]);
	if(best_action == right)
		sprintf(stats, "%sSelected action: Right, Value: %lf\n",stats, best_action_score[right]);
	if(best_action == up)
		sprintf(stats, "%sSelected action: Up, Value: %lf\n",stats, best_action_score[up]);
	if(best_action == down)
		sprintf(stats, "%sSelected action: Down, Value: %lf\n",stats, best_action_score[down]);

	sprintf(stats, "%sScore Left %f Right %f Up %f Down %f",stats,best_action_score[left],best_action_score[right],best_action_score[up],best_action_score[down]);

	return best_action;
}

void
initExploredNodes(node_t** exploredNodes, int size){
    for(int i = 0; i < size; i++){
        exploredNodes[i] = NULL;
    }
}

void
maxPropagate(node_t* node, float best_action_score[]){
    // The initial max is the current node's acc_reward
    float maxReward = INT_MIN;

    node_t* temp = node;
    node_t* prev = NULL;

    // While it isn't the root node
     while(temp->parent != NULL){
        if(temp->acc_reward > maxReward){
            maxReward = temp->acc_reward;
        }
        prev = temp;
        temp = temp->parent;
    }

    // Once here, we are at the initial node and prev is the first action following.
    prev->acc_reward = maxReward;

    // If the next move generates a new max reward, assign it in the array
    if(prev->acc_reward > best_action_score[prev->move]){
        best_action_score[prev->move] = prev->acc_reward;
    }
}




void
avgPropagate(node_t* node, float best_action_score[]){
    node_t* temp = node;

    while(temp->parent != NULL){
        temp->parent->current_sum = ((temp->parent->acc_reward * (temp->parent->num_childs - 1)) + temp->acc_reward) / temp->parent->num_childs;
        temp->parent->acc_reward = temp->parent->current_sum;

        // If we are at the next action node, save the new value into best_action_score
        if(temp->depth == 1){
            best_action_score[temp->move] = temp->acc_reward;
        }

        temp = temp->parent;
    }

}

int
hasBeenExplored(node_t* exploredNodes[], int numExplored, node_t* node){
    for(int i = 0; i < numExplored; i++){
        if(compareNodes(exploredNodes[i], node)){
            return 1;
        }
    }
    return 0;
}


int
compareNodes(node_t* node1, node_t* node2){
    //Compare the food and pacman location
    if(node1->state.Loc[4][0] == node2->state.Loc[4][0] && node1->state.Loc[4][1] == node2->state.Loc[4][1]){
        if(node1->state.Food == node2->state.Food){
            return 1;
        }
    }
    return 0;
}
