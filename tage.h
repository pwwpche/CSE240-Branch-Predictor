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

/*
 * Implementation of TAGE Predictor based on:
 *  A case for (partially)-tagged geometric history length predictors
 *  Andre Seznec, IRISA
 *  http://www.irisa.fr/caps/people/seznec/JILP-COTTAGE.pdf
 *  http://www.irisa.fr/caps/
 *  with slight modifications.
 *
 *  Configuration:
 *
 *  1. 1x basic bimodal predictor, There are BIMODAL_SIZE = 4099 entries in it.
 *     Each entry is a 2-bit saturate counter.
 *     #define BIMODAL_SIZE 4099
 *     #define LEN_BIMODAL 2
 *     int8_t t_bimodalPredictor[BIMODAL_SIZE];    // Bimodal Predictor table
 *     Size for bimodal predictor: 2 * (4099) = 8198 bits.
 *
 *  2. 7x TAGE tables
 *     #define NUM_BANKS 7                      // 7x TAGE tables
 *     #define LEN_TAG 10                       // 10 bit tag length
 *     #define LEN_GLOBAL 9                     // index into global table in each bank is 9 bit, thus there are 2^9 = 512 entries in each table.
 *     #define LEN_COUNTS 3                     // Each entry has a 3 bit saturate counter, 2 bit usefulness counter, 10 bit tag.
 *     typedef struct CompressedStruct{
            int8_t geometryLength;
            int8_t targetLength;
            uint32_t compressed;
        } CompressedHistory ;                   // 8 + 8 + 32 = 48 bits.

        typedef struct BankEntryStruct{
            int8_t saturateCounter;             // 3 bit saturate counter
            uint16_t tag;                       // 10 bit tag
            int8_t usefulness;                  // 2 bit usefulness counter
        } BankEntry ;                           // 15 bits in total.

       typedef struct BankStruct{
            int geometry;                       //This is predefined by GEOMETRICS and never changes, so does not count into total number of bits used.
            BankEntry entry[1 << LEN_GLOBAL];   // (1 << LEN_GLOBAL) = (1 << 9) = 512 items for each table
            CompressedHistory indexCompressed;  // 48 bits.
            CompressedHistory tagCompressed[2]; // 48 * 2 bits.
        } Bank;


 *     Total space for entries: 512 * 15 = 7680 bits.
 *     Each table has 3 CompressedHistory, (8 + 8 + 32) = 48 bit.
 *     Total size: 7 * (48 + 15 + 7680) = 54201 bits
 *
 *
 *  3. 1x Global History Table
 *     #define MAX_HISTORY_LEN 131
 *     uint8_t t_globalHistory[MAX_HISTORY_LEN];
 *
 *     MAX_HISTORY_LEN = 131 entries, 1 bit per entry
 *     Total space: 131 bits.
 *
 *  4. BankGlobalIndex:
 *     uint32_t bankGlobalIndex[NUM_BANKS];
 *     Store the entry index to each bank.
 *     Each entry consumes LEN_GLOBAL = 9 bit.
 *     Total space: 9 * 7 = 63 bits.
 *
 *  5. tagResult:
 *     int tagResult[NUM_BANKS];
 *     Store the 10-bit tag to each bank in last computation.
 *     Total space: 10 * 7 = 70 bits.
 *
 *  Total size:
 *     8198 + 54201 + 131 + 63 + 70 = 62663 bits.
 *
 */


// 7 Banks plus a basic bimodal predictor
#define NUM_BANKS 7

#define BIMODAL_SIZE 4099
#define LEN_BIMODAL 2
#define BIMODAL_INDEX(pc) (pc % BIMODAL_SIZE)

#define LEN_GLOBAL 9
#define LEN_TAG 10
#define LEN_COUNTS 3
#define MIN_HISTORY_LEN 5
#define MAX_HISTORY_LEN 131

// i = 0 ... NUM_BANKS - 1
// geo_i = (int) (MIN_LEN * ((MAX_LEN / MIN_LEN) ^ (i / (NUM_BANKS - 1))) + 0.5)
const uint8_t GEOMETRICS[NUM_BANKS] = {130, 76, 44, 26, 15, 9, 5};


