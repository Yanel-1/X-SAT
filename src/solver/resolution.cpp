#include <unordered_set>

#include "solver.h"

struct LessActivity {        // A compare function used to sort the activities.
    const int *activity;
    const CSat *csat;     
    bool operator() (int a, int b) const { 
        if(activity[a] != activity[b]) {
            return activity[a] > activity[b];
        }

        auto &var_a = csat->var_mem[a];
        auto &var_b = csat->var_mem[b];

        // int score_a = var_a.clauses.size();
        // for(int fanin_lit : var_a.fanin_lits) {
        //     score_a += csat->var_mem[lit2pvar(fanin_lit)].clauses.size();
        // }
        // for(int fanout_vref : var_a.fanout_vars) {
        //     score_a += csat->var_mem[fanout_vref].clauses.size();
        // }

        // int score_b = var_b.clauses.size();
        // for(int fanin_lit : var_b.fanin_lits) {
        //     score_b += csat->var_mem[lit2pvar(fanin_lit)].clauses.size();
        // }
        // for(int fanout_vref : var_b.fanout_vars) {
        //     score_b += csat->var_mem[fanout_vref].clauses.size();
        // }

        return var_a.clauses.size() > var_b.clauses.size();
    }
    LessActivity(): csat(nullptr), activity(nullptr) {}
    LessActivity(const CSat *csat, const int *activity): csat(csat),activity(activity) {}
};


void CSat::resolve() {

    int old_clauses = clauseDB.size();

    int *resolve_cost = new int[num_vars + 1];

    Heap<LessActivity> heap;
    heap.setComp(LessActivity(this, resolve_cost));

    for(int i=num_inputs+1; i<=num_vars; i++) {
        if(VAR(i).fanout_vars.size() == 1) {
            resolve_cost[i] = resolve_var(i, 1);
        }
    }
    for(int i=num_inputs+1; i<=num_vars; i++) {
        if(VAR(i).fanout_vars.size() == 1) {
            heap.insert(i);
        }
    }

    int num_resolved = 0;

    while(!heap.empty()) {
        VRef vref = heap.pop();
        Variable &var = VAR(vref);
        if(var.fanout_vars.size() != 1) continue;

        VRef fanout_vref = var.fanout_vars[0];
        Variable &fanout_var = VAR(fanout_vref);

        if(var.type == XOR || fanout_var.type == XOR) {
            continue;
        }

        // if(resolve_cost[vref] >= 500) break;

        if(fanout_var.fanin_lits.size() + var.fanin_lits.size() - 1 > OPT(max_lut_input)) continue;

        int cost = resolve_var(vref, 0);

        // printf("%d ", cost);

        assert(cost == resolve_cost[vref]);

        var.is_delete = 1;

        num_resolved++;

        for(int fanin_lit : fanout_var.fanin_lits) {
            VRef fanin_ref = (fanin_lit / 2);
            Variable &fanin_var = VAR(fanin_ref);
            if(heap.inHeap(fanin_ref) && fanin_var.fanout_vars.size() == 1) {
                resolve_cost[fanin_ref] = resolve_var(fanin_ref, 1);
                heap.update(fanin_ref);
            }
        }

        if(fanout_var.fanout_vars.size() == 1) {
            // assert(heap.inHeap(fanout_vref));
            if(heap.inHeap(fanout_vref)) {
                resolve_cost[fanout_vref] = resolve_var(fanout_vref, 1);
                heap.update(fanout_vref);
            }
        }
    }

    shrink_clauses();

    printf("c num_resolved %d (%.2f%%) vars: %d -> %d clauses: %d -> %d\n", num_resolved, (double)num_resolved/num_vars*100, num_vars, num_vars - num_resolved, old_clauses, num_gate_clauses);

    delete[] resolve_cost;
}

int CSat::resolve_var(VRef vref, bool test) {

    Variable &var = VAR(vref);

    // if(var.clauses.size() >= 100) return 1000000;

    assert(var.fanout_vars.size() == 1);

    double total_resolved_lits = 0;

    int pos_lit = var.out;
    int neg_lit = (var.out ^ 1);

    vec<CRef> pos_clauses;
    vec<CRef> neg_clauses;

    for(CRef cref : var.clauses) { 
        if(clauseDB[cref].find_lit_swap_first(pos_lit)) pos_clauses.push(cref);
        else if(clauseDB[cref].find_lit_swap_first(neg_lit)) neg_clauses.push(cref);
        else assert(false); // 一个门的子句不可能不包含自己的输出变量

        total_resolved_lits -= clauseDB[cref].size;
    }

    VRef fanout_vref = var.fanout_vars[0];
    Variable &fanout_var = VAR(fanout_vref);

    int num_clauses = fanout_var.clauses.size();

    int clause_idx = 0;
    for(CRef cref : fanout_var.clauses) { 
        Clause &cls = clauseDB[cref];
        if(cls.find_lit_swap_first(pos_lit)) pos_clauses.push(cref);
        else if(cls.find_lit_swap_first(neg_lit)) neg_clauses.push(cref);
        else if(!test) fanout_var.clauses[clause_idx++] = cref;
        total_resolved_lits -= cls.size;
    }
    if(!test) fanout_var.clauses.setsize(clause_idx);

    // 子句笛卡尔乘积
    for(CRef p_clause_ref : pos_clauses) {
        Clause &p_clause = clauseDB[p_clause_ref];
        for(CRef n_clause_ref : neg_clauses) {
            Clause &n_clause = clauseDB[n_clause_ref];
            
            std::unordered_set<int> resolved_lits;

            bool has_complementary = false;

            for(int i=1; i<p_clause.size; i++) {
                int lit = p_clause[i];
                if(resolved_lits.count(lit)) continue;
                if(resolved_lits.count(notlit(lit))) { has_complementary = true; break; };
                resolved_lits.insert(lit);
            }

            if(has_complementary) continue;

            for(int i=1; i<n_clause.size; i++) {
                int lit = n_clause[i];
                if(resolved_lits.count(lit)) continue;
                if(resolved_lits.count(notlit(lit))) { has_complementary = true; break; };
                resolved_lits.insert(lit);
            }

            if(has_complementary) continue;

            if(!test) {
                CRef resolved_cref = clauseDB.allocate(resolved_lits.size());
                Clause &resolved_clause = clauseDB[resolved_cref];
                int tmp_ptr = 0;

                for(int lit : resolved_lits) {
                    resolved_clause[tmp_ptr++] = lit;
                }
                assert(tmp_ptr >= 2);
                assert(tmp_ptr == resolved_clause.size);
                fanout_var.clauses.push(resolved_cref);
            }

            total_resolved_lits += resolved_lits.size();
        }
    }

    if(!test) {

        for(CRef p_clause_ref : pos_clauses) { clauseDB[p_clause_ref].is_deleted = 1; }
        for(CRef n_clause_ref : neg_clauses) { clauseDB[n_clause_ref].is_deleted = 1; }

        fanout_var.type = LUT;

        assert(fanout_var.fanin_lits.erase(pos_lit) || fanout_var.fanin_lits.erase(neg_lit));

        for(int fanin_lit : var.fanin_lits) {
            fanout_var.fanin_lits.push(fanin_lit);
            Variable &fanin_var = VAR(lit2pvar(fanin_lit));
            fanin_var.fanout_vars.push(fanout_vref);
            fanin_var.fanout_vars.erase(vref);
            fanin_var.fanout_vars.unique();
        }

        fanout_var.fanin_lits.unique();
    }

    return total_resolved_lits;
}