
#include "panglos/debug.h"

#include "panglos/esp32/hal.h"

    /*
     *
     */

const Code Severity_lut[] = {
    // Logging Severity
    {   "NONE", S_NONE, },
    {   "CRITICAL", S_CRITICAL, },
    {   "ERROR", S_ERROR, },
    {   "WARNING", S_WARNING, },
    {   "NOTICE", S_NOTICE, },
    {   "INFO", S_INFO, },
    {   "DEBUG", S_DEBUG, },
    { 0, 0 },
};

    /*
     *
     */

namespace panglos {

void esp_check(esp_err_t err, int line)
{
    ASSERT_ERROR(err == ESP_OK, "err=%s line=%d", err_lookup(err_lut, err), line);
}

    /*
     *
     */

const Code err_lut[] = {
    // ESP Error Codes
    {   "ESP_OK", ESP_OK, },
    {   "ESP_FAIL", ESP_FAIL, },
    {   "ESP_ERR_NO_MEM", ESP_ERR_NO_MEM, },
    {   "ESP_ERR_INVALID_ARG", ESP_ERR_INVALID_ARG, },
    {   "ESP_ERR_INVALID_STATE", ESP_ERR_INVALID_STATE, },
    {   "ESP_ERR_INVALID_SIZE", ESP_ERR_INVALID_SIZE, },
    {   "ESP_ERR_NOT_FOUND", ESP_ERR_NOT_FOUND, },
    {   "ESP_ERR_NOT_SUPPORTED", ESP_ERR_NOT_SUPPORTED, },
    {   "ESP_ERR_TIMEOUT", ESP_ERR_TIMEOUT, },
    {   "ESP_ERR_INVALID_RESPONSE", ESP_ERR_INVALID_RESPONSE, },
    {   "ESP_ERR_INVALID_CRC", ESP_ERR_INVALID_CRC, },
    {   "ESP_ERR_INVALID_VERSION", ESP_ERR_INVALID_VERSION, },
    {   "ESP_ERR_INVALID_MAC", ESP_ERR_INVALID_MAC, },
    {   "ESP_ERR_WIFI_BASE", ESP_ERR_WIFI_BASE, },
    {   "ESP_ERR_MESH_BASE", ESP_ERR_MESH_BASE, },
    {   "ESP_ERR_FLASH_BASE", ESP_ERR_FLASH_BASE, },
    {   "ESP_ERR_HW_CRYPTO_BASE", ESP_ERR_HW_CRYPTO_BASE, },
    { 0, 0 },
};

}   //  namespace panglos

//  FIN
