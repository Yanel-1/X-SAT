#include <cassert>

#include "solver.h"

void CSat::bump_var(int var, double coeff) {

    if(branchMode == BranchMode::VMTF) return;

    if ((var_activity[var] += var_inc * coeff) > 1e100) {
        for (int i = 1; i <= num_vars; i++) var_activity[i] *= 1e-100;
        for (int i = 1; i <= num_vars; i++) gate_activity[i] *= 1e-100;
        var_inc *= 1e-100;
    }

    if(branchMode == BranchMode::JFRONTIER) {
    // if(value[var * 2] == 0) {
        Variable &variable = VAR(var);
        for(VRef fanout_vref : variable.fanout_vars) {
            if (gate_activity[fanout_vref] < var_activity[var]) {
                gate_activity[fanout_vref] = var_activity[var];
                if(injheap[fanout_vref]) jheap.update(fanout_vref);
            }
        }
    // }
    } else if(branchMode == BranchMode::VSIDS) {
        if (heap.inHeap(var)) heap.update(var);
    }
}

void CSat::heap_insert(int gate_id) {
    assert(!injheap[gate_id]);
    injheap[gate_id] = 1;
    jheap.insert(gate_id);
    // printf(" insert gate %d (%d) to J-heap %d\n", gate_id, var.out, jheap.pos[gate_id]);
}

void CSat::heap_remove(int gate_id) {
    // printf(" remove %d (%d) from J-heap %d\n", gate_id, var.out, jheap.pos[gate_id]);
    assert(injheap[gate_id]);
    injheap[gate_id] = 0;
    if (jheap.top() == gate_id) {
        jheap.pop();
    }
    else {
        jheap.erase(gate_id);
    }
}

bool CSat::is_jreason(int v) {
    Variable &var = VAR(v);
    assert(!var.is_delete);
    if(value[var.out] == 0) return false;
    if(var.type == INPUT) return false;
    return true;
}

bool CSat::is_jnode(int gate_id) {
    Variable &var = VAR(gate_id);
    assert(!var.is_delete);
    for(CRef ref : var.clauses) {
        Clause &cls = clauseDB[ref];
        assert(cls.size >= 2);
        bool has_pos_lit = false;
        for(int i=0; i<cls.size; i++) {
            if(value[cls[i]] == 1) { has_pos_lit = true; break; }
        }
        if(!has_pos_lit) {
            return true;   
        }
    }
    return false;
}
