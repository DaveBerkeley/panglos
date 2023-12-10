    /*
     *
     */

#pragma once

class CliCommand;

namespace panglos {

struct CliServer {
    const char *host;
    const char *port;
    CliCommand *commands;
};

void cli_server(struct CliServer *cs);

}   //  namespace panglos

//  FIN
