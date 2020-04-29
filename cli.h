
#include "sprintf.h"

    /*
     *
     */

namespace panglos {

typedef struct Action
{
    const char *cmd;
    void (*fn)(Action *action, int argc, int *argv);
    void *arg;
    struct Action *next;
}   Action;

class CLI
{
public:
    static const int MAX_VALUES = 10;
    static const int MAX_CMD = 6;
private:
    char command[MAX_CMD];
    bool num_valid;
    bool negative;
    int idx;
    Action *actions;
    Action *match;
    int values[MAX_VALUES];

    const char *delim;
    char prev;

public:
    enum Error { 
        ERR_NONE = 0, 
        ERR_UNKNOWN_CMD,
        ERR_BAD_CHAR,
        ERR_NUM_EXPECTED,
        ERR_TOO_LONG,
        ERR_BAD_SIGN,
    };
    typedef void (*ErrorFn)(Action *a, int cursor, Error);

private:
    ErrorFn err_fn;
    int cursor;

    void reset();
    void error(enum Error err);
    void run();
    void apply_sign();
    bool match_action(Action **a);

public:
    CLI(const char *delimit=",");

    void add_action(Action *action);
    void set_error_fn(ErrorFn fn);

    void process(char c);
    void process(const char *line);
};

}   //  namespace panglos

// FIN
