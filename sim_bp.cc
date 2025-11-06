//#include <cstdint>
#include <cstdint>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sim_bp.h"

/*  argc holds the number of command line arguments
    argv[] holds the commands themselves

    Example:-
    sim bimodal 6 gcc_trace.txt
    argc = 4
    argv[0] = "sim"
    argv[1] = "bimodal"
    argv[2] = "6"
    ... and so on
*/

const uint8_t STRONG_NOT = 0;
const uint8_t WEAK_NOT = 1;
const uint8_t WEAK_TAKE = 2;
const uint8_t STRONG_TAKE = 3;
const uint8_t LSB_2 = 2;

bool Bimodal::prediction_bimodal(uint64_t addr) {
    //bool taken = 0;
    uint32_t index = (addr >> LSB_2) & ((1 << params.M2) - 1);
    if (ptable.at(index) >= WEAK_TAKE) current_prediction = true;
    else current_prediction = false;
    num_predictions++;
    return current_prediction;
}

bool Gshare::prediction_gshare(uint64_t addr) {
    uint32_t index = (addr >> LSB_2) & ((1 << params.M1) - 1) >> (params.M1 - params.N) ^ ghr;
    if (ptable.at(index) >= WEAK_TAKE) current_prediction = true;
    else current_prediction = false;
    num_predictions++;
    return current_prediction;
}

void Bimodal::update_table_bimodal(uint64_t addr, char result[2]) {
    //bool taken = false;
    uint32_t index = (addr >> LSB_2) & ((1 << params.M2) - 1);
    // update miss count
    if ((result[0] == 't' && !current_prediction) || (result[0] == 'n' && current_prediction)) {
        num_miss++;
    }

    //update table
    switch(ptable[index]) {
        case STRONG_NOT:
        if (result[0] == 't') ptable[index]++;
        break;
        case WEAK_NOT:
        if (result[0] == 't') ptable[index]++;
        else ptable[index]--;
        break;
        case WEAK_TAKE:
        if (result[0] == 't') ptable[index]++;
        else ptable[index]--;
        break;
        case 3:
        if (result[0] == 'n') ptable[index]--;
        break;
    }
}

void Gshare::update_table_gshare(uint64_t addr, char result[2]) {
    int taken = 0;
    //bool taken = false;
    uint32_t index = (addr >> LSB_2) & ((1 << params.M1) - 1) >> (params.M1 - params.N) ^ ghr;
    if (result[0] == 't') taken = 1;
    
    // update miss count
    if ((result[0] == 't' && !current_prediction) || (result[0] == 'n' && current_prediction)) {
        num_miss++;
    }

    //update table
    switch(ptable[index]) {
        case STRONG_NOT:
        if (result[0] == 't') ptable[index]++;
        break;
        case WEAK_NOT:
        if (result[0] == 't') ptable[index]++;
        else ptable[index]--;
        break;
        case WEAK_TAKE:
        if (result[0] == 't') ptable[index]++;
        else ptable[index]--;
        break;
        case 3:
        if (result[0] == 'n') ptable[index]--;
        break;
    }

    ghr = (ghr >> 1) | (taken << (params.N - 1));
    ghr &= (1 << params.N) - 1;
}


