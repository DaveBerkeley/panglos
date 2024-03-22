
    /*
     *  "I've been writing ring buffers wrong all these years"
     *  https://www.snellman.net/blog/archive/2016-12-13-ring-buffers
     *
     *  Idx must be an unsigned type
     */

namespace panglos {

template<typename T, typename Idx=uint32_t>
class RingBuffer
{
    T *data;
    volatile Idx rd;
    volatile Idx wr;
    const Idx _size;
    const Idx _mask;

    Idx mask(Idx idx) { return idx & _mask; };
public:
    RingBuffer(Idx n)
    :   data(0),
        rd(0),
        wr(0),
        _size(n),
        _mask(n-1)
    {
        // ASSERT n is >0 and a power of 2
        ASSERT(n && ((n & -n) == n));
        data = new T[_size];
    }
    ~RingBuffer()
    {
        delete[] data;
    }
    void push(T val) { data[mask(wr++)] = val; }
    T pop() { return data[mask(rd++)]; }
    bool empty() { return rd == wr; }
    bool full() { return size() == _size; }
    Idx size() { return wr - rd; }
};

}   //  panglos

//  FIN
