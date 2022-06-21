    /*
     *
     */

#if !defined(__PANGLOS_VCD__)
#define __PANGLOS_VCD__

#include <string>

namespace panglos {

class VcdWriter
{
    FILE *file;
    FILE *close_file;
    char id;
    int time;
    std::string path;

public:
    class Trace;

private:
    panglos::List<Trace*> traces;

    Trace *find(const char *name);
    void print(Trace *t, bool state);

public:

    VcdWriter(const char *path);
    ~VcdWriter();
    void close();

    void add(const char *name, bool state, int width);
    void write_header();
    void set(const char *name, bool state);
    void tick();

    // spawn sigrok-cli to convert vcd into sr
    bool sigrok_write(const char *sr_path);
};

}   //  namespace panglos

#endif  //  __PANGLOS_VCD__

//  FIN
