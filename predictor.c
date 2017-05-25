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
#include "wperceptron.h"
#include "perceptron.h"
#include "neural.h"
#include "tage.h"

//
// TODO:Student Information
//
const char *studentName = "Che Liu";
const char *studentID = "A53209595";
const char *email = "chl730@eng.ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = {"Static", "Gshare",
                         "Tournament", "Custom"};

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
uint8_t *gshareBHT;

uint32_t *localPHT;
uint8_t *localBHT;
uint8_t *choicePT;
uint32_t globalhistory;
uint8_t *globalBHT;
uint8_t localOutcome, globalOutcome;

#define USE_NEURAL1


// Shifting
void shift_prediction(uint8_t *satuate, uint8_t outcome) {
    if (outcome == NOTTAKEN) {
        if (*satuate != SN) {
            (*satuate)--;
        }
    } else {
        if (*satuate != ST) {
            (*satuate)++;
        }
    }
}


uint8_t get_gshare_prediction(uint32_t pc) {
    uint32_t gBHTIndex = (ghistory ^ pc) & ((1 << ghistoryBits) - 1);
    uint8_t gPrediction = gshareBHT[gBHTIndex];
    globalOutcome = (gPrediction == WN || gPrediction == SN) ? NOTTAKEN : TAKEN;
    return globalOutcome;
}

uint8_t get_local_prediction(uint32_t pc) {
    uint32_t localPHTIndex = pc & ((1 << pcIndexBits) - 1);
    uint32_t localBHTIndex = localPHT[localPHTIndex];
    uint8_t localPrediction = localBHT[localBHTIndex];
    localOutcome = ((localPrediction == WN || localPrediction == SN) ? NOTTAKEN : TAKEN);
    return localOutcome;
}

uint8_t get_global_prediction(uint32_t pc) {
    uint32_t gBHTIndex = (globalhistory) & ((1 << ghistoryBits) - 1);
    uint8_t gPrediction = globalBHT[gBHTIndex];
    globalOutcome = ((gPrediction == WN || gPrediction == SN) ? NOTTAKEN : TAKEN);
    return globalOutcome;
}

uint8_t get_tournament_prediction(uint32_t pc) {
    uint32_t gBHTIndex = (globalhistory) & ((1 << ghistoryBits) - 1);
    uint32_t predictor = choicePT[gBHTIndex];
    get_global_prediction(pc);
    get_local_prediction(pc);

    if (predictor == WN || predictor == SN) {     // Negtive means global predictor
        return globalOutcome;
    } else {                                      // Positive means local predictor
        return localOutcome;
    }

}

void tournament_init() {
    localBHT = malloc((1 << lhistoryBits) * sizeof(uint8_t));
    localPHT = malloc((1 << pcIndexBits) * sizeof(uint32_t));
    choicePT = malloc((1 << ghistoryBits) * sizeof(uint8_t));
    memset(localBHT, WN, (1 << lhistoryBits) * sizeof(uint8_t));
    memset(localPHT, 0, (1 << pcIndexBits) * sizeof(uint32_t));
    memset(choicePT, WN, (1 << ghistoryBits) * sizeof(uint8_t));
    globalhistory = 0;
    globalBHT = malloc((1 << ghistoryBits) * sizeof(uint8_t));
    memset(globalBHT, WN, (1 << ghistoryBits) * sizeof(uint8_t));
}


void tournament_update(uint32_t pc, uint8_t outcome) {

    if (localOutcome != globalOutcome) {
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
    return;
}


// Initialize the predictor
//
void
init_predictor() {
    //
    //TODO: Initialize Branch Predictor Data Structures
    switch (bpType) {
        case STATIC:
            return;
        case TOURNAMENT:
            tournament_init();
            break;

        case GSHARE:
            ghistory = 0;
            gshareBHT = malloc((1 << ghistoryBits) * sizeof(uint8_t));
            memset(gshareBHT, WN, (1 << ghistoryBits) * sizeof(uint8_t));
            break;
        case CUSTOM:

            //neural_path_init();
            tage_init();
            //wp_init();
            //perceptron_init();


        default:
            break;
    }


}


// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc) {
    //
    //TODO: Implement prediction scheme
    //
    if (bpType == STATIC) {
        return TAKEN;

    } else if (bpType == GSHARE) {
        return get_gshare_prediction(pc);

    } else if (bpType == TOURNAMENT) {
        return get_tournament_prediction(pc);

    } else if (bpType == CUSTOM) {

        //return get_neural_prediction(pc);

        return tage_predict(pc);
        //return wp_perdict(pc);
        //return perceptron_predict(pc);

    } else {
        return NOTTAKEN;
    }

}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void
train_predictor(uint32_t pc, uint8_t outcome) {
    //
    //TODO: Implement Predictor training
    //

    switch (bpType) {
        case STATIC:
            break;
        case TOURNAMENT:
            tournament_update(pc, outcome);
            break;
        case GSHARE:
            shift_prediction(&gshareBHT[(ghistory ^ pc) & ((1 << ghistoryBits) - 1)], outcome);
            ghistory <<= 1;
            ghistory |= outcome;
            break;
        case CUSTOM:
            //neural_train(pc, outcome);
            tage_train(pc, outcome);
            //wp_train(pc, outcome);
            //perceptron_train(pc, outcome);
        default:
            break;
    }

    // If there is not a compatable bpType then return NOTTAKEN
    return;
}
