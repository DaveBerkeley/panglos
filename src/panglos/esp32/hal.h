
#include "esp_err.h"

namespace panglos {

void esp_check(esp_err_t err, int line);

extern const Code err_lut[];

}   

//  FIN
