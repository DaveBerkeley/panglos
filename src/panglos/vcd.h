    /*
     *
     */

#if !defined(__PANGLOS_VCD__)
#define __PANGLOS_VCD__

namespace panglos {

class VcdWriter
{
    FILE *file;
    FILE *close_file;
    char id;
    int time;

public:
    class Trace;

private:
    panglos::List<Trace*> traces;

    Trace *find(const char *name);
    void print(Trace *t, bool state, bool inc);

public:

    VcdWriter(const char *path);
    ~VcdWriter();

    void add(const char *name, bool state, int width);
    void write_header();
    void set(const char *name, bool state);
};

}   //  namespace panglos

#endif  //  __PANGLOS_VCD__

//  FIN
