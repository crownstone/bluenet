#include "third/SortMedian.h"
#include <algorithm>
#include <limits>

struct Buffer {
public:
    std::vector<std::pair<power_elem_t, unsigned>> sorted;

    explicit Buffer(MedianFilter f) :
        sorted(f.k),
        filter {f}
    {}

    void init(const power_elem_t* p) {
        for (unsigned i {0}; i < filter.k; ++i) {
            sorted[i] = std::make_pair(p[i], i);
        }
        std::sort(sorted.begin(), sorted.end());
    }

private:
    const MedianFilter filter;
};


struct Part {
private:
    struct Link {
        unsigned prev;
        unsigned next;
    };

public:
    explicit Part(MedianFilter f) :
        filter {f},
        link(f.k + 1)
    {}

    void init(const power_elem_t* p, const Buffer& buf) {
        data = p;
        init_links(buf);
        med = buf.sorted[filter.half].second;
        small = filter.half;
    }

    void unwind() {
        for (unsigned j {0}; j < filter.k; ++j) {
            unsigned i {filter.k - 1 - j};
            Link l = link[i];
            link[l.prev].next = l.next;
            link[l.next].prev = l.prev;
        }
        med = tail();
        small = 0;
    }

    inline void del(unsigned i) {
        Link l = link[i];
        link[l.prev].next = l.next;
        link[l.next].prev = l.prev;
        if (below_med(i)) {
            --small;
        } else {
            if (i == med) {
                med = l.next;
            }
            if (small > 0) {
                med = link[med].prev;
                --small;
            }
        }
    }

    inline void add(unsigned i) {
        Link l = link[i];
        link[l.prev].next = i;
        link[l.next].prev = i;
        if (below_med(i)) {
            med = link[med].prev;
        }
    }

    inline void advance() {
        med = link[med].next;
        ++small;
    }

    inline power_elem_t peek() const {
        return med == tail() ? std::numeric_limits<power_elem_t>::max() : data[med];
    }

    inline bool at_end() const {
        return med == tail();
    }

    inline unsigned nsmall() const {
        return small;
    }

private:
    void init_links(const Buffer& buf) {
        unsigned a {tail()};
        for (unsigned i {0}; i < filter.k; ++i) {
            unsigned b {buf.sorted[i].second};
            link[a].next = b;
            link[b].prev = a;
            a = b;
        }
        link[a].next = tail();
        link[tail()].prev = a;
    }

    inline bool below_med(unsigned i) const {
        return med == tail() ||
            data[i] < data[med] ||
            (data[i] == data[med] && i < med);
    }

    inline unsigned tail() const {
        return filter.k;
    }

    // Element index or tail
    unsigned med;
    // Elements before med
    unsigned small;
    const power_elem_t* data;
    MedianFilter filter;
    std::vector<Link> link;
};


class SortMedian {
public:
    explicit SortMedian(MedianFilter f) :
        filter {f},
        buf {f},
        a {f},
        b {f}
    {}

    void run(const PowerVector& x, PowerVector& y) {
        init(x, 0);
        y[0] = b.peek();
        for (unsigned part {1}; part < filter.blocks; ++part) {
            std::swap(a, b);
            init(x, part);
            b.unwind();
            run_part(y, part);
        }
    }

private:
    void init(const PowerVector& x, unsigned part) {
        const power_elem_t* p = x.data() + filter.k * part;
        buf.init(p);
        b.init(p, buf);
    }

    void run_part(PowerVector& y, unsigned part) {
        for (unsigned i {0}; i < filter.k; ++i) {
            a.del(i);
            b.add(i);
//            assert(a.nsmall() + b.nsmall() <= filter.half);
            balance();
//            assert(a.nsmall() + b.nsmall() == filter.half);
            power_elem_t median {std::min(a.peek(), b.peek())};
            y[(part - 1) * filter.k + i + 1] = median;
        }
//        assert(a.nsmall() == 0);
//        assert(b.nsmall() == filter.half);
    }

    inline void balance() {
        if (a.nsmall() + b.nsmall() < filter.half) {
            if (a_next()) {
                a.advance();
            } else {
                b.advance();
            }
        }
    }

    inline bool a_next() const {
        if (b.at_end()) {
            return true;
        } else if (a.at_end()) {
            return false;
        } else {
            return a.peek() <= b.peek();
        }
    }

    const MedianFilter filter;
    Buffer buf;
    Part a;
    Part b;
};


void sort_median(MedianFilter f, const PowerVector& x, PowerVector& y) {
    SortMedian m(f);
    m.run(x, y);
}
