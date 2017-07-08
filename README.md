# CSE240-Branch-Predictor
Course project of UCSD CSE-240A Computer Architecture


### Implementation of TAGE Predictor based on:
[A case for (partially)-tagged geometric history length predictors](http://www.irisa.fr/caps/people/seznec/JILP-COTTAGE.pdf), Andre Seznec, [IRISA](http://www.irisa.fr/caps/)


with slight modifications.

### Configuration:

#### 1. 1x basic bimodal predictor, There are BIMODAL_SIZE = 4099 entries in it.
```
#define BIMODAL_SIZE 4099
#define LEN_BIMODAL 2
int8_t t_bimodalPredictor[BIMODAL_SIZE];    
```
Each entry is a 2-bit saturate counter.

Size for bimodal predictor: 2 * (4099) = 8198 bits.

#### 2. 7x TAGE tables
```
   #define NUM_BANKS 7                      // 7x TAGE tables
   #define LEN_TAG 10                       // 10 bit tag length
   #define LEN_GLOBAL 9                     // index into global table in each bank is 9 bit, thus there are 2^9 = 512 entries in each table.
   #define LEN_COUNTS 3                     // Each entry has a 3 bit saturate counter, 2 bit usefulness counter, 10 bit tag.
   typedef struct CompressedStruct{
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
            CompressedHistory tagCompressed[2]; // 48 2 bits.
        } Bank;
 ```
   Total space for entries: 512 * 15 = 7680 bits
   
   Each table has 3 CompressedHistory, (8 + 8 + 32) = 48 bit.
   
   Total size: 7 * (48 + 15 + 7680) = 54201 bits


#### 3. 1x Global History Table
```
   #define MAX_HISTORY_LEN 131
   uint8_t t_globalHistory[MAX_HISTORY_LEN]; 
```   
   MAX_HISTORY_LEN = 131 entries, 1 bit per entry
   
   Total space: 131 bits.

#### 4. BankGlobalIndex:
```
   uint32_t bankGlobalIndex[NUM_BANKS];
```
   Store the entry index to each bank.
   
   Each entry consumes LEN_GLOBAL = 9 bit.
   
   Total space: 9 * 7 = 63 bits.

#### 5. tagResult:
```
   int tagResult[NUM_BANKS];
```
   Store the 10-bit tag to each bank in last computation.
   
   Total space: 10 * 7 = 70 bits.

#### Total size:

   8198 + 54201 + 131 + 63 + 70 = 62663 bits.


### Performance
```
CUSTOM predictor's missprediction rate on benchmark: fp_1 (0.818/100.0)
CUSTOM predictor's missprediction rate on benchmark: fp_2 (0.294/100.0)
CUSTOM predictor's missprediction rate on benchmark: int_1 (6.898/100.0)
CUSTOM predictor's missprediction rate on benchmark: int_2 (0.274/100.0)
CUSTOM predictor's missprediction rate on benchmark: mm_1 (0.415/100.0)
CUSTOM predictor's missprediction rate on benchmark: mm_2 (5.576/100.0)
CUSTOM predictor's missprediction rate on benchmark: fp_3 (2.177/100.0)
CUSTOM predictor's missprediction rate on benchmark: int_3 (1.119/100.0)
CUSTOM predictor's missprediction rate on benchmark: mm_3 (10.861/100.0)
CUSTOM predictor's missprediction rate on benchmark: serv_1 (3.875/100.0)
CUSTOM predictor's missprediction rate on benchmark: serv_2 (1.819/100.0)
CUSTOM predictor's missprediction rate on benchmark: serv_3 (1.605/100.0)
```
