
#include "esp_err.h"

namespace panglos {

void esp_check(esp_err_t err, int line);

extern const LUT err_lut[];

}   

//  FIN
