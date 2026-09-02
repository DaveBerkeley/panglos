

#if defined(PO_SPI)

#include "cli/src/cli.h"

namespace panglos {

class GPIO;

void add_cli_spi_cmd(CLI *cli, GPIO* (*lookup_fn)(const char *name, void *), void *arg=0);

}

#endif  //  PO_SPI

//   FIN
