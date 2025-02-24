#include "solver.h"

int CSat::propagate()
{

  while (propagated < trail.size())
  {
    int lit_id = trail[propagated++];
    int var_id = lit2pvar(lit_id);
    int level = assigned[var_id].level;
    Variable &var = VAR(var_id);
    Literal &lit = LIT(lit_id);

    for(int target_lit : watches_bin[lit_id]) {
      if(value[target_lit] == 0) {
        assign(target_lit, level, std::make_pair(lit_id, -1));
      } else if(value[target_lit] == -1) {
        conflict.clear();
        conflict.push(notlit(lit_id));
        conflict.push(target_lit);
        return 0;
      }
    }

    int *tri_i, *tri_j, *tri_end;
    vec<int> &ws_tri = watches_tri[lit_id];
    
    for(tri_i = tri_j = &ws_tri[0], tri_end = tri_i + ws_tri.size(); tri_i != tri_end;) {

        CRef cref = *tri_i;
        Clause &clause = clauseDB[cref];
        assert(clause.size == 3);

        int not_lit = notlit(lit_id);
        if (clause[0] == not_lit)
          clause[0] = clause[1],
          clause[1] = not_lit;

        // printf("watched: %d %d %d clause: %d %d %d\n", tri_i - &ws_tri[0], not_lit, cref, clause[0], clause[1], clause[2]);
        
        assert(clause[1] == not_lit);

        // 如果存在 1 ，跳过
        if(value[clause[0]] == 1) {
          *tri_j++ = *tri_i++;
          continue;
        }

        *tri_i++;

        // 看看第三个赋值是不是 0，不是的话可以 watch
        if(value[clause[2]] != -1) {
          std::swap(clause[1], clause[2]);
          watches_tri[notlit(clause[1])].push(cref);
          continue;
        }

        *tri_j++ = cref;

        if(value[clause[0]] == 0) {
          assign(clause[0], assigned[lit2pvar(lit_id)].level,
                 std::make_pair(cref, -4));
        } else {
          while (tri_i < tri_end)
                    *tri_j++ = *tri_i++;
          ws_tri.shrink_(tri_i - tri_j);
          conflict.clear();
          for (int r = 0; r < clause.size; r++)
          {
            conflict.push(clause[r]);
          }
          return 0; 
        }
    }

    ws_tri.shrink_(tri_i - tri_j);
    
    Watcher        *i, *j, *end;
    vec<Watcher>&  ws  = watches[lit_id];
    
    for (i = j = (Watcher*)ws, end = i + ws.size();  i != end;)
    {

      watches_cnt++;
      int blocker=i->blocker;
      if(value[blocker] == 1) {
        *j++=*i++;
        continue;
      }

      CRef cref = i->cref;
      Clause &clause = clauseDB[cref];

      // clause: -x1 + ... + -xn + f;  watch: -f, x1;  ilit = -f / x1
      int not_lit = notlit(lit_id);

      if (clause[0] == not_lit)
        clause[0] = clause[1],
        clause[1] = not_lit;

      assert(clause[1] == not_lit);
        
      i++;

      int first = clause[0];
      Watcher w  = Watcher{cref, first};
      // 另一 watch 变量是 1 的话，肯定无法传播，跳过。
      if (first!=blocker && value[first] == 1)
      {
        *j++=w;      
        continue;
      }

      // 寻找第一个不是 0 的，（1 或者 未赋值）
      int r = 2, sz = clause.size;
      for (; r < sz && value[clause[r]] == -1; r++);
      
      // 如果找到了 （1 或者 未赋值的），就移动到 watch 里
      if (r < sz)
      {
        clause[1] = clause[r];
        clause[r] = not_lit;
        watches[notlit(clause[1])].push(w);
      }
      // 如果全部都是 0，说明可以单元传播了，另一个 watch 必须是 1
      else
      {
        *j++ = w;
        if (value[clause[0]] == -1)
        {
          // conflict
          while (i < end)
                    *j++ = *i++;
          ws.shrink_(i - j);
          conflict.clear();
          for (int r = 0; r < clause.size; r++)
          {
            conflict.push(clause[r]);
          }
          return 0;
        }
        else
          assign(clause[0], assigned[lit2pvar(lit_id)].level,
                 std::make_pair(cref, -4));
      }
    }
    ws.shrink_(i - j);
  }

  return 1;
}