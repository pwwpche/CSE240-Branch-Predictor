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
uint8_t ghistory;
uint8_t* gshareBHT;

uint32_t* localPHT;
uint8_t* localBHT;
uint8_t* choicePT;

// Data for perceptron predictor

// Notice: PCLEN usually should be smaller than 16.
#define p_PCLEN 8
#define p_HEIGHT 30
#define p_SATUATELEN 8

int8_t p_W[(1 << p_PCLEN)][p_HEIGHT + 1];
int32_t p_gHistory;
int32_t p_train_theta;
uint8_t p_recent_prediction = NOTTAKEN;
uint8_t p_need_train = 0;
int32_t p_last_out = 0;



// Data for path based neural predictor
#define n_PCLEN 8
#define n_HISTORYLEN 30
#define n_SATUATELEN 8

#define MASK_PC(x) (x & ((1 << n_PCLEN) - 1))

int8_t n_W[(1 << n_PCLEN)][n_HISTORYLEN + 1];
int8_t n_gHistory;
int32_t n_shiftWeight[n_HISTORYLEN + 1];
uint32_t n_branches[n_HISTORYLEN + 1];

uint8_t n_recentPrediction = NOTTAKEN;
uint8_t n_needTrain = 0;
int32_t n_trainTheta = 0;

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

//======================Perceptron Predictor==================================

void perceptron_shift(int8_t* satuate, uint8_t same){
  if(same){
    if(*satuate != (1 << (p_SATUATELEN - 1) - 1)){
      (*satuate)++;
    }
  }else{
    if(*satuate != -(1 << (p_SATUATELEN - 1 ) )){
      (*satuate)--;
    }
  }
}

void perceptron_init(){
  printf("percep: PC_len: %d\theight: %d\tsatuate_len: %d\n", p_PCLEN, p_HEIGHT, p_SATUATELEN);
  p_train_theta = (int32_t)(1.93 * p_HEIGHT + 14);
  memset(p_W, 0, sizeof(int8_t) * (1 << p_PCLEN) * (p_HEIGHT + 1));
  p_gHistory = 0;
}



uint8_t get_perceptron_prediction(uint32_t pc){
  uint32_t index = pc & ((1 << p_PCLEN) - 1);
  int16_t out = p_W[index][0];

  for(int i = 1 ; i <= p_HEIGHT ; i++){
    out += (p_gHistory & (1 << i - 1)) ?
          p_W[index][i] : -p_W[index][i];
  }

  if(out >= 0){
    p_recent_prediction = TAKEN;
  }else{
    p_recent_prediction = NOTTAKEN;
  }
  p_last_out = out;
  if(out < p_train_theta && out > -p_train_theta){
    p_need_train = 1;
  }else{
    p_need_train = 0;
  }

  return p_recent_prediction;
}

void perceptron_train(uint32_t pc, uint8_t outcome){

  uint32_t index = pc & ((1 << p_PCLEN) - 1);

  if((p_recent_prediction != outcome) || p_need_train){
    perceptron_shift(&(p_W[index][0]), outcome);
    for(int i = 1 ; i <= p_HEIGHT ; i++){
      uint8_t predict = (p_gHistory & (1 << i-1)) >> (i-1);
      perceptron_shift(&(p_W[index][i]), (outcome == predict));
    }
  }
  p_gHistory <<= 1;
  p_gHistory = p_gHistory & ((1 << p_HEIGHT) - 1);
  p_gHistory |= outcome;
}
//====================== end of Perceptron Predictor ==============================


//====================== Neural Path-based Predictor ==============================

