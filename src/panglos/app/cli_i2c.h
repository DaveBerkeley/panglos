
#if defined(PO_I2C)

#include "cli/src/cli.h"

namespace panglos {

class I2C;

void add_cli_i2c_cmd(CLI *cli, I2C* (*lookup_fn)(const char *name, void *), void *arg=0);

}

#endif  //  PO_I2C

//   FIN
