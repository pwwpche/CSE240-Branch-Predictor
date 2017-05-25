//
// Created by pwwpche on 5/22/17.
//

#ifndef TAGE_MASTER_TAGE_H
#define TAGE_MASTER_TAGE_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define NOTTAKEN  0
#define TAKEN     1

// 7 Banks plus a basic bimodal predictor
#define NUM_BANKS 7

#define LEN_BIMODAL 13
#define BIMODAL_INDEX(pc) (pc & ((1 << (LEN_BIMODAL)) - 1))

#define LEN_GLOBAL 9
#define LEN_TAG 12
#define LEN_COUNTS 3
#define MIN_HISTORY_LEN 5
#define MAX_HISTORY_LEN 131

// i = 0 ... NUM_BANKS - 1
// geo_i = (int) (MIN_LEN * ((MAX_LEN / MIN_LEN) ^ (i / (NUM_BANKS - 1))) + 0.5)
const uint8_t GEOMETRICS[NUM_BANKS] = {130, 76, 44, 26, 15, 9, 5};

// SaturateCounter is LEN_COUNTS=3 bits
// Tag is LEN_TAG = 12 bits
// Usefulness is 2 bits
typedef struct BankEntryStruct{
    int8_t saturateCounter;
    uint16_t tag;
    int8_t usefulness;
} BankEntry ;

typedef struct CompressedStruct{
    int8_t geometryLength;
    int8_t targetLength;
    uint32_t compressed;
} CompressedHistory ;

typedef struct BankStruct{
    int geometry;
    BankEntry entry[1 << LEN_GLOBAL];
    CompressedHistory indexCompressed;
    CompressedHistory tagCompressed[2];

} Bank;

int8_t t_bimodalPredictor[1 << LEN_BIMODAL];    // Bimodal Predictor table
uint8_t t_globalHistory[MAX_HISTORY_LEN];             // Global History Register
uint32_t t_pathHistory;                               // Path History Register

Bank tageBank[NUM_BANKS];
uint8_t primaryBank = NUM_BANKS;
uint8_t alternateBank = NUM_BANKS;
uint8_t primaryPrediction = NOTTAKEN;
uint8_t alternatePrediction = NOTTAKEN;
uint8_t lastPrediction = NOTTAKEN;

uint32_t bankGlobalIndex[NUM_BANKS];
uint32_t instrIndex = 0;
int8_t useAlternate = 8;



uint8_t t_getBimodalPrediction(uint32_t pc){
    return (uint8_t) ((t_bimodalPredictor[BIMODAL_INDEX(pc)] > 1) ? TAKEN : NOTTAKEN);
}

void t_updateCompressed(CompressedHistory* history, uint8_t* global){
    uint32_t newCompressed = (history->compressed << 1) + global[0];
    newCompressed ^=  global[history->geometryLength] << (history->geometryLength % history->targetLength);
    newCompressed ^= (newCompressed >> history->targetLength);
    newCompressed &= (1 << history->targetLength) - 1;
    history->compressed = newCompressed;

}

//  tag computation
uint16_t generateGlobalEntryTag(uint32_t pc, int bankIndex) {
    int tag = pc ^(tageBank[bankIndex].tagCompressed[0].compressed) ^((tageBank[bankIndex].tagCompressed[1].compressed) << 1);
    return (uint16_t) (tag & ((1 << (LEN_TAG - ((bankIndex + (NUM_BANKS & 1)) / 2))) - 1));
    //does not use the same length for all the components
}

void updateSaturate(int8_t *saturate, int taken, int nbits) {
    if (taken) {
        if ((*saturate) < ((1 << (nbits - 1)) - 1)) {
            (*saturate)++;
        }
    } else {
        if ((*saturate) > -(1 << (nbits - 1))) {
            (*saturate)--;
        }
    }
}

void updateSaturateMinMax(int8_t *saturate, int taken, int min, int max) {
    if (taken) {
        if ((*saturate) < max) {
            (*saturate)++;
        }
    } else {
        if ((*saturate) > min) {
            (*saturate)--;
        }
    }
}

int F(int A, int size, int bank) {
    int A1, A2;

    A = A & ((1 << size) - 1);
    A1 = (A & ((1 << LEN_GLOBAL) - 1));
    A2 = (A >> LEN_GLOBAL);
    A2 = ((A2 << bank) & ((1 << LEN_GLOBAL) - 1)) + (A2 >> (LEN_GLOBAL - bank));
    A = A1 ^ A2;
    A = ((A << bank) & ((1 << LEN_GLOBAL) - 1)) + (A >> (LEN_GLOBAL - bank));
    return (A);
}