// SaturateCounter is LEN_COUNTS=3 bits
// Tag is LEN_TAG = 10 bits
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
    int geometry;       //This is predefined by GEOMETRICS and never changes, so does not count into total number of bits used.
    BankEntry entry[1 << LEN_GLOBAL];
    CompressedHistory indexCompressed;
    CompressedHistory tagCompressed[2];
} Bank;

int8_t t_bimodalPredictor[BIMODAL_SIZE];    // Bimodal Predictor table
uint8_t t_globalHistory[MAX_HISTORY_LEN];             // Global History Register
uint32_t t_pathHistory;                               // Path History Register

Bank tageBank[NUM_BANKS];
uint8_t primaryBank = NUM_BANKS;
uint8_t alternateBank = NUM_BANKS;
uint8_t primaryPrediction = NOTTAKEN;
uint8_t alternatePrediction = NOTTAKEN;
uint8_t lastPrediction = NOTTAKEN;

uint32_t bankGlobalIndex[NUM_BANKS];
int tagResult[NUM_BANKS];

int8_t useAlternate = 8;


// Bimodal prediction. If tag miss in every table, use bimodal predictor result.
uint8_t t_getBimodalPrediction(uint32_t pc){
    return (uint8_t) ((t_bimodalPredictor[BIMODAL_INDEX(pc)] >= (1 << (LEN_BIMODAL / 2))) ? TAKEN : NOTTAKEN);
}

// Update the compressed history of each TAGE bank.
void t_updateCompressed(CompressedHistory* history, uint8_t* global){
    uint32_t newCompressed = (history->compressed << 1) + global[0];
    newCompressed ^=  global[history->geometryLength] << (history->geometryLength % history->targetLength);
    newCompressed ^= (newCompressed >> history->targetLength);
    newCompressed &= (1 << history->targetLength) - 1;
    history->compressed = newCompressed;

}

// Tag in global TAGE table entry computation
uint16_t generateGlobalEntryTag(uint32_t pc, int bankIndex) {
    int tag = pc ^(tageBank[bankIndex].tagCompressed[0].compressed) ^((tageBank[bankIndex].tagCompressed[1].compressed) << 1);
    return (uint16_t) (tag & ((1 << (LEN_TAG - ((bankIndex + (NUM_BANKS & 1)) / 2))) - 1));
}

// Update saturate counter.
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

// Update saturate counter using min-max constraints.
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

// Function for compuation of entry index in each bank.
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
    memset(t_bimodalPredictor, (1 << (LEN_BIMODAL / 2)) - 1, sizeof(uint8_t) * BIMODAL_SIZE);
    for(uint32_t i = 0 ; i < NUM_BANKS ; i++){
        tageBank[i].geometry = GEOMETRICS[i];

        // Initialize PPM-based compressed global history
        tageBank[i].indexCompressed.compressed = 0;
        tageBank[i].indexCompressed.geometryLength = GEOMETRICS[i];
        tageBank[i].indexCompressed.targetLength = LEN_GLOBAL;

        tageBank[i].tagCompressed[0].compressed = 0;
        tageBank[i].tagCompressed[0].geometryLength = GEOMETRICS[i];
        tageBank[i].tagCompressed[0].targetLength = (int8_t) (LEN_TAG - ((i + (NUM_BANKS & 1)) / 2));
        tageBank[i].tagCompressed[1].compressed = 0;
        tageBank[i].tagCompressed[1].geometryLength = GEOMETRICS[i];
        tageBank[i].tagCompressed[1].targetLength = (int8_t) (LEN_TAG - ((i + (NUM_BANKS & 1)) / 2) - 1);

        // Initialize entries in TAGE banks
        for(uint32_t j = 0 ; j < (1 << LEN_GLOBAL) ; j++){
            tageBank[i].entry[j].saturateCounter = 0;
            tageBank[i].entry[j].tag = 0;
            tageBank[i].entry[j].usefulness = 0;
        }

    }


    memset(bankGlobalIndex, 0, sizeof(uint32_t) * NUM_BANKS);
    memset(t_globalHistory, 0, sizeof(uint8_t) * MAX_HISTORY_LEN);
    t_pathHistory = 0;
    srand((unsigned int) time(NULL));
}



