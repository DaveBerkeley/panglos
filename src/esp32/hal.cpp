
#if defined(ESP32)

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "panglos/debug.h"
#include "panglos/thread.h"

#include "panglos/esp32/hal.h"

    /*
     *
     */

const LUT Severity_lut[] = {
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

void Error_Handler(void)
{
    printf("ERROR ERROR ERROR\n\r");
    abort();
}

uint32_t get_time(void)
{
    TickType_t ticks = xTaskGetTickCount();
    return ticks;
}

const char *get_task(void)
{
    panglos::Thread *thread = panglos::Thread::get_current();
    return thread ? thread->get_name() : pcTaskGetName(0);
}

    /*
     *
     */

namespace panglos {

void esp_check(esp_err_t err, int line)
{
    ASSERT_ERROR(err == ESP_OK, "err=%s line=%d", lut(err_lut, err), line);
}

    /*
     *
     */

const LUT err_lut[] = {
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

#endif  //  ESP32

//  FIN