uint32_t getGlobalIndex(uint32_t pc, int bankIdx) {
    int index;
    if (tageBank[bankIdx].geometry >= 16)
        index =
                pc ^ (pc >> ((LEN_GLOBAL - (NUM_BANKS - bankIdx - 1)))) ^ tageBank[bankIdx].indexCompressed.compressed
                   ^ F(t_pathHistory, 16, bankIdx);

    else
        index =
                pc ^ (pc >> (LEN_GLOBAL - NUM_BANKS + bankIdx + 1)) ^
                        tageBank[bankIdx].indexCompressed.compressed ^
                        F(t_pathHistory, tageBank[bankIdx].geometry, bankIdx);

    return (uint32_t) (index & ((1 << (LEN_GLOBAL)) - 1));

}

void tage_init(){
    memset(t_bimodalPredictor, 1, sizeof(uint8_t) * (1 << LEN_BIMODAL));
    for(uint32_t i = 0 ; i < NUM_BANKS ; i++){
        tageBank[i].geometry = GEOMETRICS[i];

        //TODO: Reconfirm length of compressed history.
        tageBank[i].indexCompressed.compressed = 0;
        tageBank[i].indexCompressed.geometryLength = GEOMETRICS[i];
        tageBank[i].indexCompressed.targetLength = LEN_GLOBAL;

        tageBank[i].tagCompressed[0].compressed = 0;
        tageBank[i].tagCompressed[0].geometryLength = GEOMETRICS[i];
        tageBank[i].tagCompressed[0].targetLength = (int8_t) (LEN_TAG - ((i + (NUM_BANKS & 1)) / 2));
        tageBank[i].tagCompressed[1].compressed = 0;
        tageBank[i].tagCompressed[1].geometryLength = GEOMETRICS[i];
        tageBank[i].tagCompressed[1].targetLength = (int8_t) (LEN_TAG - ((i + (NUM_BANKS & 1)) / 2) - 1);

        for(uint32_t j = 0 ; j < (1 << LEN_GLOBAL) ; j++){
            tageBank[i].entry[j].saturateCounter = 0;
            tageBank[i].entry[j].tag = 0;
            tageBank[i].entry[j].usefulness = 0;
        }

    }

    memset(bankGlobalIndex, 0, sizeof(uint32_t) * NUM_BANKS);
    memset(t_globalHistory, 0, sizeof(uint8_t) * MAX_HISTORY_LEN);
    instrIndex = 0;
    t_pathHistory = 0;
    srand((unsigned int) time(NULL));
}



uint8_t tage_predict(uint32_t pc){

    int tagResult[NUM_BANKS];

    for(uint32_t i = 0 ; i < NUM_BANKS ; i++){
        tagResult[i] = generateGlobalEntryTag(pc, i);
        bankGlobalIndex[i] = getGlobalIndex(pc, i);
    }

    primaryPrediction = NOTTAKEN;
    alternatePrediction = NOTTAKEN;
    primaryBank = NUM_BANKS;
    alternateBank = NUM_BANKS;

    for(uint8_t i = 0 ; i < NUM_BANKS ; i++){
        if(tageBank[i].entry[bankGlobalIndex[i]].tag == tagResult[i]){
            primaryBank = i; break;
        }
    }

    for(uint8_t i = primaryBank + 1 ; i < NUM_BANKS ; i++){
        if(tageBank[i].entry[bankGlobalIndex[i]].tag == tagResult[i]){
            alternateBank = i; break;
        }
    }

    if (primaryBank < NUM_BANKS) {
        if (alternateBank < NUM_BANKS) {
            alternatePrediction = ((tageBank[alternateBank].entry[bankGlobalIndex[alternateBank]]
                                                                    .saturateCounter >= 0)
                                              ? TAKEN : NOTTAKEN);
        }else {
            alternatePrediction = t_getBimodalPrediction(pc);
        }
        //if the entry is recognized as a newly allocated entry and
        //counter PWIN is negative use the alternate prediction
        // see section 3.2.4
        if((tageBank[primaryBank].entry[bankGlobalIndex[primaryBank]].saturateCounter != 0) ||
           (tageBank[primaryBank].entry[bankGlobalIndex[primaryBank]].saturateCounter != 1) ||
           (tageBank[primaryBank].entry[bankGlobalIndex[primaryBank]].usefulness != 0) ||
           (useAlternate < 8)
                ){
            lastPrediction = (tageBank[primaryBank].entry[bankGlobalIndex[primaryBank]].saturateCounter >= 0) ? TAKEN : NOTTAKEN;
        }
        else {
            lastPrediction = alternatePrediction;
        }

    } else {
        alternatePrediction = t_getBimodalPrediction(pc);
        lastPrediction = alternatePrediction;
    }

    return lastPrediction;
}



