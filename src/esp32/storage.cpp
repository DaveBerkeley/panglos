
#include <string.h>

#include "nvs.h"

#include "panglos/debug.h"
#include "panglos/esp32/hal.h"

#include "panglos/storage.h"

namespace panglos {

static bool error(esp_err_t err, const char *fn, int line, const char *ns=0, const char *extra=0)
{
    PO_ERROR("err=%s err=%d fn=%s +%d %s.%s", lut(err_lut, err), err, fn, line, 
            ns ? ns : "",
            extra ? extra : "");
    return false;
}

    /*
     *
     */

Storage::Storage(const char *_ns, bool _verbose)
:   ns(_ns),
    handle(0),
    verbose(_verbose)
{
    if (verbose) PO_DEBUG("ns=%s", ns);
    nvs_handle_t nvs_handle = 0;
    esp_err_t err = nvs_open(ns, NVS_READWRITE, & nvs_handle);

    if (err != ESP_OK)
    {
        error(err, __FUNCTION__, __LINE__, ns);
        return;
    }

    handle = nvs_handle;
    if (verbose) PO_DEBUG("handle=%#x", handle);
}

Storage::~Storage()
{
    if (verbose) PO_DEBUG("");
    nvs_close((nvs_handle_t) handle);
}

    /*
     *
     */

bool Storage::commit()
{
    if (verbose) PO_DEBUG("");
    esp_err_t err = nvs_commit((nvs_handle_t) handle);
    if (err != ESP_OK)
    {
        return error(err, __FUNCTION__, __LINE__, ns);
    }
    return err == ESP_OK;
}

    /*
     *
     */

Storage::Type Storage::get_type(const char *key)
{
    // There doesn't seem to be a direct call to determine the type
    // so iterate through the namespace looking for the key

    List db(ns);

    char k[NVS_KEY_NAME_MAX_SIZE];
    Type type = VAL_NONE;

    while (db.get(0, k, & type))
    {
        if (!strcmp(k, key))
        {
            return type;
        }
    }

    return VAL_NONE;
}

    /*
     *
     */

bool Storage::set(const char *key, const char *value)
{
    ASSERT(key);
    ASSERT(value);

    if (verbose) PO_DEBUG("key=%s value=%s", key, value);
    esp_err_t err = nvs_set_str((nvs_handle_t) handle, key, value);
    if (err != ESP_OK)
    {
        return error(err, __FUNCTION__, __LINE__, ns, key);
    }

    return true;
}

bool Storage::set(const char *key, int32_t value)
{
    ASSERT(key);
    ASSERT(value);

    if (verbose) PO_DEBUG("key=%s value=%#x", key, (int) value);
    esp_err_t err = nvs_set_i32((nvs_handle_t) handle, key, value);
    if (err != ESP_OK)
    {
        return error(err, __FUNCTION__, __LINE__, ns, key);
    }

    return true;
}

    /*
     *
     */

bool Storage::get(const char *key, int8_t *value)
{
    if (verbose) PO_DEBUG("%s", key);
    esp_err_t err = nvs_get_i8((nvs_handle_t) handle, key, value);
    if (err != ESP_OK)
    {
        return error(err, __FUNCTION__, __LINE__, ns, key);
    }

    return true;
}

bool Storage::get(const char *key, int16_t *value)
{
    if (verbose) PO_DEBUG("%s", key);
    esp_err_t err = nvs_get_i16((nvs_handle_t) handle, key, value);
    if (err != ESP_OK)
    {
        return error(err, __FUNCTION__, __LINE__, ns, key);
    }

    return true;
}

bool Storage::get(const char *key, int32_t *value)
{
    if (verbose) PO_DEBUG("%s", key);
    esp_err_t err = nvs_get_i32((nvs_handle_t) handle, key, value);
    if (err != ESP_OK)
    {
        return error(err, __FUNCTION__, __LINE__, ns, key);
    }

    return true;
}

bool Storage::get(const char *key, char *value, size_t *s)
{
    if (verbose) PO_DEBUG("%s", key);
    esp_err_t err = nvs_get_str((nvs_handle_t) handle, key, value, s);
    if (err != ESP_OK)
    {
        PO_ERROR("k=%s", key);
        return error(err, __FUNCTION__, __LINE__, ns, key);
    }

    return true;
}

bool Storage::erase(const char *key)
{
    esp_err_t err = nvs_erase_key((nvs_handle_t) handle, key);
    if (err != ESP_OK)
    {
        return error(err, __FUNCTION__, __LINE__, ns, key);
    }

    return true;
}

    /*
     *  Query Iterator
     */

Storage::List::List(const char *_ns)
:   ns(_ns),
    iter(0)
{
    esp_err_t err = nvs_entry_find("nvs", ns, NVS_TYPE_ANY, & iter);
    if (err != ESP_OK)
    {
        PO_ERROR("%s", ns);
    }
}

Storage::List::~List()
{
    if (iter)
    {
        nvs_release_iterator((nvs_iterator_t) iter);
    }
}

bool Storage::List::get(char *_ns, char *key, Type *type)
{
    if (!iter)
    {
        return false;
    }

    nvs_entry_info_t info;
    nvs_entry_info(iter, & info);

    ASSERT(key);
    strncpy(key, info.key, NVS_KEY_NAME_MAX_SIZE);
    if (_ns)
    {
        strncpy(_ns, info.namespace_name, NVS_KEY_NAME_MAX_SIZE);
    }
    if (type)
    {
        //PO_DEBUG("%s %s %d", info.namespace_name, info.key, info.type);
        switch (info.type)
        {
            case NVS_TYPE_STR : *type = VAL_STR;    break;
            case NVS_TYPE_U32 :
            case NVS_TYPE_I32 : *type = VAL_INT32;  break;
            case NVS_TYPE_U16 :
            case NVS_TYPE_I16 : *type = VAL_INT16;  break;
            case NVS_TYPE_U8  :
            case NVS_TYPE_I8  : *type = VAL_INT8;   break;
            default           : *type = VAL_OTHER;  break;
        }
    }

    const bool ok = iter;
    esp_err_t err = nvs_entry_next(& iter);
    if (err != ESP_OK)
    {
        return error(err, __FUNCTION__, __LINE__, ns, key);
    }
    return ok;
}

}   //  namespace panglos

//  FIN
