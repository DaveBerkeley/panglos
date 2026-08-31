
#pragma once

class CLI;

namespace panglos {

class Display;

void add_cli_display_cmd(CLI *cli, Display* (*fn)(const char *name, void *), void *arg=0);

}

//  FIN
