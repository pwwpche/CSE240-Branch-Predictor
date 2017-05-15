//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <string.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "Che Liu";
const char *studentID   = "A53209595";
const char *email       = "chl730@eng.ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
//TODO: Add your own Branch Predictor data structures here
//
uint32_t ghistory;
uint8_t* gshareBHT;

uint32_t* localPHT;
uint8_t* localBHT;
uint8_t* choicePT;
uint32_t globalhistory;
uint8_t* globalBHT;
uint8_t localOutcome, globalOutcome;

// Data for path based neural predictor
#define n_PCSIZE 399
#define n_HISTORYLEN 19
#define n_SATUATELEN 8

#define MASK_PC(x) (x % n_PCSIZE)


int16_t n_W[n_PCSIZE][n_HISTORYLEN + 1];
int8_t n_gHistory[n_HISTORYLEN];
int32_t n_shiftWeight[n_HISTORYLEN + 1];
uint32_t n_branches[n_HISTORYLEN + 1];

uint8_t n_recentPrediction = NOTTAKEN;
uint8_t n_needTrain = 0;
int32_t n_trainTheta = 0;

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//



//====================== Neural Path-based Predictor ==============================

void neural_shift(int16_t* satuate, uint8_t same){
  if(same){
    if(*satuate != (1 << (n_SATUATELEN - 1) - 1)){
      (*satuate)++;
    }
  }else{
    if(*satuate != -(1 << (n_SATUATELEN - 1) )){
      (*satuate)--;
    }
  }
}


void neural_path_init(){
  //printf("neural: historylen: %d, PCSIZE: %d, saturate=%d\n", n_HISTORYLEN, n_PCSIZE, n_SATUATELEN);
  n_trainTheta = (int32_t)(2.14 * (n_HISTORYLEN + 1) + 20.58);
  memset(n_W, 0, sizeof(int16_t) * n_PCSIZE * (n_HISTORYLEN + 1));
  memset(n_shiftWeight, 0, sizeof(int32_t) * (n_HISTORYLEN + 1));
  memset(n_branches, 0, sizeof(uint32_t) * (n_HISTORYLEN + 1));
  memset(n_gHistory, 0, sizeof(uint8_t) * n_HISTORYLEN);
}

uint8_t get_neural_prediction(uint32_t pc){
  int32_t out = (int32_t)(n_W[MASK_PC(pc)][0]) + n_shiftWeight[n_HISTORYLEN];
  n_recentPrediction = (out >= 0) ? TAKEN : NOTTAKEN;
  n_needTrain = (out < n_trainTheta && out > -n_trainTheta) ? 1 : 0;
  return n_recentPrediction;
}



void neural_train(uint32_t pc, uint8_t outcome){

  // Add this branch pc to branch history list
  for(int i = n_HISTORYLEN; i >= 1 ; i--){
    n_branches[i] = n_branches[i - 1];
  }
  n_branches[0] = MASK_PC(pc);

  // Update cumulative array
  for (int i = n_HISTORYLEN ; i >= 1 ; i-- ) {
			n_shiftWeight[i] = n_shiftWeight[i - 1] +
          ((outcome == TAKEN) ? n_W[MASK_PC(pc)][n_HISTORYLEN - i + 1] : -n_W[MASK_PC(pc)][n_HISTORYLEN - i + 1]);
	}
	n_shiftWeight[0] = 0;
  // perceptron learning rule
	if ((outcome != n_recentPrediction) || n_needTrain) {
		neural_shift(&(n_W[MASK_PC(pc)][0]), outcome);
    for (int i = 1 ; i <= n_HISTORYLEN ; i++ ) {
  		uint32_t k = MASK_PC(n_branches[i]);
      uint8_t predict = n_gHistory[i-1];
			neural_shift(&(n_W[k][i]), (outcome == predict));
		}
	}


  // Update global history register
  for(int i = n_HISTORYLEN - 1; i > 0 ; i--){
    n_gHistory[i] = n_gHistory[i-1];
  }
  n_gHistory[0] = outcome;

}


// Shifting
void shift_prediction(uint8_t* satuate, uint8_t outcome){
  if(outcome == NOTTAKEN){
    if (*satuate != SN){
      (*satuate)--;
    }
  }else{
    if (*satuate != ST){
      (*satuate)++;
    }
  }
}


uint8_t get_gshare_prediction(uint32_t pc){
  uint32_t gBHTIndex = (ghistory ^ pc) & ((1 << ghistoryBits) - 1);
  uint8_t gPrediction = gshareBHT[gBHTIndex];
   globalOutcome = (gPrediction == WN || gPrediction == SN) ? NOTTAKEN : TAKEN;
  return globalOutcome;
}

