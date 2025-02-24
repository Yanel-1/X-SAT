#pragma once

#include <cassert>
#include <vector>

typedef unsigned CRef;
typedef unsigned VRef;

class Clause {

public:

int& operator [] (int index) {
    return data[index];
}

Clause(int size) : lbd(0), activity(0.0), is_deleted(0), id(-1), learn(0), size(size), vref(0) {}

bool find_lit_swap_first(int lit) {
    for(int i=0; i<size; i++) { 
        if(data[i] == lit) {
            std::swap(data[i], data[0]);
            return true; 
        }
    }
    return false;
}

// header
int lbd;
double activity;
int size;
int is_deleted;
int id;
int learn;
VRef vref;

// body
int data[];

};

class ClauseDB {

public:

ClauseDB(int cap = 1e9) {
    capacity = cap;
    end_index = 0;
    wasted = 0;
    memory = (int*)malloc(sizeof(int)*capacity);
}

Clause& operator [] (CRef ref) {
    return *reinterpret_cast<Clause*>(&memory[ref]);
}

void free(CRef ref) {
    Clause& cls = operator[](ref);
    cls.is_deleted = 1;
    wasted += sizeof(Clause) / 4 + cls.size;
}

CRef allocate(int size) {
    assert(sizeof(Clause) % 4 == 0);
    int need_space = sizeof(Clause) / 4 + size;
    if(end_index + need_space > capacity) {
        capacity *= 2;
        memory = (int*)realloc(memory, sizeof(int)*capacity);
        memset(memory + end_index, 0, sizeof(int) * (capacity - end_index));
    }
    CRef res = end_index;
    end_index += need_space;
    Clause* clause_ptr = new (&memory[res]) Clause(size);
    clause_ptr->id = res;
    clauses.push_back(res);
    return res;
}

CRef end() {
    return end_index;
}

uint32_t size() {
    return clauses.size();
}

std::vector<CRef> clauses;

int wasted;
CRef end_index;
int capacity;

int *memory;

};