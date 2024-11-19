#include "solver.h"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>

CSat::~CSat() {
	delete[] var_stamp;
	delete[] watches;
	delete[] watches_bin;
	delete[] watches_tri;
	delete[] assigned;
	delete[] value;
	delete[] mark;
	delete[] injheap;
	delete[] var_activity;
	delete[] gate_activity;
	delete[] saved;
	delete[] local_best;
	delete[] var_mem;
	delete[] lit_mem;
	delete[] assgined_vars_by_level;
	delete[] simu_prop;
	delete[] cwatches;
	delete[] tabu_stamp_arr;
	delete[] vmtf_stamp;
}

void CSat::alloc_memory(int maxvar) {
	int maxlit = maxvar << 1 | 1;

	var_mem = new Variable[maxvar + 1];
	lit_mem = new Literal[maxlit + 1];

	tabu_stamp_arr = new int[maxvar + 1];

	var_stamp = new int[maxvar + 1];
	simu_prop = new double[maxvar + 1];

	saved = new int[maxvar + 1]{0};
	local_best = new int[maxvar + 1]{0};

	for (int i = 1; i <= maxvar; i++)
		var_stamp[i] = 0;

	watches = new vec<Watcher>[maxlit + 1];
	watches_bin = new vec<int>[maxlit + 1];
	watches_tri = new vec<int>[maxlit + 1];
	cwatches = new std::unordered_set<int>[maxlit + 1];

	assigned = new type_assigned[maxvar + 1];
	assgined_vars_by_level = new vec<int>[maxvar + 1];
	value = new int[maxlit + 1];
	mark = new int[maxvar + 1]{0};

	vmtf_stamp = new int[maxvar + 1]{0};

	for (int i = 1; i <= maxlit; i++)
		value[i] = 0;
	for (int i = 1; i <= maxvar; i++)
		assigned[i].level = 0;

	injheap = new int[maxvar + 1];
	var_activity = new double[maxvar + 1];
	heap.setComp(GreaterActivity(var_activity));
	gate_activity = new double[maxvar + 1];
	jheap.setComp(GreaterActivity(gate_activity));

	tabu_heap.setComp(TBLessActivity(tabu_stamp_arr));

	// !!!!!!!!!!!
	raq.value = value;
}

void CSat::bump_clause(Clause &cls, double coeff) {
	if ((cls.activity += clause_inc * coeff) > 1e100) {
		for (CRef cref = 0; cref < clauseDB.end();) {
			Clause& cls = clauseDB[cref];
			cref += sizeof(Clause) / 4 + cls.size;
			if (!cls.is_deleted)
				cls.activity *= 1e-100;
		}
		clause_inc *= 1e-100;
	}
}

void CSat::assign(int lit, int level, std::pair<int, int> reason) {
	// if (reason.second == -1) printf("assign %d, level %d, reason is direct implicate %d\n", lit, level, reason.first);
	// else if (reason.second == -3) printf("assign %d, level %d, reason is OUTPUT\n", lit, level);
	// else if (reason.second == -2) printf("assign %d, level %d, reason is decision var\n", lit, level);
	// else if (reason.second == -4) printf("assign %d, level %d, reason is watch implicate with clause %d\n", lit, level, reason.first);
	// else if (reason.second == -0) printf("assign %d, level %d, reason is and-gate implicate %d\n", lit, level, reason.first);
	// else if (reason.second == 1) printf("assign %d, level %d, reason is xor-gate implicate %d\n", lit, level, reason.first);
	// else { assert(false); }
	assert(value[lit] == 0);

	int g = lit2pvar(lit);
	value[lit] = 1, value[notlit(lit)] = -1;
	assigned[g].level = level;
	assigned[g].reason = reason;
	trail.push(lit);

if(branchMode == BranchMode::JFRONTIER) {
	if (!injheap[g]) {
		
        // for(int fanin_lit : VAR(g).fanin_lits) {
        //   VRef fanin_vref = lit2pvar(fanin_lit);
        //   if(value[fanin_lit] != 0) continue;
        //   gate_activity[g] = std::max(gate_activity[g], var_activity[fanin_vref]);
        // }

		heap_insert(g);
	}
}

}

