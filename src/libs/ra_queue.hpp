#include <iostream>
#include <unordered_map>
#include <list>
#include <vector>
#include <stdexcept>
#include <cstdlib>

template<typename T>
class RaQueue {
public:

std::list<T> elements; 
std::unordered_map<T, typename std::list<T>::iterator> index;
std::unordered_map<T, int> stamp_map;
int *value;

typename std::list<T>::iterator start_it;
int start_stamp;

int time_stamp = 0;

    void insert(const T& v) {
        if (index.find(v) == index.end()) {
            stamp_map[v] = ++time_stamp;
            elements.push_front(v);
            index[v] = elements.begin();
            if(value[v*2] == 0) {
                start_it = elements.begin();
                start_stamp = stamp_map[v];
            }
        }
    }

    void remove(const T& v) {
        auto it = index.find(v);
        if (it != index.end()) {
            elements.erase(it->second);
            index.erase(it);
        }
    }

    size_t size() const {
        return elements.size();
    }

    bool isEmpty() const {
        return elements.empty();
    }

    bool contains(const T& v) const {
        return index.find(v) != index.end();
    }

    void print() const {
        for (const auto& element : elements) {
            std::cout << element << " ";
        }
        std::cout << std::endl;
    }

    void clear() {
        elements.clear();
        index.clear();
    }
};