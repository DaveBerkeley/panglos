
#if !defined(__PANGLOS_OBJECT__)
#define __PANGLOS_OBJECT__

namespace panglos {

class Objects
{
public:
    virtual ~Objects() {}

    virtual void add(const char *name, void *obj) = 0;
    virtual void *get(const char *name) = 0;
    virtual bool remove(const char *name) = 0;

    static Objects *create(bool verbose=false);
    static Objects* objects; // global store for app

    typedef void (*visitor)(const char *name, void *obj, void *arg);
    static void visit(Objects* objects, visitor fn, void *arg);
};

}   //  namespace panglos

#endif  //  __PANGLOS_OBJECT__

//  FIN
