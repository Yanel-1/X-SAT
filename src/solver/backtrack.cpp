#include "solver.h"

void CSat::backtrack(int backtrack_level) {

    // printf(">> back to %d\n", backtrack_level);

    if (pos_in_trail.size() <= backtrack_level) return;

    for (int i = trail.size() - 1; i >= pos_in_trail[backtrack_level]; i--) {

        value[trail[i]] = value[notlit(trail[i])] = 0;
        int g = lit2pvar(trail[i]);
        if(branchMode == BranchMode::JFRONTIER) {
          for(int var : VAR(g).fanout_vars) {
              Variable &v = VAR(var);
              if(var_activity[g] > gate_activity[var]) {
                gate_activity[var] = var_activity[g];
                if(injheap[var]) heap.update(var);
              }
          }
        } else if(branchMode == BranchMode::VMTF) {
          if(raq.stamp_map[g] > raq.start_stamp) {
            raq.start_stamp = raq.stamp_map[g];
            raq.start_it = raq.index[g];
          }
        } else {
          assert(branchMode == BranchMode::VSIDS);
          if (!heap.inHeap(g)) heap.insert(g);
        }
        // phase saving
        saved[g] = (trail[i] & 1) ? -1 : 1;
    }


    if(branchMode == BranchMode::JFRONTIER) {
      for(int level = pos_in_trail.size(); level > backtrack_level; level--) {
        for(int i=0; i<assgined_vars_by_level[level].size(); i++) {
          int var = assgined_vars_by_level[level][i];
          // Variable &v = VAR(var);
          // double mx_act = 0;
          // for(int fanin_lit : v.fanin_lits) {
          //   if(value[fanin_lit] == 0) {
          //     mx_act = std::max(mx_act, var_activity[fanin_lit / 2]);
          //   }
          // }
          // gate_activity[var] = mx_act;
          if(assigned[var].level <= level) {
            if(!injheap[var]) { heap_insert(var); }
          }
        }
        assgined_vars_by_level[level].clear();
      }
    }

    propagated = pos_in_trail[backtrack_level];
    trail.setsize(propagated);
    pos_in_trail.setsize(backtrack_level);
}