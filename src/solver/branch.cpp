#include "solver.h"

#include <algorithm>
#include <vector>

void CSat::initVsidsMode() {
    branchMode = BranchMode::VSIDS;
    std::vector<int> variables;

    for (int i = 1; i <= num_vars; i++) {
        if (VAR(i).is_delete) continue;
        variables.push_back(i);
        injheap[i] = 0;
        var_activity[i] = gate_activity[i] = 0;
    }

    	int _xors = 0;
      int _all = 0;

      for(int i=num_inputs+1; i<=num_vars; i++) {
        Variable &var = VAR(i);
        if(var.is_delete) continue;

        int is_xor = 0;

        if(var.type == XOR) is_xor = 1;
        // if(var.fanout_vars.size() == 0) continue;
        for(int fanin_lit : var.fanin_lits) {
          Variable &fanin_var = VAR(lit2pvar(fanin_lit));
          if(fanin_var.type == XOR) {
            var.neighbors.push(lit2pvar(fanin_lit));
            is_xor++;
          }
        }
        for(VRef fanout_vref : var.fanout_vars) {
          Variable &fanout_var = VAR(fanout_vref);
          if(fanout_var.type == XOR) {
            var.neighbors.push(fanout_vref);
            is_xor++;
          }
        }

        var.xor_edges = is_xor;

        _all++;
        if(is_xor) _xors++;
      }

      printf("c xor edge vars: %.2f ( %d / %d )\n", double(_xors) / _all, _xors, _all);
    for (int var : variables) {
        heap.insert(var);
    }
}


void CSat::initJfontierMode() {
  branchMode = BranchMode::JFRONTIER;
}

void CSat::switchVsidsMode() {
  branchMode = BranchMode::VSIDS;
  while(!heap.empty()) heap.pop();

  for (int i = 1; i <= num_vars; i++) {
		if(VAR(i).is_delete) continue;
		heap.insert(i);
	}

}

void CSat::switchJfontierMode() {
  branchMode = BranchMode::JFRONTIER;
  for (int i = 1; i <= num_vars; i++) {
		if(VAR(i).is_delete) continue;
		injheap[i] = 0;
    Variable &v = VAR(i);
    double mx_act = 0;
    for(int fanin_lit : v.fanin_lits) {
      if(value[fanin_lit] == 0) {
        mx_act = std::max(mx_act, var_activity[fanin_lit / 2]);
      }
    }
    gate_activity[i] = mx_act;
	}
  while(!jheap.empty()) jheap.pop();

  for(int i=0; i<trail.size(); i++) {
    VRef vref = trail[i] / 2;
    if(is_jnode(vref)) heap_insert(vref);
  }
}

