
#pragma once

#include "panglos/app/event.h"

namespace panglos {

bool net_cli_init(void *, Event *, Event::Queue *);
bool net_server_init(void *, Event *, Event::Queue *);

}   //  namespace panglos

//  FIN
