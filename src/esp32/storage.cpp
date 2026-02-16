
#include <string.h>

#include "esp_idf_version.h"
#include "nvs.h"

#include "panglos/debug.h"
#include "panglos/esp32/hal.h"

#include "panglos/storage.h"

namespace panglos {

static bool error(esp_err_t err, const char *fn, int line, const char *ns=0, const char *extra=0)
{
    PO_ERROR("err=%s err=%d fn=%s +%d %s.%s", 
            //lut(err_lut, err), 
            esp_err_to_name(err),
            err, fn, line, 
            ns ? ns : "",
            extra ? extra : "");
    return false;
}

static nvs_handle_t *make_handle(Storage::Handle **handle)
{
    return (nvs_handle_t*) handle;
}

static Storage::Type totype(nvs_type_t ty)
{
    switch (ty) 
    {
        case NVS_TYPE_BLOB : return Storage::VAL_BLOB;
        case NVS_TYPE_STR  : return Storage::VAL_STR;
        case NVS_TYPE_U8   : 
        case NVS_TYPE_I8   : return Storage::VAL_INT8;
        case NVS_TYPE_U16  :
        case NVS_TYPE_I16  : return Storage::VAL_INT16;
        case NVS_TYPE_U32  :
        case NVS_TYPE_I32  : return Storage::VAL_INT32;
        default            : return Storage::VAL_OTHER;
    }
}

    /*
     *
     */

Storage::Storage(const char *_ns, bool _verbose)
:   ns(_ns),
    verbose(_verbose),
    handle(0)
{
    ASSERT(sizeof(Storage::Handle *) == sizeof(nvs_handle_t));

    if (verbose) PO_DEBUG("ns=%s", ns);
    esp_err_t err = nvs_open(ns, NVS_READWRITE, make_handle(& handle));

    if (err != ESP_OK)
    {
        error(err, __FUNCTION__, __LINE__, ns);
        return;
    }

    if (verbose) PO_DEBUG("handle=%p", handle);
}

Storage::~Storage()
{
    if (verbose) PO_DEBUG("");
    nvs_close(*make_handle(& handle));
}

    /*
     *
     */

bool Storage::commit()
{
    if (verbose) PO_DEBUG("");
    esp_err_t err = nvs_commit(*make_handle(& handle));
    if (err != ESP_OK)
    {
        return error(err, __FUNCTION__, __LINE__, ns);
    }
    return err == ESP_OK;
}

    /*
     *
     */

#if ESP_IDF_VERSION > ESP_IDF_VERSION_VAL(5, 1, 0)
    // see https://github.com/espressif/esp-idf/issues/12155
#define NEW_nvs_find_key
#else
#endif

Storage::Type Storage::get_type(const char *key)
{
#if defined(NEW_nvs_find_key)

    nvs_type_t ty = NVS_TYPE_ANY;
    esp_err_t err = nvs_find_key(*make_handle(& handle), key, & ty);
    if (err != ESP_OK)
    {
        error(err, __FUNCTION__, __LINE__, ns, key);
        return VAL_NONE;
    }
    return totype(ty);
#else
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
#endif
}