uint8_t get_local_prediction(uint32_t pc){
  uint32_t localPHTIndex = pc & ((1 << pcIndexBits) - 1);
  uint32_t localBHTIndex = localPHT[localPHTIndex];
  uint8_t localPrediction = localBHT[localBHTIndex];
  localOutcome = ((localPrediction == WN || localPrediction == SN) ? NOTTAKEN : TAKEN);
  return localOutcome;
}

uint8_t get_global_prediction(uint32_t pc){
  uint32_t gBHTIndex = (globalhistory) & ((1 << ghistoryBits) - 1);
  uint8_t gPrediction = globalBHT[gBHTIndex];
  globalOutcome = ((gPrediction == WN || gPrediction == SN) ? NOTTAKEN : TAKEN);
  return globalOutcome;
}

uint8_t get_tournament_prediction(uint32_t pc){
  uint32_t predictor = choicePT[globalhistory];
  get_global_prediction(pc);
  get_local_prediction(pc);

  if(predictor == WN || predictor == SN){     // Negtive means global predictor
    return globalOutcome;
  }else{                                      // Positive means local predictor
    return localOutcome;
  }

}

void tournament_init(){
  localBHT = malloc((1 << lhistoryBits) * sizeof(uint8_t));
  localPHT = malloc((1 << pcIndexBits)  * sizeof(uint32_t));
  choicePT = malloc((1 << ghistoryBits)  * sizeof(uint8_t));
  memset(localBHT, WN, (1 << lhistoryBits) * sizeof(uint8_t));
  memset(localPHT, 0, (1 << pcIndexBits) * sizeof(uint32_t));
  memset(choicePT, WN, (1 << ghistoryBits) * sizeof(uint8_t));
  globalhistory = 0;
  globalBHT = malloc((1 << ghistoryBits) * sizeof(uint8_t));
  memset(globalBHT, WN, (1 << ghistoryBits) * sizeof(uint8_t));
}


void tournament_update(uint32_t pc, uint8_t outcome){
  if(localOutcome != globalOutcome){
    shift_prediction(&choicePT[globalhistory],
                                (localOutcome == outcome) ? TAKEN : NOTTAKEN
                    );
  }
  uint32_t localPHTIndex = pc & ((1 << pcIndexBits) - 1);
  uint32_t localBHTIndex = localPHT[localPHTIndex];

  shift_prediction(&(localBHT[localBHTIndex]), outcome);
  localPHT[localPHTIndex] <<= 1;
  localPHT[localPHTIndex] &= ((1 << lhistoryBits) - 1);
  localPHT[localPHTIndex] |= outcome;
  shift_prediction(&globalBHT[globalhistory], outcome);
  globalhistory <<= 1;
  globalhistory &= ((1 << ghistoryBits) - 1);
  globalhistory |= outcome;
  return ;
}


// Initialize the predictor
//
void
init_predictor()
{
  //
  //TODO: Initialize Branch Predictor Data Structures
  switch (bpType) {
    case STATIC:
      return ;
    case TOURNAMENT:
      tournament_init();
      break;

    case GSHARE:
      ghistory = 0;
      gshareBHT = malloc((1 << ghistoryBits) * sizeof(uint8_t));
      memset(gshareBHT, WN, (1 << ghistoryBits) * sizeof(uint8_t));
      break;
    case CUSTOM:

      neural_path_init();
    default:
      break;
  }



}


// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{
  //
  //TODO: Implement prediction scheme
  //
  if(bpType == STATIC){
    return TAKEN;

  }else if(bpType == GSHARE){
    return get_gshare_prediction(pc);

  }else if(bpType == TOURNAMENT){
    return get_tournament_prediction(pc);

  }else if(bpType == CUSTOM){
    return get_neural_prediction(pc);
  }else{
    return NOTTAKEN;
  }

}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void
train_predictor(uint32_t pc, uint8_t outcome)
{
  //
  //TODO: Implement Predictor training
  //

  switch (bpType) {
    case STATIC:
      break ;
    case TOURNAMENT:
      tournament_update(pc, outcome);
      break;
    case GSHARE:
      shift_prediction(&gshareBHT[(ghistory ^ pc) & ((1 << ghistoryBits) - 1)], outcome);
      ghistory <<= 1;
      ghistory |= outcome;
      break;
    case CUSTOM:
      neural_train(pc, outcome);
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return ;
}
