
#pragma once

namespace panglos {

class RTC;

void add_cli_rtc_cmd(CLI *cli, RTC* (*fn)(const char *name, void *), void *arg);

}

//  FIN