uint8_t tage_predict(uint32_t pc){



    // Get tag in each bank and indices for entry in each bank
    for(uint32_t i = 0 ; i < NUM_BANKS ; i++){
        tagResult[i] = generateGlobalEntryTag(pc, i);
        bankGlobalIndex[i] = getGlobalIndex(pc, i);
    }

    primaryPrediction = NOTTAKEN;
    alternatePrediction = NOTTAKEN;
    primaryBank = NUM_BANKS;
    alternateBank = NUM_BANKS;

    // Look for primary bank and alternate bank.
    // Primary bank gives the final result. If primary bank is not found, use alternate bank.
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

        // Detecting newly allocated entries.
        // They are not quite useful in some circumenstances.
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

    // Update the model under false prediction in TAGE banks.
    int need_allocate = ((lastPrediction != outcome) & (primaryBank > 0));

    if (primaryBank < NUM_BANKS) {
        BankEntry entry = tageBank[primaryBank].entry[bankGlobalIndex[primaryBank]];

        // Find if this entry is newly allocated.
        int isNewAllocated =
                (entry.saturateCounter == -1 || entry.saturateCounter == 0)
                && (entry.usefulness == 0);

        if (isNewAllocated) {
            if (primaryPrediction == outcome)
                need_allocate = 0;

            // If the provider component  was delivering the correct prediction; no need to allocate a new entry
            // even if the overall prediction was false
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

    // Allocate new entry under false prediction.
    if (need_allocate) {

        // Find not-useful entries.
        int8_t min = 127;
        for (int i = 0; i < primaryBank; i++) {
            if (tageBank[i].entry[bankGlobalIndex[i]].usefulness < min){
                min = tageBank[i].entry[bankGlobalIndex[i]].usefulness;
            }
        }

        if (min > 0) {
            // If there are no useful entry, make all corresponding entries a little bit not useful.
            for (int i = primaryBank - 1; i >= 0; i--) {
                tageBank[i].entry[bankGlobalIndex[i]].usefulness--;
            }
        } else {
            // Randomly allocate the entry to banks.
            // Previous ones (banks associated with longer history) have higher priority.
            int Y = rand() & ((1 << (primaryBank - 1)) - 1);
            int X = primaryBank - 1;
            while ((Y & 1) != 0) {
                X--;
                Y >>= 1;
            }

            // Find the banks we are re-allocating.
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

    // Update the counter that gives the prediction.
    if (primaryBank < NUM_BANKS) {
        updateSaturate(&(tageBank[primaryBank].entry[bankGlobalIndex[primaryBank]].saturateCounter), outcome, LEN_COUNTS);
    } else {
        updateSaturateMinMax(&(t_bimodalPredictor[BIMODAL_INDEX(pc)]), outcome, 0, (1 << LEN_BIMODAL) - 1);

    }

    if ((lastPrediction != alternatePrediction)) {
        updateSaturateMinMax(&(tageBank[primaryBank].entry[bankGlobalIndex[primaryBank]].usefulness),
                             (lastPrediction == outcome),
                             0, 3);
    }

    // Update global history
    for(int i = MAX_HISTORY_LEN - 1 ; i > 0 ; i--){
        t_globalHistory[i] = t_globalHistory[ i - 1];
    }
    t_globalHistory[0] = outcome ? TAKEN : NOTTAKEN;

    // Update compressed history
    t_pathHistory = (t_pathHistory << 1) + (pc & 1);
    t_pathHistory = (t_pathHistory & ((1 << 16) - 1));
    for (int i = 0; i < NUM_BANKS; i++) {
        t_updateCompressed(&(tageBank[i].indexCompressed), t_globalHistory);
        t_updateCompressed(&(tageBank[i].tagCompressed[0]), t_globalHistory);
        t_updateCompressed(&(tageBank[i].tagCompressed[1]), t_globalHistory);
    }

}
#endif //TAGE_MASTER_TAGE_H