int main (int argc, char* argv[])
{
    FILE *FP;               // File handler
    char *trace_file;       // Variable that holds trace file name;
    bp_params params;       // look at sim_bp.h header file for the the definition of struct bp_params
    char outcome;           // Variable holds branch outcome
    unsigned long int addr; // Variable holds the address read from input file
    Bimodal* bimodal_pred = nullptr;
    Gshare* gshare_pred = nullptr;
    if (!(argc == 4 || argc == 5 || argc == 7))
    {
        printf("Error: Wrong number of inputs:%d\n", argc-1);
        exit(EXIT_FAILURE);
    }
    
    params.bp_name  = argv[1];
    
    // strtoul() converts char* to unsigned long. It is included in <stdlib.h>
    if(strcmp(params.bp_name, "bimodal") == 0)              // Bimodal
    {
        if(argc != 4)
        {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc-1);
            exit(EXIT_FAILURE);
        }
        params.M2       = strtoul(argv[2], NULL, 10);
        trace_file      = argv[3];
        printf("COMMAND\n%s %s %lu %s\n", argv[0], params.bp_name, params.M2, trace_file);

        // bimodal object creation
        bimodal_pred = new Bimodal(params);
    }
    else if(strcmp(params.bp_name, "gshare") == 0)          // Gshare
    {
        if(argc != 5)
        {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc-1);
            exit(EXIT_FAILURE);
        }
        params.M1       = strtoul(argv[2], NULL, 10);
        params.N        = strtoul(argv[3], NULL, 10);
        trace_file      = argv[4];
        printf("COMMAND\n%s %s %lu %lu %s\n", argv[0], params.bp_name, params.M1, params.N, trace_file);

        // gshare object
        gshare_pred = new Gshare(params);

    }
    else if(strcmp(params.bp_name, "hybrid") == 0)          // Hybrid
    {
        if(argc != 7)
        {
            printf("Error: %s wrong number of inputs:%d\n", params.bp_name, argc-1);
            exit(EXIT_FAILURE);
        }
        params.K        = strtoul(argv[2], NULL, 10);
        params.M1       = strtoul(argv[3], NULL, 10);
        params.N        = strtoul(argv[4], NULL, 10);
        params.M2       = strtoul(argv[5], NULL, 10);
        trace_file      = argv[6];
        printf("COMMAND\n%s %s %lu %lu %lu %lu %s\n", argv[0], params.bp_name, params.K, params.M1, params.N, params.M2, trace_file);

    }
    else
    {
        printf("Error: Wrong branch predictor name:%s\n", params.bp_name);
        exit(EXIT_FAILURE);
    }
    
    // Open trace_file in read mode
    FP = fopen(trace_file, "r");
    if(FP == NULL)
    {
        // Throw error and exit if fopen() failed
        printf("Error: Unable to open file %s\n", trace_file);
        exit(EXIT_FAILURE);
    }
    
    char str[2];
    while(fscanf(FP, "%lx %s", &addr, str) != EOF)
    {
        
        // outcome = str[0];
        // if (outcome == 't')
        //     printf("%lx %s\n", addr, "t");           // Print and test if file is read correctly
        // else if (outcome == 'n')
        //     printf("%lx %s\n", addr, "n");          // Print and test if file is read correctly
        /*************************************
            Add branch predictor code here
        **************************************/
        if(strcmp(params.bp_name, "bimodal") == 0) {
            bimodal_pred->prediction_bimodal(addr);
            bimodal_pred->update_table_bimodal(addr, str);
        }
        else if (strcmp(params.bp_name, "gshare") == 0) {
            gshare_pred->prediction_gshare(addr);
            gshare_pred->update_table_gshare(addr, str);
        }
    }
    printf("OUTPUT\n");
    if(strcmp(params.bp_name, "bimodal") == 0) {
        printf("number of predictions: %d\n", bimodal_pred->num_predictions);
        printf("number of mispredictions: %d\n", bimodal_pred->num_miss);
        bimodal_pred->miss_rate = ((float)bimodal_pred->num_miss / bimodal_pred->num_predictions) * 100;
        printf("misprediction rate: %.2f%%\n", bimodal_pred->miss_rate);
        printf("FINAL BIMODAL CONTENTS\n");
        for (uint32_t i = 0; i < bimodal_pred->ptable.size(); i++) {
            printf("%d	%d\n", i, bimodal_pred->ptable[i]);
        }
    }
    else {
        printf("number of predictions: %d\n", gshare_pred->num_predictions);
        printf("number of mispredictions: %d\n", gshare_pred->num_miss);
        gshare_pred->miss_rate = ((float)gshare_pred->num_miss / gshare_pred->num_predictions) * 100;
        printf("misprediction rate: %.2f%%\n", gshare_pred->miss_rate);
        printf("FINAL BIMODAL CONTENTS\n");
        for (uint32_t i = 0; i < gshare_pred->ptable.size(); i++) {
            printf("%d	%d\n", i, gshare_pred->ptable[i]);
        }
    }
    
    return 0;
}