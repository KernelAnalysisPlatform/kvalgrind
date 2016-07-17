#ifndef __MEMCHECK_H
#define __MEMCHECK_H

// Necessary information to keep track of an alloc call on the stack.
struct alloc_info {
    target_ulong heap;
    target_ulong size;
    target_ulong retaddr;
};

struct free_info {
    target_ulong heap;
    target_ulong addr;
    target_ulong retaddr;
};

struct range_info {
    target_ulong heap, begin, end;
    std::set<target_ulong> valid_ptrs; // Addresses of valid ptrs to this range

    range_info(target_ulong heap_, target_ulong begin_, target_ulong end_) {
        heap = heap_; begin = begin_; end = end_;
    }
    range_info() {
        heap = 0; begin = 0; end = 0;
    }
};

struct merge_range_set {
    std::map<target_ulong, target_ulong> impl;

    void insert(target_ulong heap, target_ulong begin, target_ulong end) {
        auto it = impl.upper_bound(begin); // first range past begin
        if (it != impl.begin()) { // possible overlap on left. have to handle.
            it--;
            if (begin < it->second) {
                begin = std::min(begin, it->first);
                it = impl.erase(it);
            } else {
                it++;
            }
        }
        // Erase all subranges.
        while (it != impl.end() && end > it->second) {
            it = impl.erase(it);
        }
        // Handle right overlap. At this point should have end <= it->second
        if (it != impl.end()) {
            if (end >= it->first) { // merge when touching, even. performance.
                end = std::max(end, it->second);
                impl.erase(it);
            }
        }
        impl[begin] = end;
    }

    bool contains(target_ulong addr) {
        if (impl.empty()) return false;
        else {
            auto it = impl.upper_bound(addr);
            if (it != impl.begin()) it--; // now greatest <= addr
            return addr >= it->first && addr < it->second;
        }
    }

    void dump() {
        printf("{  ");
        for (auto it = impl.begin(); it != impl.end(); it++) {
            printf("[%lx, %lx) ", it->first, it->second);
        }
        printf(" }\n");
    }
};

// Set of ranges [begin, end).
// Should satisfy guarantee that all ranges are disjoint at all times.
struct range_set {
    std::map<target_ulong, range_info> impl; // map from range begin -> end

    bool insert(target_ulong heap, target_ulong begin, target_ulong end) {
        bool error = false;

        // Check left overlap. FIXME make sure this is correct.
        auto it = impl.upper_bound(begin);
        if (it != impl.begin()) {
            it--; // now points to greatest elt <= begin
            if (begin < it->second.end) {
                printf("error! we shouldn't be merging [ %lx, %lx ). assuming missed free of [ %lx, %lx ).\n", begin, end, it->first, it->second.end);
                error = true;
                impl.erase(it->first);
            }
        }

        // Check right overlap.
        it = impl.upper_bound(begin); // least elt > (begin, end);
        if (it != impl.end()) {
            if (end > it->first) {
                printf("error! we shouldn't be merging. assuming missed free.\n");
                error = true;
                impl.erase(it->first);
            }
        }

        range_info ri(heap, begin, end);
        impl[begin] = ri;

        return error;
    }

    bool contains(target_ulong addr) {
        if (impl.empty()) return false;
        else {
            auto it = impl.upper_bound(addr);
            if (it != impl.begin()) it--; // now greatest <= addr
            return addr >= it->first && addr < it->second.end;
        }
    }

    bool has_range(target_ulong begin) {
        return impl.count(begin) > 0;
    }

    void resize(target_ulong begin, target_ulong new_end) {
        if (impl.count(begin) > 0) {
            range_info &ri = impl[begin];
            ri.end = new_end;
        } else {
            printf("error! resizing nonexistent range @ %lx\n", begin);
        }
    }

    range_info& operator[](target_ulong addr) {
        if (impl.count(addr) > 0) return impl[addr];
        auto it = impl.upper_bound(addr);
        if (it != impl.begin()) {
            it--; // now points to greatest elt <= addr
            if (addr >= it->first && addr < it->second.end) {
                return it->second;
            }
        }
        printf("error! lookup on nonexistent addr %lx\n", addr);
        throw 0;
    }

    // We will only ever use this with alloc_now, which should never have an
    // overlapping range inserted. So we can implement this the easy way.
    void remove(target_ulong begin) {
        if (impl.count(begin) == 0) {
            printf("error! %lx not found!\n", begin);
            dump();

            auto it = impl.upper_bound(begin);
            if (it != impl.begin()) {
                it--;
                if (false) {// begin < it->second) {
                    printf("freeing containing block.\n");
                    impl.erase(it->first);
                }
            }
        } else {
            impl.erase(begin);
        }
    }

    void dump() {
        printf("{  ");
        for (auto it = impl.begin(); it != impl.end(); it++) {
            printf("%lx:[%lx, %lx) ", it->second.heap, it->first, it->second.end);
        }
        printf(" }\n");
    }
};

struct read_info {
    target_ulong pc, loc, val;
    read_info(target_ulong pc_, target_ulong loc_, target_ulong val_) {
        pc = pc_; loc = loc_; val = val_;
    }
};

#endif
