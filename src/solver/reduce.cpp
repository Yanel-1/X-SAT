#include <algorithm>

#include "solver.h"


void CSat::shrink_clauses() {
    int maxlit = num_vars << 1 | 1;
    restart();

    ClauseDB new_clauseDB(clauseDB.capacity);

    std::unordered_map<CRef, CRef> reduce_map;

    for(int i=0; i<clauseDB.clauses.size(); i++) {
        CRef cref = clauseDB.clauses[i];
        Clause& clause = clauseDB[cref];

        if(clause.is_deleted) continue;

        CRef new_cref = new_clauseDB.allocate(clause.size);
        Clause& new_clause = new_clauseDB[new_cref];
        new_clause.lbd = clause.lbd;
        new_clause.size = clause.size;
        new_clause.activity = clause.activity;
        new_clause.is_deleted = clause.is_deleted;
        new_clause.learn = clause.learn;
        for(int i=0; i<clause.size; i++) {
          new_clause[i] = clause[i];
        }
        reduce_map[cref] = new_cref;
    }

    for(int lit=2; lit<=maxlit; lit++) {
        int old_sz = watches[lit].size();
        int new_sz = 0;
        for(int i=0; i<old_sz; i++) {
            CRef cref = watches[lit][i].cref;
            if(reduce_map.count(cref) == 0) continue;
            watches[lit][new_sz++] = Watcher{reduce_map[cref], watches[lit][i].blocker};
        }
        watches[lit].setsize(new_sz);
    }

    for(int lit=2; lit<=maxlit; lit++) {
        int old_sz = watches_tri[lit].size();
        int new_sz = 0;
        for(int i=0; i<old_sz; i++) {
            CRef cref = watches_tri[lit][i];
            if(reduce_map.count(cref) == 0) continue;
            watches_tri[lit][new_sz++] = reduce_map[cref];
        }
        watches_tri[lit].setsize(new_sz);
    }

    for(int i = num_inputs + 1; i <= num_vars; i++) {
      if(VAR(i).is_delete) continue;
      for(CRef &cref : VAR(i).clauses) {
        assert(reduce_map.count(cref));
        cref = reduce_map[cref];
      }
    }

    free(clauseDB.memory);

    clauseDB = new_clauseDB;

    for(int i = num_inputs + 1; i <= num_vars; i++) {
      if(VAR(i).is_delete) continue;
      for(CRef &cref : VAR(i).clauses) {
        assert(clauseDB[cref].size >= 2);
      }
    }
}

void CSat::reduce() {

    int maxlit = num_vars << 1 | 1;

    double clause_len = 0;
    double num_clauses = 0;

    std::vector<std::pair<Clause*, CRef>> tmp_sort;


    for(int i=num_gate_clauses; i<clauseDB.clauses.size(); i++) {
      CRef cref = clauseDB.clauses[i];
      Clause &clause = clauseDB[cref];
      assert(clause.learn);
      if(!clauseDB[cref].is_deleted) {
        clause_len += clause.size;
        num_clauses++;
        tmp_sort.push_back(std::make_pair(&clauseDB[cref], cref));
      }
    }

    // printf("c avg len: %.2f ( %.2f / %.2f )\n", clause_len / num_clauses, clause_len, num_clauses);

    std::sort(tmp_sort.begin(), tmp_sort.end(), [](const std::pair<Clause*, CRef> lhs, const std::pair<Clause*, CRef> rhs){
          if(lhs.first->lbd != rhs.first->lbd) return lhs.first->lbd < rhs.first->lbd;
          return lhs.first->activity > rhs.first->activity;
    });

    int save_per=OPT(reduce_per);
    for(int i=tmp_sort.size()  * save_per / 100; i<tmp_sort.size(); i++) {
      clauseDB.free(tmp_sort[i].second);
    }

    for(int lit=2; lit<=maxlit; lit++) {
        int old_sz = watches[lit].size();
        int new_sz = 0;
        for(int i=0; i<old_sz; i++) {
            CRef cref = watches[lit][i].cref;
            if(!clauseDB[cref].is_deleted) {
              watches[lit][new_sz++] = watches[lit][i];
            }
        }
        watches[lit].setsize(new_sz);
    }

    // check garbage

    if((double)clauseDB.wasted/clauseDB.end_index < 0.2) return;

    shrink_clauses();
    
}