#include "solver.h"

bool CSat::test_jheap_score() {
  return true;


    for(int i=1; i<=num_vars; i++) {
        Variable &v = VAR(i);
        if(v.is_delete) continue;
        if(is_jreason(i) && is_jnode(i)) {
          double mx_act = -1;
          int max_fanin_lit;
          for(int fanin_lit : v.fanin_lits) {
            if(value[fanin_lit] == 0) {
              if(var_activity[fanin_lit / 2] > mx_act) {
                mx_act = var_activity[fanin_lit / 2];
                max_fanin_lit = fanin_lit;
              }
            }
          }
          if(mx_act > gate_activity[i] + 0.0000001) { 
              printf("gate: %d(level:%d) max_act: %.6lf gate_acitivty: %.6lf fanin_var: %d(level:%d)\n", i, assigned[i].level, mx_act, gate_activity[i], max_fanin_lit / 2, assigned[max_fanin_lit / 2].level);
              return false;
          }
        }
    }
    return true;
}

bool CSat::test_in_jheap() {
  return true;

  for(int i=1; i<=num_vars; i++) {
    if(VAR(i).is_delete) continue;
    if(is_jreason(i) && is_jnode(i)) {
      if(!injheap[i]) {
        return false;
      }
    }
  }

  return true;
}