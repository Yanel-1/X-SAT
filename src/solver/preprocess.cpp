#include "solver.h"

void CSat::write_cnf() {

    std::ofstream out(OPT(pre_out), std::ios::out);
    assert(out.is_open());

    out << "p cnf " << num_vars << " " << num_gate_clauses + outputs.size() << std::endl;

    for(int i=num_inputs+1; i<=num_vars; i++) {
        if(VAR(i).is_delete) continue;
        for(CRef cref : VAR(i).clauses) {
            Clause& cls = clauseDB[cref];
            for(int j=0; j<cls.size; j++) {
                int v = lit2pvar(cls[j]);
                if(cls[j] & 1) out << -v << " ";
                else out << v << " ";
            }
            out << "0" << std::endl;
        }
    }

    for (int i = 0; i < outputs.size(); i++) {

        int v = lit2pvar(outputs[i]);
        if(outputs[i] & 1) {
            out << -v << " 0" << std::endl;
        } else {    
            out << v << " 0" << std::endl;
        }
		
	}

    out.close();

    printf("c write cnf done\n");
}