void tage_train(uint32_t pc, uint8_t outcome) {

    int need_allocate = ((lastPrediction != outcome) & (primaryBank > 0));

    if (primaryBank < NUM_BANKS) {
        BankEntry entry = tageBank[primaryBank].entry[bankGlobalIndex[primaryBank]];
        int isNewAllocated =
                (entry.saturateCounter == -1 || entry.saturateCounter == 0)
                && (entry.usefulness == 0);

        // is entry "pseudo-new allocated"
        if (isNewAllocated) {
            if (primaryPrediction == outcome)
                need_allocate = 0;

            // if the provider component  was delivering the correct prediction; no need to allocate a new entry
            //even if the overall prediction was false
            //see section 3.2.4
            if (primaryPrediction != alternatePrediction) {
                updateSaturate(&useAlternate, alternatePrediction == outcome, 4);
                if (alternatePrediction == outcome) {
                    if (useAlternate < 7)
                        useAlternate++;
                } else if (useAlternate > -8){
                    useAlternate--;
                }
            }
        }
    }

    // try to allocate a  new entries only if prediction was wrong
    if (need_allocate) {
        // is there some "unuseful" entry to allocate
        int8_t min = 127;
        for (int i = 0; i < primaryBank; i++) {
            if (tageBank[i].entry[bankGlobalIndex[i]].usefulness < min){
                min = tageBank[i].entry[bankGlobalIndex[i]].usefulness;
            }
        }

        if (min > 0) {
            //NO UNUSEFUL ENTRY TO ALLOCATE: age all possible targets, but do not allocate
            for (int i = primaryBank - 1; i >= 0; i--) {
                tageBank[i].entry[bankGlobalIndex[i]].usefulness--;
            }
        } else {
            //YES: allocate one entry, but apply some randomness
            // primaryBank I is twice more probable than primaryBank I-1
            int Y = rand() & ((1 << (primaryBank - 1)) - 1);
            int X = primaryBank - 1;
            while ((Y & 1) != 0) {
                X--;
                Y >>= 1;
            }


            for (int i = X; i >= 0; i--) {
                if (tageBank[i].entry[bankGlobalIndex[i]].usefulness == min) {
                    tageBank[i].entry[bankGlobalIndex[i]].tag = generateGlobalEntryTag(pc, i);
                    tageBank[i].entry[bankGlobalIndex[i]].saturateCounter = (outcome == TAKEN) ? 0 : -1;
                    tageBank[i].entry[bankGlobalIndex[i]].usefulness = 0;
                    break;
                }
            }

        }
    }
    //periodic reset of ubit: reset is not complete but bit by bit
    instrIndex++;
    if ((instrIndex & ((1 << 18) - 1)) == 0) {
        int X = (instrIndex >> 18) & 1;
        if ((X & 1) == 0)
            X = 2;
        for (int i = 0; i < NUM_BANKS; i++)
            for (int j = 0; j < (1 << LEN_GLOBAL); j++)
                tageBank[i].entry[j].usefulness &= X;
    }

    // update the counter that provided the prediction, and only this counter
    if (primaryBank < NUM_BANKS) {
        updateSaturate(&(tageBank[primaryBank].entry[bankGlobalIndex[primaryBank]].saturateCounter), outcome, LEN_COUNTS);
    } else {
        updateSaturateMinMax(&(t_bimodalPredictor[BIMODAL_INDEX(pc)]), outcome, 0, 3);

    }
    // update the ubit counter
    if ((lastPrediction != alternatePrediction)) {
        updateSaturateMinMax(&(tageBank[primaryBank].entry[bankGlobalIndex[primaryBank]].usefulness),
                             (lastPrediction == outcome),
                             0, 3);
    }

    // update global history and cyclic shift registers
    //use also history on unconditional branches as for OGEHL predictors.

    for(int i = MAX_HISTORY_LEN - 1 ; i > 0 ; i--){
        t_globalHistory[i] = t_globalHistory[ i - 1];
    }
    t_globalHistory[0] = outcome ? TAKEN : NOTTAKEN;

    t_pathHistory = (t_pathHistory << 1) + (pc & 1);
    t_pathHistory = (t_pathHistory & ((1 << 16) - 1));
    for (int i = 0; i < NUM_BANKS; i++) {
        t_updateCompressed(&(tageBank[i].indexCompressed), t_globalHistory);
        t_updateCompressed(&(tageBank[i].tagCompressed[0]), t_globalHistory);
        t_updateCompressed(&(tageBank[i].tagCompressed[1]), t_globalHistory);
    }

}
#endif //TAGE_MASTER_TAGE_H
