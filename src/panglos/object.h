
namespace panglos {

class Objects
{
public:
    virtual ~Objects() {}

    virtual void add(const char *name, void *obj) = 0;
    virtual void *get(const char *name) = 0;
    virtual bool remove(const char *name) = 0;

    static Objects *create();
};

}   //  namespace panglos

//  FIN
