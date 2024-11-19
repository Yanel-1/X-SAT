#include "solver.h"

int CSat::analyze(int &lbd,int &cipher_cnt)
{ // First-UIP
  ++stamp;
  bump.clear();
  learn.clear();
  int highestlevel = -1;
  for (int i = 0; i < conflict.size(); i++)
    if (assigned[lit2pvar(conflict[i])].level > highestlevel)
      highestlevel = assigned[lit2pvar(conflict[i])].level;
  assert(highestlevel == assigned[lit2pvar(conflict[0])].level);
  if (highestlevel == 0)
    return -1;
  learn.push(0);
  int should_visit_ct = 0, resolve_lit = 0, index = trail.size() - 1;
  // printf("======================================================================\n");
  do
  {
    // printf("resolve_lit: %d ->", resolve_lit);
    // for (int r = 0; r < conflict.size(); r++) printf("%d ", conflict[r]);
    // puts("0");
    
    for (int i = 0; i < conflict.size(); i++)
    {
      int var = lit2pvar(conflict[i]);
      // printf("var: %d  resolve_lit: %d\n", var, resolve_lit);
      assert(var != lit2pvar(resolve_lit));
      if (var_stamp[var] != stamp && assigned[var].level > 0)
      {
        bump_var(var, 1);
        bump.push(var);
        var_stamp[var] = stamp;
        if (assigned[var].level >= highestlevel)
          should_visit_ct++; //, printf("%d + 1\n", conflict[i]);
        else
          learn.push(conflict[i]);
      }
    }

    while (var_stamp[lit2pvar(trail[index--])] != stamp);

    resolve_lit = trail[index + 1];

    should_visit_ct--;
    int resolve_var = lit2pvar(resolve_lit);

    // bump.push(resolve_var);

    var_stamp[resolve_var] = 0;
    if (!should_visit_ct)
      break;

    std::pair<int, int> reason = assigned[resolve_var].reason;
    // assert(reason.second >= -1); // not OUTPUT & decision variable
    conflict.clear();
    if (reason.second == -1)
    { // direct imply (Binary clause)
      conflict.push(notlit(reason.first));
    }
    else if(reason.second == -4)
    { 
      // watches imply
      CRef cref = reason.first;
      Clause& cls = clauseDB[cref];
      assert(lit2pvar(cls[0]) == resolve_var);
      for (int i = 1; i < cls.size; i++) {
        conflict.push(cls[i]);
      }
      bump_clause(cls, 1);
    }
    else
      printf("wrong %d\n", reason.second), exit(-1);
  } while (should_visit_ct > 0);

  int decision_lit = notlit(resolve_lit);
  learn[0] = decision_lit;
  int backtrack_level = 0;

  if (learn.size() == 1)
    backtrack_level = 0;
  else
  {
    int max_id = 1;
    for (int i = 2; i < learn.size(); i++)
      if (assigned[lit2pvar(learn[i])].level >
          assigned[lit2pvar(learn[max_id])].level)
        max_id = i;
    int tmp = learn[max_id];
    learn[max_id] = learn[1], learn[1] = tmp,
    backtrack_level = assigned[lit2pvar(tmp)].level;
  }
  
  for (int i = 0; i < bump.size(); i++)
  {
    // double coeff = 0.5;
    // if (assigned[bump[i]].level >= backtrack_level - 1) coeff += 1.0;
    if (assigned[bump[i]].level >= backtrack_level - 1) {
      bump_var(bump[i], 1);
    }

    if(branchMode == BranchMode::VSIDS && OPT(enable_xvsids)) {
      for(int nb : VAR(bump[i]).neighbors) {
        bump_var(nb, 0.5);
      }
    }
  }

  if(branchMode == BranchMode::VMTF) bump_vmtf();

  // cal lbd
  time_stamp++;
  lbd = 0;
  // cipher_cnt = 0;
  for (int i = 0; i < (int)learn.size(); i++)
  { // Calculate the LBD.
    int l = assigned[lit2pvar(learn[i])].level;
    if (l && mark[l] != time_stamp)
      mark[l] = time_stamp, ++lbd;
  }

  // resart info
  if (lbd_queue_size < 50)
    lbd_queue_size++; // update fast-slow.
  else
    fast_lbd_sum -= lbd_queue[lbd_queue_pos];
  fast_lbd_sum += lbd, lbd_queue[lbd_queue_pos++] = lbd;
  if (lbd_queue_pos == 50)
    lbd_queue_pos = 0;
  slow_lbd_sum += (lbd > 50 ? 50 : lbd);

  return backtrack_level;
}