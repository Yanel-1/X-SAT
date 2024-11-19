#include "solver.h"

void CSat::bump_vmtf() {
    std::vector<int> bumped;
    for(int i=0; i<bump.size(); i++) bumped.push_back(bump[i]);

    std::sort(bumped.begin(), bumped.end(), [this](const int lhs, const int rhs){
        return raq.stamp_map[lhs] < raq.stamp_map[rhs];
    });

    // for(int vref : bumped) {
    //     printf("%d ", raq.stamp_map[vref]);
    // }
    // printf("\n");

    for(int vref : bumped) {
        
        raq.remove(vref);
        raq.insert(vref);
    }
    
}