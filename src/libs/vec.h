#ifndef _vec_h_INCLUDED
#define _vec_h_INCLUDED

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <unordered_set>

// Forward declaration for iterator
template<class T>
class vec;

template<class T>
class vec_iterator {
public:
    vec_iterator(T* ptr) : ptr(ptr) {}

    T& operator*() const { return *ptr; }
    T* operator->() const { return ptr; }

    vec_iterator& operator++() { ++ptr; return *this; }
    vec_iterator operator++(int) { vec_iterator temp = *this; ++ptr; return temp; }

    bool operator==(const vec_iterator& other) const { return ptr == other.ptr; }
    bool operator!=(const vec_iterator& other) const { return ptr != other.ptr; }

private:
    T* ptr;
};

template<class T>
class vec {
     static inline int  imax   (int x, int y) { int mask = (y-x) >> (sizeof(int)*8-1); return (x&mask) + (y&(~mask)); }
    static inline void nextCap(int &cap) { cap += ((cap >> 1) + 2) & ~1; }

    public:
        T* data;
        int sz, cap;
        vec()                   :  data(NULL), sz(0), cap(0) {}
        vec(const vec<T>& other);
        vec(int size, const T& pad) : data(NULL) , sz(0)   , cap(0)    { growTo(size, pad); }
        explicit vec(int size)  :  data(NULL), sz(0), cap(0) { growTo(size); }
        ~vec()                                               { clear(true); }

        operator T*      (void)         { return data; }
        
        int     size     (void) const   { return sz;   }
        int     capacity (void) const   { return cap;  }
        void    capacity (int min_cap);

        void    setsize  (int v)        { sz = v;} 
        void    push  (void)            { if (sz == cap) capacity(sz + 1); new (&data[sz]) T(); sz++; }
        void    push   (const T& elem)  { if (sz == cap) capInc(sz + 1); data[sz++] = elem; }
        void    push_  (const T& elem)  { assert(sz < cap); data[sz++] = elem; }
        void    pop    (void)           { assert(sz > 0), sz--, data[sz].~T(); } 
        void    copyTo (vec<T>& copy)   { copy.clear(); copy.growTo(sz); for (int i = 0; i < sz; i++) copy[i] = data[i]; }
        void    shrink_  (int nelems)     { assert(nelems <= sz); sz -= nelems; }
        
        void    growTo   (int size);
        void    growTo   (int size, const T& pad);
        void    clear    (bool dealloc = false);
        void    capInc   (int to_cap);

        void    unique();
        bool    erase(const T& value);

        T&       operator [] (int index)       { return data[index]; }
        const T& operator [] (int index) const { return data[index]; }

        T&       back        (void)            { return data[sz - 1]; }
        const T& back        (void)      const { return data[sz - 1]; }

        // iterator
        // Iterator methods
        vec_iterator<T> begin() { return vec_iterator<T>(data); }
        vec_iterator<T> end() { return vec_iterator<T>(data + sz); }
};


class OutOfMemoryException{};

template<class T>
vec<T>::vec(const vec<T>& other) : data(NULL), sz(other.sz), cap(other.cap) {
    if (cap > 0) {
        data = (T*)malloc(cap * sizeof(T));
        if (data == NULL) throw OutOfMemoryException();
        for (int i = 0; i < sz; i++) new (&data[i]) T(other.data[i]);
    }
}

template<class T>
void vec<T>::clear(bool dealloc) {
    if (data != NULL) {
        sz = 0;
        if (dealloc) free(data), data = NULL, cap = 0;
    }
}

template<class T>
void vec<T>::capInc(int to_cap) {
    if (cap >= to_cap) return;
    int add = imax((to_cap - cap + 1) & ~1, ((cap >> 1) + 2) & ~1); 
    if (add > __INT_MAX__ - cap || ((data = (T*)::realloc(data, (cap += add) * sizeof(T))) == NULL) && errno == ENOMEM)
        throw OutOfMemoryException();
}

template<class T>
void vec<T>::capacity(int min_cap) {
    if (cap >= min_cap) return;
    int add = imax((min_cap - cap + 1) & ~1, ((cap >> 1) + 2) & ~1);   // NOTE: grow by approximately 3/2
    if (add > __INT_MAX__ - cap || ((data = (T*)::realloc(data, (cap += add) * sizeof(T))) == NULL) && errno == ENOMEM)
        throw OutOfMemoryException();
 }

template<class T>
void vec<T>::growTo(int size) {
    if (sz >= size) return;
    capInc(size);
    for (int i = 0; i < sz; i++) new (&data[i]) T();
    sz = size;
}

template<class T>
void vec<T>::growTo(int size, const T& pad) {
    if (sz >= size) return;
    capacity(size);
    for (int i = sz; i < size; i++) data[i] = pad;
    sz = size; }

template<class T>
void vec<T>::unique() {
    if (sz <= 1) return;  // No duplicates possible in a vec with 0 or 1 element
    std::unordered_set<T> seen;
    int new_sz = 0;
    for (int i = 0; i < sz; ++i) {
        if (seen.find(data[i]) == seen.end()) {
            seen.insert(data[i]);
            data[new_sz++] = data[i];
        }
    }
    sz = new_sz;
}


template<class T>
bool vec<T>::erase(const T& value) {
    for (int i = 0; i < sz; ++i) {
        if (data[i] == value) {
            for (int j = i; j < sz - 1; ++j) {
                data[j] = data[j + 1];
            }
            --sz;
            data[sz].~T();
            return true;
        }
    }
    return false;
}


#endif