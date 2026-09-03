
#pragma once

namespace panglos {

class GPIO;

void add_cli_gpio_cmd(CLI *cli, GPIO* (*lookup_fn)(const char *name, void *), void *arg=0);

}   //  namespace panglos

//  FIN
