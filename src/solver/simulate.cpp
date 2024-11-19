#include <unordered_map>

#include "solver.h"

void CSat::lut_simulate() {

    int *tmp_value = new int[num_vars*2+1];
    memset(tmp_value, 0, sizeof(tmp_value));

    for(int i=num_inputs+1; i<=num_vars; i++) {
        Variable &var = VAR(i);
        if(var.is_delete) continue;

        if(var.fanin_lits.size() > 8) {
            var.lut_phase = 0.5;
            continue;
        }

        int num_pos = 0;
        int num_neg = 0;

        for(int state=0; state<(1<<var.fanin_lits.size()); state++) {
            // generate assigenment for LUT input
            std::vector<int> vals;
            int tmp_state = state;
            while(tmp_state) {
                vals.push_back(tmp_state % 2);
                tmp_state /= 2;
            }
            while(vals.size() < var.fanin_lits.size()) vals.push_back(0);
            assert(vals.size() == var.fanin_lits.size());
            for(int i=0; i<vals.size(); i++) {
                if(vals[i] == 1) tmp_value[var.fanin_lits[i]] = 1;
                else tmp_value[var.fanin_lits[i]] = -1;
                tmp_value[var.fanin_lits[i] ^ 1 ] = - tmp_value[var.fanin_lits[i]];
            }

            int final_olit = -1;

            // cal_result
            for(CRef cref : var.clauses) {
                Clause &cls = clauseDB[cref];
                int olit = -1;
                bool ok = 1;
                for(int j=0; j<cls.size; j++) {
                    if(lit2pvar(cls[j]) == i) {
                        olit = cls[j];
                        continue;
                    }
                    assert(tmp_value[cls[j]] != 0);
                    if(tmp_value[cls[j]] == 1)  {
                        ok = 0;
                        break;
                    }
                }

                if(ok) {
                    assert(olit != -1);
                    // printf("%d\n", olit);
                    if(final_olit == -1) final_olit = olit;
                    else assert(final_olit == olit);
                }

            }

            assert(final_olit != -1);

            if(final_olit & 1) {
                num_neg++;
            } else {
                num_pos++;
            }
        }

        var.lut_phase = (double)num_pos / (num_pos + num_neg);
    }

    delete []tmp_value;
}

void CSat::cal_topo() {
    std::queue<int> q;

    for(int i=1; i<=num_inputs; i++) {
        q.push(i);
    }

    std::unordered_map<int, int> num_fanin_assigned;

    while(!q.empty()) {
        int now = q.front(); q.pop();
        for(VRef fanout_vref : VAR(now).fanout_vars) {
            num_fanin_assigned[fanout_vref]++;
            if(num_fanin_assigned[fanout_vref] == VAR(fanout_vref).fanin_lits.size()) {
                q.push(fanout_vref);
                topo.push_back(fanout_vref);
            }
        }
    }

    assert(topo.size() + num_inputs == num_vars);
}

void CSat::simulate() {

    cal_topo();

    int round = 1e8 / num_vars;

    printf("round: %d\n", round);

    std::unordered_map<int, int> pos_cnt;
    std::unordered_map<int, int> neg_cnt;

    for(int r = 1; r <= round; r++) {
        // randomly set input vector
        for(int i=1; i<=num_inputs; i++) {
            VAR(i).value = rand() % 2;
        }
        // simulate
        for(VRef vref : topo) {
            Variable &var = VAR(vref);            
            assert(var.fanin_lits.size() == 2);
            int lit0 = var.fanin_lits[0];
            int lit1 = var.fanin_lits[1];
            int var0 = lit2pvar(lit0);
            int var1 = lit2pvar(lit1);
            int value0 = VAR(var0).value ^ (lit0 & 1);
            int value1 = VAR(var1).value ^ (lit0 & 1);
            if(var.type == AND) {
                var.value = (value0 & value1);
            } else if(var.type == XOR) {
                var.value = (value0 ^ value1);
            } else {
                assert(false);
            }
        }
        for(int i=1; i<=num_vars; i++) {
            if(VAR(i).value == 1) {
                pos_cnt[i]++;
            } else {
                neg_cnt[i]++;
            }
        }
    }

    for(int i=1; i<=num_vars; i++) {
        simu_prop[i] = (double)pos_cnt[i] / (pos_cnt[i] + neg_cnt[i]);
        // printf("var: %d pro %.2f\n", i, simu_prop[i]);
    }
}