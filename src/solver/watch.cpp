#include <cassert>

#include "solver.h"


void CSat::build_watches() {
    // init gate clauses;
	for (int i = num_inputs + 1; i <= num_vars; i++) {
		Variable &var = VAR(i);
        if(VAR(i).is_delete) continue;
		// add gate watches;
		for(CRef cref : var.clauses) {
			Clause &clause = clauseDB[cref];
            assert(clause.size >= 2);
            if(clause.size == 2) {
                watches_bin[notlit(clause[0])].push(clause[1]);
                watches_bin[notlit(clause[1])].push(clause[0]);
            } else if(clause.size == 3) {
                watches_tri[notlit(clause[0])].push(cref);
                watches_tri[notlit(clause[1])].push(cref);
            } else {
                watches[notlit(clause[0])].push(Watcher{cref, clause[1]});
                watches[notlit(clause[1])].push(Watcher{cref, clause[0]});
            }
		}
	}
}