#include <stdio.h>
#include <stdlib.h>

#define WT_SIZE1 171
#define WT_SIZE2 171
#define WT_SIZE3 173
#define GHL 60	// length of global history for SGHR, GHR, HA and HTrain
#define WL_1 20 // history length for the separate 1024-entry tables
#define WL_2 20 // history length for the single 1024-entry table
#define WL_3 20 // history length for the single 512-entry table
#define THH 50 // Initial threshold of trainning

/* --- weight tables --- */
int8_t     weightT[WT_SIZE1][WL_1]; // Taken weight table for the most recent 20 branches. 7 * 1024 * 20 = 143,360 bits
int8_t	   weightNT[WT_SIZE1][WL_1]; // Not-Taken weight table for the most recent 20 branches. 7 * 1024 * 20 = 143,360 bits
int8_t	   weight1[WT_SIZE2][WL_2]; // Single weight table for the next 16 branches. 7 * 1024 * 16 = 114,688 bits
int8_t	   weight2[WT_SIZE3][WL_3]; // Single weight table for the last 29 branches. 7 * 512 * 29 = 103,936 bits


/* --- Shift registers --- */
uint8_t HTrain; // A register indicate if training is needed
uint8_t *GHR;	//perfect history used for prediction and updates
uint32_t *HA;	// Path address register

int32_t threshold; // dynamic threshold value as in O-GEHL
int8_t TC; //threshold counter as in O-GEHL
uint8_t wp_recent_prediction = 0;


/*----- Hash function that calculating index of weight tables -----*/
uint32_t gen_widx(uint32_t cur_pc, uint32_t path_pc, uint32_t wt_size)
{
    cur_pc = (cur_pc ) ^ (cur_pc / wt_size);
    path_pc = (path_pc) ^ (path_pc / wt_size);
    uint32_t widx = cur_pc ^ (path_pc);
    widx = widx % wt_size;
    return widx;
    //return (path_pc * 17) % (1 << wt_size);
}



void wp_init() {
  printf("init\n");
    HTrain = 0;
    GHR = malloc(GHL * sizeof(uint8_t));
    HA = malloc(GHL * sizeof(uint32_t));
    memset(GHR, 0, GHL * sizeof(uint8_t));
    memset(HA, 0, GHL * sizeof(uint32_t));

    for (int i = 0; i < WT_SIZE1; i++) {
        for(int j = 0; j < WL_1; j++){
            weightT[i][j] = 0;
            weightNT[i][j] = 0;
        }
    }
    for (int i = 0; i < (WT_SIZE2); i++) {
        for(int j = 0; j < WL_3; j++)
        {
            weight1[i][j] = 0;
        }
    }

    for (int i = 0; i < (WT_SIZE3); i++) {
        for(int j = 0; j < WL_3; j++)
        {
            weight2[i][j] = 0;
        }
    }
    threshold = THH;
    TC = 0;
}


uint8_t wp_perdict(uint32_t pc) {
    int32_t accum = 0;  // This should not be uint32_t!!!
    /*----- First WL_1 branches: use 1024-entry separate T/NT weight tables -----*/
    for (int j = 0; j < WL_1; j++) {
        uint32_t widx = gen_widx(pc, HA[j], WT_SIZE1); // compute index to access either weight table

        if (GHR[j] == 1)        // If history is Taken
            accum += weightT[widx][j];  // Then add Taken weight
        else                // If history is Not-Taken
            accum += weightNT[widx][j]; // Then add Not-Taken weight
    }
    /*----- Next WL_2 branches: use 1024-enrty single weight tables -----*/
    for (int j = 0; j < WL_2; j++) {
        uint32_t widx = gen_widx(pc, HA[WL_1 + j], WT_SIZE2); // compute index to access either weight table
        if (GHR[WL_1 + j] == 1)
            accum += weight1[widx][j];
        else
            accum -= weight1[widx][j];
    }
    /*----- Last WL_3 branches: use 512-entry single weight tables -----*/
    for (int j = 0; j < WL_3; j++) {
        uint32_t widx = gen_widx(pc, HA[WL_1 + WL_2 + j],
                                 WT_SIZE3); // compute index to access weight table
        if (GHR[WL_1 + WL_2 + j] == 1)
            accum += weight2[widx][j];
        else
            accum -= weight2[widx][j];
    }

    uint8_t pred = (accum >= 0); // Predict Taken if sum >= 0; Predict Not-Taken if sum <= 0;
    wp_recent_prediction = pred;

    if ((accum > -threshold) && (accum < threshold)){
      HTrain = 1; // 1 means trainning needed
    }else{
      HTrain = 0;
    }

    return pred;
}




void wp_train(uint32_t pc, uint8_t outcome){


    uint8_t t = outcome;
    if( (t != wp_recent_prediction) || (HTrain == 1) ) 	//Training needed if threshold not exceeded or predict wrong
    {
        /*----- Algorithm for Update -----*/

        /*----- First WL_1 branches: update T/NT weight tables separately -----*/
        for(int j = 0; j < WL_1; j++)	{
            uint32_t widx = gen_widx(pc, HA[j], WT_SIZE1); // compute the index to access either weight table;
            if(t==1 && GHR[j]==1)
            { if(weightT[widx][j]<63) weightT[widx][j]++;}
            else if(t==0 && GHR[j]==1)
            { if(weightT[widx][j]>-64) weightT[widx][j]--;}
            else if(t==1 && GHR[j]==0)
            { if(weightNT[widx][j]<63) weightNT[widx][j]++;}
            else if(t==0 && GHR[j]==0)
            { if(weightNT[widx][j]>-64) weightNT[widx][j]--;}
        }
        /*----- Next WL_2 branches: update regular weight tables -----*/
        for(int j = 0; j < WL_2; j++)	{
            uint32_t widx = gen_widx(pc, HA[j+WL_1], WT_SIZE2); // compute the index to access either weight table;
            if(t==GHR[j+WL_1])
            { if(weight1[widx][j]<63) weight1[widx][j]++;}
            else
            { if(weight1[widx][j]>-64) weight1[widx][j]--;}
        }
        /*----- Last WL_3 branches: update regular weight tables -----*/
        for(int j = 0; j < WL_3; j++)	{
            uint32_t widx = gen_widx(pc, HA[j+WL_1+WL_2], WT_SIZE3); // compute the index to access either weight table;
            if(t==GHR[j+WL_1+WL_2])
            { if(weight2[widx][j]<63) weight2[widx][j]++;}
            else
            { if(weight2[widx][j]>-64) weight2[widx][j]--;}
        }
    }

    // Update history shift registers (to be used in Update phase)
    for (int j = GHL - 1; j > 0; j--) {

        GHR[j] = GHR[j - 1];
        HA[j] = HA[j - 1];
    }
    GHR[0] = wp_recent_prediction;
    HA[0] = pc; // HA records the path address

    /*------Update the threshold -------*/
    if(t != wp_recent_prediction) {
        TC++;
        if(TC==63) {
            TC = 0;
            threshold++;
        }
    }
    else if(t == wp_recent_prediction && HTrain == 1) {
        TC--;
        if(TC==-63) {
            TC = 0;
            threshold--;
        }
    }

}