    /*
     *
     */

bool Storage::set(const char *key, const char *value)
{
    ASSERT(key);
    ASSERT(value);

    if (verbose) PO_DEBUG("key=%s value=%s", key, value);
    esp_err_t err = nvs_set_str(*make_handle(& handle), key, value);
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
    esp_err_t err = nvs_set_i32(*make_handle(& handle), key, value);
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
    esp_err_t err = nvs_get_i8(*make_handle(& handle), key, value);
    if (err != ESP_OK)
    {
        return error(err, __FUNCTION__, __LINE__, ns, key);
    }

    return true;
}

bool Storage::get(const char *key, int16_t *value)
{
    if (verbose) PO_DEBUG("%s", key);
    esp_err_t err = nvs_get_i16(*make_handle(& handle), key, value);
    if (err != ESP_OK)
    {
        return error(err, __FUNCTION__, __LINE__, ns, key);
    }

    return true;
}

bool Storage::get(const char *key, int32_t *value)
{
    if (verbose) PO_DEBUG("%s", key);
    esp_err_t err = nvs_get_i32(*make_handle(& handle), key, value);
    if (err != ESP_OK)
    {
        return error(err, __FUNCTION__, __LINE__, ns, key);
    }

    return true;
}

bool Storage::get(const char *key, char *value, size_t *s)
{
    if (verbose) PO_DEBUG("%s", key);
    esp_err_t err = nvs_get_str(*make_handle(& handle), key, value, s);
    if (err != ESP_OK)
    {
        PO_ERROR("k=%s", key);
        return error(err, __FUNCTION__, __LINE__, ns, key);
    }

    return true;
}

bool Storage::set_blob(const char *key, void *data, size_t size)
{
    esp_err_t err = nvs_set_blob(*make_handle(& handle), key, data, size);
    if (err != ESP_OK)
    {
        PO_ERROR("k=%s", key);
        return error(err, __FUNCTION__, __LINE__, ns, key);
    }
    return true;
}

bool Storage::get_blob(const char *key, void *data, size_t *size)
{
    esp_err_t err = nvs_get_blob(*make_handle(& handle), key, data, size);
    if (err != ESP_OK)
    {
        PO_ERROR("k=%s", key);
        return error(err, __FUNCTION__, __LINE__, ns, key);
    }
    return true;
}

bool Storage::erase(const char *key)
{
    esp_err_t err = nvs_erase_key(*make_handle(& handle), key);
    if (err != ESP_OK)
    {
        return error(err, __FUNCTION__, __LINE__, ns, key);
    }

    return true;
}

    /*
     *  Query Iterator
     */

class Storage::List::Iter
{
public:
    nvs_iterator_t iter;
};

Storage::List::List(const char *_ns)
:   ns(_ns),
    iter(0)
{
    iter = new Storage::List::Iter;

#if (ESP_IDF_VERSION_MAJOR == 4)
    iter->iter = nvs_entry_find("nvs", ns, NVS_TYPE_ANY);
#endif
#if (ESP_IDF_VERSION_MAJOR == 5)
    esp_err_t err = nvs_entry_find("nvs", ns, NVS_TYPE_ANY, & iter->iter);
    if (err != ESP_OK)
    {
        PO_ERROR("%s", ns);
    }
#endif
}

static void del_iter(Storage::List::Iter **piter)
{
    Storage::List::Iter *iter = *piter;
    if (!iter) return;
    nvs_release_iterator(iter->iter);
    delete iter;
    *piter = 0;
}

Storage::List::~List()
{
    del_iter(& iter);
}

bool Storage::List::get(char *_ns, char *key, Type *type)
{
    if (!iter)
    {
        return false;
    }
    if (!iter->iter)
    {
        return false;
    }

    nvs_entry_info_t info;
    nvs_entry_info(iter->iter, & info);

    ASSERT(key);
    strncpy(key, info.key, NVS_KEY_NAME_MAX_SIZE);
    if (_ns)
    {
        strncpy(_ns, info.namespace_name, NVS_KEY_NAME_MAX_SIZE);
    }
    if (type)
    {
        //PO_DEBUG("%s %s %d", info.namespace_name, info.key, info.type);
        *type = totype(info.type);
    }

    const bool ok = iter;
#if (ESP_IDF_VERSION_MAJOR == 4)
    iter->iter = nvs_entry_next(iter->iter);
#endif
#if (ESP_IDF_VERSION_MAJOR == 5)
    esp_err_t err = nvs_entry_next(& iter->iter);
    if (err != ESP_OK)
    {
        del_iter(& iter);
    }
#endif
    return ok;
}

}   //  namespace panglos

//  FIN
