#pragma once

#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include <set>
#include <cassert>

#include "libs/heap.h"
#include "conf/options.hpp"
#include "clause.hpp"
#include "chrono"
#include "solver.h"
#include "libs/vec.h"
#include "libs/ra_queue.hpp"

#define VAR(x) (var_mem[x])
#define LIT(x) (lit_mem[x])

typedef int lit;
inline int lit2pvar(lit x) { return x >> 1; }
inline int lit2var (lit x) { return (x & 1) ? -(x >> 1) : (x >> 1); }
inline int var2lit (int x) { return (x > 0) ? (x << 1) : ((-x) << 1 | 1); }
inline int litsign (lit x) { return (x & 1) ? -1 : 1; }
inline int varsign (int x) { return x > 0 ? 1 : -1; }
inline int litval  (lit x) { return (x & 1) ? 0 : 1; }
inline int varval  (int x) { return x > 0 ? 1 : 0; }
inline int notlit  (lit x) { return x ^ 1; }
inline lit makelit (int x, int val) { return (val == 1 ? (x << 1) : (x << 1 | 1)); }



typedef unsigned VRef;

enum gate_type { UNUSE, AND, XOR, INPUT, LUT }
;
const std::string gate_type[5] = { "UNUSE", "AND", "XOR", "INPUT", "LUT" };

enum class BranchMode {
    VSIDS,
    JFRONTIER,
    VMTF
};

struct Variable {
    bool is_input;
    bool is_output;
    int type;
    int out;
    vec<int> fanin_lits;
    vec<int> fanout_vars;
    vec<CRef> clauses;
    vec<int> neighbors;
    int is_delete;
    int decide_by;
    int value; // only for simulate
    int xor_edges;
    double lut_phase;
    Variable ():is_delete(0),decide_by(0),value(0),xor_edges(0) {}
};

struct Literal {
    // and watch
    int *fanout_AND_start;
    int *fanout_AND_end;
    // direct
    int *direct_start;
    int *direct_end;
};

struct type_assigned {
    int level;
    std::pair<int, int> reason;
};

struct GreaterActivity {        // A compare function used to sort the activities.
    const double *activity;     
    bool operator() (int a, int b) const { return activity[a] > activity[b]; }
    GreaterActivity(): activity(nullptr) {}
    GreaterActivity(const double *s): activity(s) {}
};

struct TBLessActivity {        // A compare function used to sort the activities.
    const int *activity;     
    bool operator() (int a, int b) const { return activity[a] < activity[b]; }
    TBLessActivity(): activity(nullptr) {}
    TBLessActivity(const int *s): activity(s) {}
};

inline int generateBase3Value(Variable &var, const int* value) {    
    int base3_value = (value[var.out] + 1);
    for (const auto &lit : var.fanin_lits) {
        int state = (value[lit] + 1);
        base3_value = 3 * base3_value + state;
    }
    return base3_value;
}

class CSat {
public: 
    ~CSat();
    CSat() {
        
    }

    // statitics
    uint64_t num_analyze_loop = 0;
    uint64_t num_circuit_watches = 0;

    char* gmem;

    int num_inputs;
    int num_outputs;
    int num_ands;
    int num_xors;
    int num_vars;
    int num_gate_clauses;

    BranchMode branchMode;
    
    vec<int> inputs, outputs, tmp;

    Variable *var_mem;
    Literal *lit_mem;

    std::queue<int> que;
    int stamp = 0, *var_stamp;

    double *var_activity, *gate_activity;
    Heap<GreaterActivity> jheap;
    Heap<GreaterActivity> heap;
    int *injheap;

    vec<int> *assgined_vars_by_level;
    std::unordered_set<int> *cwatches;

    struct Watcher {
      CRef cref;
      int blocker;
    };

    ClauseDB clauseDB;

    double* simu_prop;

    vec<Watcher> *watches;
    vec<int> *watches_bin;
    vec<int> *watches_tri;

    vec<int> free_gates;
    
    type_assigned *assigned;
    int *value;
    vec<int> trail, conflict, learn, delay_watch, pos_in_trail, bump;  // pos_in_trail[i]: the pos of ((Level i)'s decision variable) in trail
    int propagated = 0;

    double var_inc = 1;
    double clause_inc = 1;
    int curRestart = 1;
    int reduce_limit = 8192;
    int reduces = 0;
    int *mark; // 计算 lbd 的辅助数组
    int time_stamp = 0;
    vec<int> reduce_map;

    int restarts = 0;
    int conflicts = 0;
    int rephases = 0, rephase_limit = 1024;
    int threshold = 0;
    int *saved, *local_best;
    int watches_cnt = 0;

    int lbd_queue[50] = {0},                              // circled queue saved the recent 50 LBDs.
        lbd_queue_size = 0,                             // The number of LBDs in this queue
        lbd_queue_pos = 0;                              // The position to save the next LBD.
    double fast_lbd_sum = 0, slow_lbd_sum = 0;              // Sum of the Global and recent 50 LBDs.

    Heap<TBLessActivity> tabu_heap;
    int *tabu_stamp_arr;
    int tabu_stamp = 0;

    void add_watch(CRef cref);
    
    void show_circuit();
    void parse_aig(const char* filename);
    void alloc_memory(int maxvar);
    void build_data_structure();
    void build_watches();
    int  solve();
    void assign(int lit, int level, std::pair<int, int> reason);
    int  propagate();
    int  decide();
    int  analyze(int &lbd,int &cipher_cnt);
    void reduce();
    void restart();
    void rephase();
    void backtrack(int backtrack_level);
    CRef add_learning_clause(int lbd ,int cipher_cnt);
    void bump_var(int var, double coeff);
    void bump_clause(Clause &cls, double coeff);
    void heap_insert(int var);
    void heap_remove(int var);
    bool is_jnode(int var);
    bool is_jreason(int var);
    void resolve();
    int resolve_var(VRef vref, bool test = 1);
    void shrink_clauses();

    std::vector<int> topo;
    void cal_topo();

    void simulate();
    void write_cnf();

    int queue_index = 0;
    int queue_stamp = 0;
    int *vmtf_stamp;
    RaQueue<int> raq;


    void initVsidsMode();
    void initJfontierMode();
    void initVmtfMode();
    void switchVsidsMode();
    void switchJfontierMode();
    void swtichVmtfMode();
    int  branchVsidsMode();
    int  branchJfontierMode();
    int  branchVmtfMode();
    void bump_vmtf();


    void lut_simulate();

    bool test_jheap_score();
    bool test_in_jheap();
};