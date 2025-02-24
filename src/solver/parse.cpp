#include <cstring>
#include <fstream>
#include <unordered_set>

#include "solver.h"
extern "C" {
    #include "libs/aiger.h"
}

using namespace std;

char *read_whitespace(char *p) {
    while ((*p >= 9 && *p <= 13) || *p == 32)
        ++p;
    return p;
}
char *read_until_new_line(char *p)
{
    while (*p != '\n')
    {
        if (*p == '\0')
            exit(0);
        ++p;
    }
    return ++p;
}

char *read_int(char *p, int *i)
{
    *i = 0;
    bool sym = true;
    while ((*p != '-') && (*p < '0' || *p > '9')) p++;
    if (*p == '-')
        sym = false, ++p;
    while (*p >= '0' && *p <= '9')
    {
        if (*p == '\0')
            return p;
        *i = *i * 10 + *p - '0';
        ++p;
    }
    if (!sym)
        *i = -(*i);
    return p;
}

bool is_xor_output(aiger* aig, aiger_and &gate, int &lit0, int &lit1, int &unused_var0, int &unused_var1) {

    #define INDEX(LIT) ((int)aiger_lit2var(LIT) - (int)aig->num_inputs - 1)

    // 两个输入必须都是 AND 门
    if(INDEX(gate.rhs0) < 0 || INDEX(gate.rhs1) < 0) return false;
    // 两个输入必须都是 NOT
    if(!aiger_sign(gate.rhs0) || !aiger_sign(gate.rhs1)) return false;
    
    aiger_and &g0 = aig->ands[INDEX(gate.rhs0)];
    aiger_and &g1 = aig->ands[INDEX(gate.rhs1)];

    #undef INDEX

    int g0r0 = std::min(g0.rhs0, g0.rhs1); int g0r1 = std::max(g0.rhs0, g0.rhs1);
    int g1r0 = std::min(g1.rhs0, g1.rhs1); int g1r1 = std::max(g1.rhs0, g1.rhs1);

    // 两个输入代表的门必须是同一个门
    if(aiger_lit2var(g0r0) != aiger_lit2var(g1r0) || aiger_lit2var(g0r1) != aiger_lit2var(g1r1)) return 0;

    int var0 = aiger_lit2var(g0r0);
    int var1 = aiger_lit2var(g0r1);

    int sign00 = aiger_sign(g0r0);
    int sign01 = aiger_sign(g0r1);
    int sign10 = aiger_sign(g1r0);
    int sign11 = aiger_sign(g1r1);

    // XOR  - ( 1100 | 0011 )
    if((sign00 && sign01 && !sign10 && !sign11) || (!sign00 && !sign01 && sign10 && sign11)) {
        // printf("XOR  - sign: %d %d %d %d\n", sign00, sign01, sign10, sign11);
        lit0 = aiger_lit2var(g0r0) * 2;
        lit1 = aiger_lit2var(g0r1) * 2;
        unused_var0 = var0;
        unused_var1 = var1;
        return true;
    }

    // XNOR - ( 0110 | 1001 ) convert to XOR by fliping one of inputs
    if((!sign00 && sign01 && sign10 && !sign11) || (sign00 && !sign01 && !sign10 && sign11)) {
        // printf("XNOR - sign: %d %d %d %d\n", sign00, sign01, sign10, sign11);
        lit0 = aiger_lit2var(g0r0) * 2;
        lit1 = aiger_lit2var(g0r1) * 2 + 1;
        unused_var0 = var0;
        unused_var1 = var1;
        return true;
    }

    // printf("AND  - sign: %d %d %d %d\n", sign00, sign01, sign10, sign11);

    return false;
}

