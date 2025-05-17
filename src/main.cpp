#include "solver/solver.h"
#include "conf/options.hpp"
int main(int argc, char **argv) {
    INIT_ARGS(argc, argv);//定义并将参数赋给变量
    __gloabal_options.print_change();

    CSat csat;

    printf("c parse: %s\n", OPT(instance).c_str());

    csat.parse_aig(OPT(instance).c_str());
    
    printf("c num_gates: %d\n", csat.num_vars);
  
    int res = csat.solve();

    if (res == 10) {
        printf("s SATISFIABLE\n");

        printf("v ");
        for(int i=1; i<=csat.num_vars; i++) {
            printf("%d ", csat.value[i*2] > 0 ? i : -i);
        }
        printf("0\n");
    }
    else if (res == 20) printf("s UNSATISFIABLE\n");

    printf("c num_conlicts: %d \n", csat.conflicts);
  //  printf("c num_conlicts: %d  res=%d\n", S->conflicts,res);
    return 0;
}