int CSat::branchJfontierMode() {
  assert(test_in_jheap());

    int g = VAR(lit2pvar(trail.back())).decide_by;
    int decide_vref = -1;

    if(!(is_jreason(g) && is_jnode(g))) {

      // printf("new\n");

    while(true) {
      if (jheap.size() == 0) return -1;
      g = jheap.top();
      jheap.pop(); injheap[g] = 0;
      if(!is_jreason(g)) { continue; }
      if(is_jnode(g)) {

        // check gate is right
        decide_vref = -1;
        Variable &var = VAR(g);
        for(int fanin_lit : var.fanin_lits) {
          VRef fanin_vref = lit2pvar(fanin_lit);
          if(value[fanin_lit] != 0) continue;
          if(decide_vref == -1 || var_activity[fanin_vref] > var_activity[decide_vref])
            decide_vref = fanin_vref;
        }
        if(decide_vref != -1 && var_activity[decide_vref] != gate_activity[g]) {
          assert(var_activity[decide_vref] <= gate_activity[g]);
          gate_activity[g] = var_activity[decide_vref];
          heap_insert(g);
          continue;
        }

        // finded!
        assgined_vars_by_level[pos_in_trail.size() + 1].push(g);
        break;
      }

      Variable &var = VAR(g);

      int max_fanin_level = -1;

      for(int fanin_lit : var.fanin_lits) {
        if(value[fanin_lit] == 0) continue;
        max_fanin_level = std::max(max_fanin_level, assigned[lit2pvar(fanin_lit)].level);
      }


      if(max_fanin_level != -1) {
        assgined_vars_by_level[max_fanin_level].push(g);
      }
    }

    } else {
      Variable &var = VAR(g);
      decide_vref = -1;
      for(int fanin_lit : var.fanin_lits) {
        VRef fanin_vref = lit2pvar(fanin_lit);
        if(value[fanin_lit] != 0) continue;
          if(decide_vref == -1 || var_activity[fanin_vref] > var_activity[decide_vref])
            decide_vref = fanin_vref;
      }
    }

    Variable &var = VAR(g);

    assert(decide_vref != -1 && value[decide_vref * 2] == 0);

    VAR(decide_vref).decide_by = g;

    // assert(var_activity[decide_vref] == gate_activity[g]);

    pos_in_trail.push(trail.size());

    // printf("decide-var: %d(%d)\n", decide_vref, pos_in_trail.size());

    int phase = 1; // can improved
    if(saved[decide_vref]) phase = (saved[decide_vref] > 0);



    int lit = makelit(decide_vref, phase);

    assign(lit, pos_in_trail.size(), std::make_pair(0, -2));

    if(is_jreason(g) && is_jnode(g)) {
      if(!injheap[g]) heap_insert(g);
    }

    return 0;
}

int CSat::branchVsidsMode() {

    int next = -1;

      while (next == -1 || value[var2lit(next)] != 0) {    // Picking a variable according to VSIDS
        if (heap.empty()) return -1;
        next = heap.pop();
      }

      for(int fanin_lit : VAR(next).fanin_lits) {
        int neibor = lit2pvar(fanin_lit);
        if(!VAR(neibor).is_input && heap.inHeap(neibor)) {
          // printf("FUCK!!\n");
          heap.erase(neibor);
          tabu_stamp_arr[neibor] = tabu_stamp;
          tabu_heap.insert(neibor);
        }
      }

    pos_in_trail.push(trail.size());
    
    int phase = 1; // need updated
    if(saved[next]) phase = (saved[next] > 0);
    // if(simu_prop[next] > 0.5) phase = 0;
    // else phase = 1;

    int lit = makelit(next, phase);
    assign(lit, pos_in_trail.size(), std::make_pair(0, -2));
    return 0;
}

void CSat::initVmtfMode() {
  for(int i=1; i<=num_vars; i++) {
    if(VAR(i).is_delete) continue;
    raq.insert(i);
  }
}

void CSat::swtichVmtfMode() {
  branchMode = BranchMode::VMTF;
  raq.start_it = raq.elements.begin();
}

int CSat::branchVmtfMode() {
  VRef vref = -1;


  // int cnt = 0;
  for(; raq.start_it != raq.elements.end(); raq.start_it++) {
    // cnt++;
    if(value[(*raq.start_it)*2] == 0) { vref = *raq.start_it; break;}
  }
  // printf("%d ", cnt);

  // sat
  if(vref == -1) {
    // printf("FUCK!\n");
    return -1;
  }

  pos_in_trail.push(trail.size());

  int phase = 1; // need updated
  if(saved[vref]) phase = (saved[vref] > 0);

  int lit = makelit(vref, phase);
  assign(lit, pos_in_trail.size(), std::make_pair(0, -2));

  // printf("decide: %d\n", lit);

  return 0;
}

int CSat::decide() { // use J-frontier
  if(branchMode == BranchMode::VSIDS) return branchVsidsMode();
  else if(branchMode == BranchMode::VMTF) {
    return branchVmtfMode();
  } else {
    assert(branchMode == BranchMode::JFRONTIER);
    return branchJfontierMode();
  }
}
