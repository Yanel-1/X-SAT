#include "vec.h"
#include <fstream>
#define left(x) (x << 1 | 1)
#define right(x) ((x + 1) << 1)
#define father(x) ((x - 1) >> 1)

template<class Comp>
class Heap {
public:
    Comp lt;
    vec<int> heap;
    vec<int> pos;
    
    void up(int v) {
        int x = heap[v], p = father(v);
        while (v && lt(x, heap[p])) {
            heap[v] = heap[p], pos[heap[p]] = v;
            v = p, p = father(p);
        }
        heap[v] = x, pos[x] = v;
    }

    void down(int v) {
        int x = heap[v];
        while (left(v) < (int)heap.size()){
            int child = right(v) < (int)heap.size() && lt(heap[right(v)], heap[left(v)]) ? right(v) : left(v);
            if (!lt(heap[child], x)) break;
            heap[v] = heap[child], pos[heap[v]] = v, v = child;
        }
        heap[v] = x, pos[x] = v;
    }

    void setComp   (Comp c)              { lt = c; }
    bool empty     ()              const { return heap.size() == 0; }
    bool inHeap    (int n)         const { return n < (int)pos.size() && pos[n] >= 0; }
    void update    (int x)               { up(pos[x]); }
    int  size      ()                    { return heap.size(); }
    int  top       ()                    { return heap.size() ? heap[0] : -1; };

    void insert(int x) {
        if ((int)pos.size() < x + 1) 
            pos.growTo(x + 1, -1);
        pos[x] = heap.size();
        heap.push(x);
        up(pos[x]); 
    }

    int pop() {
        int x = heap[0];
        heap[0] = heap.back();
        pos[heap[0]] = 0, pos[x] = -1;
        heap.pop();
        if (heap.size() > 1) down(0);
        return x; 
    }

    void erase(int g) {
        assert(pos[g] != -1);
        int p = pos[g], o = heap.back();
        if (g == o) {
            pos[g] = -1;
            heap.pop();
            return;
        }
        heap[p] = o;
        pos[o] = p, pos[g] = -1;
        heap.pop();
        if (heap.size() > 1) up(pos[o]), down(pos[o]);
    } 
};