void neural_shift(int8_t* satuate, uint8_t same){
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
  printf("neural: historylen: %d, pclen: %d\n", n_HISTORYLEN, n_PCLEN);
  n_trainTheta = (int32_t)(2.14 * (n_HISTORYLEN + 1) + 20.58);
  memset(n_W, 0, sizeof(int8_t) * (1 << n_PCLEN) * (n_HISTORYLEN + 1));
  memset(n_shiftWeight, 0, sizeof(int32_t) * (n_HISTORYLEN + 1));
  memset(n_branches, 0, sizeof(uint32_t) * (n_HISTORYLEN + 1));
  n_gHistory = 0;
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
      uint8_t predict = (n_gHistory & (1 << i-1)) >> (i-1);
			neural_shift(&(n_W[k][i]), (outcome == predict));
		}
	}


  // Update global history register
  n_gHistory <<= 1;
  n_gHistory = n_gHistory & ((1 << n_HISTORYLEN) - 1);
  n_gHistory |= outcome;

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
        localBHT = malloc((1 << lhistoryBits) * sizeof(uint8_t));
        localPHT = malloc((1 << pcIndexBits)  * sizeof(uint32_t));
        choicePT = malloc((1 << pcIndexBits)  * sizeof(uint8_t));
        memset(localBHT, 1, (1 << lhistoryBits) * sizeof(uint8_t));
        memset(localPHT, 0, (1 << pcIndexBits) * sizeof(uint32_t));
        memset(choicePT, 1, (1 << pcIndexBits) * sizeof(uint8_t));

    case GSHARE:
      ghistory = 0;
      gshareBHT = malloc((1 << ghistoryBits) * sizeof(uint8_t));
      memset(gshareBHT, 1, (1 << ghistoryBits) * sizeof(uint8_t));
      break;
    case CUSTOM:
      perceptron_init();
      neural_path_init();
    default:
      break;
  }



}


uint8_t get_local_prediction(uint32_t pc){
  uint32_t localPHTIndex = pc & ((1 << pcIndexBits) - 1);
  uint32_t localBHTIndex = localPHT[localPHTIndex] & ((1 << lhistoryBits) - 1);
  uint8_t localPrediction = localBHT[localBHTIndex];
  return (localPrediction == WN || localPrediction == SN) ? NOTTAKEN : TAKEN;
}

uint8_t get_gshare_prediction(uint32_t pc){
  uint32_t gBHTIndex = (ghistory ^ pc) & ((1 << ghistoryBits) - 1);
  uint8_t gPrediction = gshareBHT[gBHTIndex];
  return (gPrediction == WN || gPrediction == SN) ? NOTTAKEN : TAKEN;
}

uint8_t get_tournament_prediction(uint32_t pc){
  uint32_t predictor = choicePT[pc & ((1 << pcIndexBits) - 1)];
  if(predictor == WN || predictor == SN){     // Negtive means global predictor
    return get_gshare_prediction(pc);
  }else{                                      // Positive means local predictor
    return get_local_prediction(pc);
  }
}


// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint32_t index = 0;
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

  if(bpType == TOURNAMENT){
    //////printf("pc: %x\tghistory: %x\tprediction: %d\toutcome: %d \n", pc, ghistory, get_tournament_prediction(pc),outcome);
    uint8_t localPrediction = get_local_prediction(pc);
    uint8_t gsharePrediction = get_gshare_prediction(pc);
    if(localPrediction != gsharePrediction){
      if(localPrediction == outcome){
        shift_prediction(&choicePT[pc & ((1 << pcIndexBits) - 1)], TAKEN);
      }else{
        shift_prediction(&choicePT[pc & ((1 << pcIndexBits) - 1)], NOTTAKEN);
      }
    }
  }

  switch (bpType) {
    case STATIC:
      break ;

    case TOURNAMENT:
      shift_prediction(&localBHT[localPHT[pc & ((1 << pcIndexBits) - 1)]], outcome);
      localPHT[pc & ((1 << pcIndexBits) - 1)] <<= 1;
      localPHT[pc & ((1 << pcIndexBits) - 1)] &= ((1 << lhistoryBits) - 1);
      if(outcome == TAKEN){
        localPHT[pc & ((1 << pcIndexBits) - 1)]++;
      }
    case GSHARE:

      shift_prediction(&gshareBHT[(ghistory ^ pc) & ((1 << ghistoryBits) - 1)], outcome);
      ghistory <<= 1;
      if(outcome == TAKEN){
        ghistory++;
      }
      break;
    case CUSTOM:
      //perceptron_train(pc, outcome);
      neural_train(pc, outcome);
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return ;
}
