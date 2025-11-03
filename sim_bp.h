#ifndef SIM_BP_H
#define SIM_BP_H
#include <cstdint>
#include <vector>

typedef struct bp_params{
    unsigned long int K;
    unsigned long int M1;
    unsigned long int M2;
    unsigned long int N;
    char*             bp_name;
}bp_params;

//m2 used for bimodal
class Bimodal {
    public:
    bp_params params;
    int num_predictions;
    int num_miss;
    int miss_rate;
    std::vector<uint8_t> ptable;
    bool current_prediction;

    Bimodal() {
        params.K = 0;
        params.M1 = 0;
        params.M2 = 0;
        params.N = 0;
        params.bp_name = nullptr;
        num_predictions = 0;
        num_miss = 0;
        miss_rate = 0;
    }

    Bimodal(bp_params input) {
        params.K = input.K;
        params.M1 = input.M1;
        params.M2 = input.M2;
        params.N = input.N;
        params.bp_name = input.bp_name;
        num_predictions = 0;
        num_miss = 0;
        miss_rate = 0;
        ptable.resize(1 << input.M2, 2);
    }

    bool prediction(uint64_t addr);
    void update_table(uint64_t addr, char result);
    //bool prediction_result()
};
class gshare {

};

// Put additional data structures here as per your requirement

#endif