void CSat::parse_aig(const char *filename) {

    aiger *aig = aiger_init();

    // read aiger
    FILE* file = fopen(filename, "r");
    aiger_read_from_file(aig, file);
    fclose(file);

    num_vars = aig->num_inputs + aig->num_ands;
    num_inputs = aig->num_inputs;
    num_outputs = aig->num_outputs;
    num_ands = 0;
    num_xors = 0;

    std::unordered_set<int> unused_vars;

    printf("c ---------------- AIG-reader -------------------\n");

    struct XAGate {
        enum { XOR, AND } type;
        int fanin_lit0;
        int fanin_lit1;
        std::unordered_set<int> fanouts;
        int deleted = 0;
        int is_output = 0;
    };

    XAGate *gates = new XAGate[num_vars + 1];

    // 加载进内部数据结构
    for(int i=0; i<aig->num_ands; i++) {
        aiger_and &gate = aig->ands[i];
        XAGate &xag = gates[gate.lhs / 2];
        int lit0, lit1, unused_var0, unused_var1;
        if(is_xor_output(aig, gate, lit0, lit1, unused_var0, unused_var1)) {
            xag.type = XAGate::XOR;
            xag.fanin_lit0 = lit0;
            xag.fanin_lit1 = lit1;
        } else {
            xag.type = XAGate::AND;
            xag.fanin_lit0 = gate.rhs0;
            xag.fanin_lit1 = gate.rhs1;
        }
        // 初始化 fanout
        gates[xag.fanin_lit0 / 2].fanouts.insert(gate.lhs / 2);
        gates[xag.fanin_lit1 / 2].fanouts.insert(gate.lhs / 2);
    }
    for(int i=0; i<num_outputs; i++) {
        int out_lit = aig->outputs[i].lit;
        gates[out_lit / 2].is_output = 1;
    }

    // 迭代删除无扇出的节点
    std::queue<int> q;
    for(int i=1; i<=num_vars; i++) {
        if(gates[i].fanouts.size() == 0 && !gates[i].is_output) {
            q.push(i);
        }
    }
    while(!q.empty()) {
        int var = q.front(); q.pop();
        gates[var].deleted = 1;
        // printf("delete: %d\n", var);
        if(var <= num_inputs) continue;
        XAGate &g0 = gates[gates[var].fanin_lit0 / 2];
        g0.fanouts.erase(var);
        if(g0.fanouts.size() == 0) q.push(gates[var].fanin_lit0 / 2);
        XAGate &g1 = gates[gates[var].fanin_lit1 / 2];
        g1.fanouts.erase(var);
        if(g1.fanouts.size() == 0) q.push(gates[var].fanin_lit1 / 2);
    }

    int num_deleted = 0;
    for(int i=1; i<=num_vars; i++) {
        if(gates[i].deleted) num_deleted++;
    }

    printf("c num_vars: %d\n", num_vars);
    printf("c num_deleted: %d\n", num_deleted);

    num_vars -= num_deleted;

    alloc_memory(num_vars);

    // 重排所有编号（这里有可能可以优化一下内存布局）
    std::unordered_map<int, int> id_map;
    int __id = 0;

    #define ID(var) (id_map.count(var) ? id_map[var] : (id_map[var] = ++__id))
    #define LIT(l) ( ID(aiger_lit2var(l)) * 2 + (l & 1) )

    // 优先排列输入
    for(int i=1; i<=num_inputs; i++) {
        if(gates[i].deleted) { continue; }
        Variable &var = VAR(ID(i));
        var.is_input = 1;
        var.is_output = 0;
        var.type = INPUT;
        var.out = 2 * i;
        inputs.push(var.out);
    }

    for(int i=0; i<aig->num_ands; i++) {
        aiger_and &gate = aig->ands[i];
        int id = gate.lhs / 2;

        if(gates[id].deleted) continue;

        int ovar = ID(aiger_lit2var(gate.lhs));

        Variable &var = VAR(ovar);
        var.is_input = 0;
        var.is_output = 0;
        var.out = ovar * 2;

        if(gates[id].type == XAGate::XOR) {
            num_xors++;
            var.type = XOR;
        } else {
            num_ands++;
            var.type = AND;
        }

        var.fanin_lits.push(LIT(gates[id].fanin_lit0));
        var.fanin_lits.push(LIT(gates[id].fanin_lit1));
    }

    for(int i=0; i<num_outputs; i++) {
        int out_lit = aig->outputs[i].lit;
        Variable &var = VAR(ID(aiger_lit2var(out_lit)));
        var.is_output = 1;
        outputs.push(LIT(out_lit));
    }

    num_inputs = inputs.size();

    printf("c num_vars: %d num_ands: %d num_xors: %d num_inputs: %d\n", num_vars, num_ands, num_xors, num_inputs);

    assert(num_vars == num_ands + num_xors + num_inputs);
    
    #undef LIT
    #undef ID

    delete []gates;

    // show_circuit();

    // exit(0);
    // exit(0);
}

void CSat::show_circuit() {
    printf("c ------------------- ANG -------------------\n");
    printf("inputs:");
    for (int i = 0; i < inputs.size(); i++) printf(" %d", inputs[i]);
    puts("");
    printf("outputs:");
    for (int i = 0; i < outputs.size(); i++) printf(" %d", outputs[i]);
    puts("");
    for (int i = num_inputs+1; i <= num_vars; i++) {
        Variable &var = VAR(i);
        if(var.is_delete) continue;
        // if(i != 231) continue;
        printf("( %3d,jf=%d ) %d", i, is_jreason(i) && is_jnode(i), var.out);
        printf(" = %s(%d,level=%d) (", gate_type[var.type].c_str(), value[var.out], assigned[i].level);
        for(int i=0; i<var.fanin_lits.size()-1; i++) {
            int fanin_lit = var.fanin_lits[i];
            printf(" %d(%d,level=%d),", fanin_lit, value[fanin_lit], assigned[fanin_lit/2].level);
        }
        printf(" %d(%d,level=%d) )\n", var.fanin_lits.back(), value[var.fanin_lits.back()], assigned[var.fanin_lits.back()/2].level);
    }
    printf("c -------------------  End  -------------------\n");
}