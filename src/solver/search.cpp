#include <cmath>

#include "solver.h"

int CSat::solve() {

	build_data_structure();

	printf("c num_clauses: %d\n", clauseDB.size());

	auto resol_start = std::chrono::high_resolution_clock::now();

	// simulate();
	
	if(OPT(enable_elim)) {		
		resolve();
		printf("c elimination time: %.2f s\n", std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - resol_start).count() / 1e9);
	} else {
		printf("c elimination disabled\n");
	}

	num_gate_clauses = clauseDB.size();

	printf("c enable xvsids: %d\n", OPT(enable_xvsids));

	// lut_simulate();

	if(OPT(pre_out) != "") {
		printf("c write cnf to path: %s\n", OPT(pre_out).c_str());
		write_cnf();
		return 0;
	}

	build_watches();

	initVsidsMode();

	initVmtfMode();

	if(OPT(branchMode) == 2) {
		printf("c branch: jfontier\n");
		switchJfontierMode();
	} else if(OPT(branchMode) == 0) {
		printf("c branch: VMTF\n");
		swtichVmtfMode();
	} else {
		printf("c branch: VSIDS\n");
		switchVsidsMode();
	}

	// start search
	for (int i = 0; i < outputs.size(); i++) {
		assign(outputs[i], 0, std::make_pair(0, -3));
	}

	auto start = std::chrono::high_resolution_clock::now();

	double propagate_time = 0;
	double analyze_time = 0;
	double backtrak_time = 0;
	double decide_time = 0;

	auto last_time = std::chrono::high_resolution_clock::now();	

	while (true) {

		auto print_duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - last_time);

		if (print_duration.count() >= 1e3) {
			last_time = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start);
			double speed = conflicts / (duration.count() / 1000.0);
			printf("c num_conflicts: %d  (%.2lf / Sec)  num_clauses: %d  "
			       "num_cwatches: %lu (%.2lf / Sec) jheap: %d\n",
			       conflicts, speed, clauseDB.size(), num_circuit_watches, num_circuit_watches / (duration.count() / 1000.0), jheap.size());
			printf("total: %.2f propagte: %.2f analyze: %.2f decide: %.2f backtack: %.2f\n", duration.count() / 1e3, propagate_time / 1e9, decide_time /1e9, analyze_time / 1e9, backtrak_time / 1e9);
			int num_total_watch = 0;
			for (int lit = 2; lit <= (num_vars * 2 + 1); lit++) {
				num_total_watch += watches[lit].size();
			}

			// int just = 0;
			// int __all_V = 0;
			// for(int i=num_inputs+1; i<=num_vars; i++) {
			// 	Variable &var = VAR(i);
			// 	if(var.is_delete) continue;
			// 	if(!var.is_cwatch) continue;
			// 	int index = generateBase3Value(var, value);
			// 	if(var.just_table[index]) just++;
			// 	__all_V++;
			// }

			// printf("c just-rate %.2f ( %d / %d )\n", (double) just / __all_V, just, __all_V );
		}

		// show_circuit();

		// printf("????????\n");

		auto pro_start = std::chrono::high_resolution_clock::now();
		int res = propagate();
		
		propagate_time += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - pro_start).count();

		if (res == 0) {

			int lbd;
			int cipher_cnt;
			auto anal_start = std::chrono::high_resolution_clock::now();

			
			int backtrack_level = analyze(lbd, cipher_cnt);
			
			analyze_time += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - anal_start).count();

			// printf("back to %d\n", backtrack_level);
			if (backtrack_level == -1) {
				// while(!heap.empty()) heap.pop();
				// for (int i = 1; i <= num_vars; i++) {
				// 	if (VAR(i).is_delete) continue;
				// 	heap.insert(i);
				// }

				// while(!heap.empty()) {
				// 	VRef vref = heap.pop();
				// 	Variable &var = VAR(vref);
				// 	printf("var: %d vsids: %.2f fanout: %d fanin: %d lutp: %.2f simp: %.2f xor_edges: %d\n", vref, var_activity[vref], var.fanout_vars.size(), var.fanin_lits.size(), var.lut_phase, simu_prop[vref], var.xor_edges);
				// }
				return 20; // conflict analyze & backtrack & decide one lit
			}
			auto back_start = std::chrono::high_resolution_clock::now();
			backtrack(backtrack_level);
			backtrak_time += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - back_start).count();

			

			if (learn.size() == 1) {
				assign(learn[0], 0, std::make_pair(0, -3));
			}
			else if(learn.size() == 2) {
				watches_bin[notlit(learn[1])].push(learn[0]);
				watches_bin[notlit(learn[0])].push(learn[1]);
				assign(learn[0], backtrack_level, std::make_pair(notlit(learn[1]), -1));
			} else {
				CRef cref = add_learning_clause(lbd, cipher_cnt);
				bump_clause(clauseDB[cref], 1);
				assign(learn[0], backtrack_level, std::make_pair(cref, -4));
			}
			var_inc *= (1.0 / 0.8);
			clause_inc *= (1.0 / 0.8);
			++reduces;
			++conflicts;
			++rephases;
			if ((int)trail.size() > threshold) { // update the local-best phase
				threshold = trail.size();
				int cnt_0 = 0, cnt_1 = 0;
				for (int i = 1; i <= num_vars; i++) {
					local_best[i] = value[i];
					if (local_best[i] == 0)
						local_best[i] = 1;
					if (local_best[i] == 1)
						cnt_1++;
					else
						cnt_0++;
				}
			}

		} else if (conflicts >= curRestart * reduce_limit) {
			reduce();
			curRestart = (conflicts / reduce_limit) + 1;
			int reduce_limit_inc = OPT(reduce_limit_inc);
			reduce_limit += reduce_limit_inc;
		} else if (lbd_queue_size == 50 && 0.80 * fast_lbd_sum / lbd_queue_size > slow_lbd_sum / conflicts) {
			restart();
		} else if (rephases >= rephase_limit) {
			rephase();
		} else {
			auto decide_start = std::chrono::high_resolution_clock::now();			
			int ilit = decide();
			decide_time += std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - decide_start).count();
			if (ilit == -1) {
				for (int i = 1; i <= num_vars; i++) {
					if(VAR(i).is_delete) {
						assert(value[i*2] == 0);
					}
				}
				return 10;
			}
		}
	}

	return 0;
}