void CSat::build_data_structure() {
	// init fanouts
	for (int i = num_inputs + 1; i <= num_vars; i++) {
		Variable &var = VAR(i);
		for (int fanin_lit : var.fanin_lits) {
			int fanin_var = lit2pvar(fanin_lit);
			VAR(fanin_var).fanout_vars.push(i);
		}
	}

	// init gate clauses;
	for (int i = num_inputs + 1; i <= num_vars; i++) {
		Variable &var = VAR(i);
		assert(var.fanin_lits.size() == 2);
		int i0 = var.fanin_lits[0], i1 = var.fanin_lits[1], out = var.out;
		if(var.type == AND) {
			CRef cr0, cr1, cr2;
			Clause &c0 = clauseDB[cr0 = clauseDB.allocate(2)];
			Clause &c1 = clauseDB[cr1 = clauseDB.allocate(2)];
			Clause &c2 = clauseDB[cr2 = clauseDB.allocate(3)];
			c0[0] = (out ^ 1); c0[1] = i0;
			c1[0] = (out ^ 1); c1[1] = i1;
			c2[0] = out; c2[1] = (i0 ^ 1); c2[2] = (i1 ^ 1);
			var.clauses.push(cr0); var.clauses.push(cr1); var.clauses.push(cr2);
		} else {
			assert(var.type == XOR);
			CRef cr0, cr1, cr2, cr3;
			Clause &c0 = clauseDB[cr0 = clauseDB.allocate(3)];
			Clause &c1 = clauseDB[cr1 = clauseDB.allocate(3)];
			Clause &c2 = clauseDB[cr2 = clauseDB.allocate(3)];
			Clause &c3 = clauseDB[cr3 = clauseDB.allocate(3)];
			c0[0] = (out ^ 1); c0[1] = i0; c0[2] = i1;
			c1[0] = out; c1[1] = (i0 ^ 1); c1[2] = i1;
			c2[0] = out; c2[1] = i0; c2[2] = (i1 ^ 1);
			c3[0] = (out ^ 1); c3[1] = (i0 ^ 1); c3[2] = (i1 ^ 1);
			var.clauses.push(cr0); var.clauses.push(cr1);
			var.clauses.push(cr2); var.clauses.push(cr3);
		}
	}
}

CRef CSat::add_learning_clause(int lbd, int cipher_cnt) {
	assert(learn.size() > 0);

	CRef cref = clauseDB.allocate(learn.size());
	Clause& clause = clauseDB[cref];
	clause.lbd = lbd;
	clause.size = learn.size();
	clause.activity = 0;
	clause.is_deleted = 0;
	clause.learn = 1;

	for (int i = 0; i < learn.size(); i++) {
		clause[i] = learn[i];
	}

	if(clause.size == 3) {
		watches_tri[notlit(clause[0])].push(cref);
		watches_tri[notlit(clause[1])].push(cref);
	} else {
		watches[notlit(clause[0])].push(Watcher{cref, clause[1]});
		watches[notlit(clause[1])].push(Watcher{cref, clause[0]});
	}

	return cref;
}

void CSat::restart() {
	fast_lbd_sum = lbd_queue_size = lbd_queue_pos = 0;
	backtrack(0);
	int phase_rand = rand() % 100; // probabilistic rephasing
	if ((phase_rand -= 60) < 0)
		for (int i = 1; i <= num_vars; i++)
			saved[i] = local_best[i];
	else if ((phase_rand -= 5) < 0)
		for (int i = 1; i <= num_vars; i++)
			saved[i] = -local_best[i];
	else if ((phase_rand -= 10) < 0)
		for (int i = 1; i <= num_vars; i++)
			saved[i] = rand() % 2 ? 1 : -1;
}

void CSat::rephase() { 
	// printf("Rephasing!!!!!!!!\n");
	rephases = 0, threshold *= 0.9, rephase_limit += 8192;
}