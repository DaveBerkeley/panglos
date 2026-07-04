
#include <string.h>

#include "esp_idf_version.h"
#include "nvs.h"

#include "panglos/debug.h"
#include "panglos/esp32/hal.h"

#include "panglos/storage.h"

namespace panglos {

static bool error(esp_err_t err, const char *fn, int line, const char *ns=0, const char *extra=0)
{
    PO_INFO("'%s.%s' err=%s fn=%s() line=%d", 
            ns ? ns : "", extra ? extra : "",
            esp_err_to_name(err),
            fn, line
    );
    return false;
}

static nvs_handle_t *make_handle(Storage::Handle **handle)
{
    return (nvs_handle_t*) handle;
}

static StorageType totype(nvs_type_t ty)
{
    switch (ty) 
    {
        case NVS_TYPE_BLOB : return StorageType::VAL_BLOB;
        case NVS_TYPE_STR  : return StorageType::VAL_STR;
        case NVS_TYPE_U8   : 
        case NVS_TYPE_I8   : return StorageType::VAL_INT8;
        case NVS_TYPE_U16  :
        case NVS_TYPE_I16  : return StorageType::VAL_INT16;
        case NVS_TYPE_U32  :
        case NVS_TYPE_I32  : return StorageType::VAL_INT32;
        default            : return StorageType::VAL_OTHER;
    }
}

#if ESP_IDF_VERSION > ESP_IDF_VERSION_VAL(5, 1, 0)
    // see https://github.com/espressif/esp-idf/issues/12155
#define NEW_nvs_find_key
#else
#endif

    /*
     *
     */

class NvsStorage : public StorageInterface
{
    nvs_handle_t handle;
    char name[NVS_KEY_NAME_MAX_SIZE];
    bool verbose;
public:
    NvsStorage()
    :   handle(0),
        verbose(false)
    {
    }

    bool open(const char *ns)
    {
        ASSERT(ns);

        const bool same = !strcmp(ns, name);

        if (handle)
        {
            if (same) return true; // already open
            close();
        }

        strncpy(name, ns, sizeof(name));

        esp_err_t err = nvs_open(ns, NVS_READWRITE, & handle);

        if (err != ESP_OK)
        {
            error(err, __FUNCTION__, __LINE__, "ctor");
            return false;
        }

        if (verbose) PO_DEBUG("handle=%p", (void*) handle);
        return true;
    }

    void close()
    {
        if (verbose) PO_DEBUG("");
        if (handle) nvs_close(handle);
        handle = 0;
    }
 
    virtual ~NvsStorage()
    {
        close();
    }

    virtual StorageType get_type(const char *ns, const char *key) override
    {
#if defined(NEW_nvs_find_key)
        if (!open(ns)) return VAL_NONE;

        nvs_type_t ty = NVS_TYPE_ANY;
        esp_err_t err = nvs_find_key(handle, key, & ty);
        if (err != ESP_OK)
        {
            error(err, __FUNCTION__, __LINE__, ns, key);
            return VAL_NONE;
        }
        return totype(ty);
#else
        ASSERT(0);
        return VAL_NONE;
#endif
    }

    virtual bool get(const char *ns, const char *key, int8_t *value) override
    {
        if (!open(ns)) return false;
        if (verbose) PO_DEBUG("%s", key);
        esp_err_t err = nvs_get_i8(handle, key, value);
        if (err != ESP_OK)
        {
            return error(err, __FUNCTION__, __LINE__, ns, key);
        }

        return true;
    }
    
    virtual bool get(const char *ns, const char *key, int16_t *value) override
    {
        if (!open(ns)) return false;
        if (verbose) PO_DEBUG("%s", key);
        esp_err_t err = nvs_get_i16(handle, key, value);
        if (err != ESP_OK)
        {
            return error(err, __FUNCTION__, __LINE__, ns, key);
        }

        return true;
    }
    
    virtual bool get(const char *ns, const char *key, int32_t *value) override
    {
        if (!open(ns)) return false;
        if (verbose) PO_DEBUG("%s", key);
        esp_err_t err = nvs_get_i32(handle, key, value);
        if (err != ESP_OK)
        {
            return error(err, __FUNCTION__, __LINE__, ns, key);
        }

        return true;
    }
    
    virtual bool get(const char *ns, const char *key, char *value, size_t *s) override
    {
        if (!open(ns)) return false;
        if (verbose) PO_DEBUG("%s", key);
        esp_err_t err = nvs_get_str(handle, key, value, s);
        if (err != ESP_OK)
        {
            //PO_INFO("k=%s", key);
            return error(err, __FUNCTION__, __LINE__, ns, key);
        }

        return true;
    }
    

    virtual bool set(const char *ns, const char *key, int8_t value) override
    {
        if (!open(ns)) return false;
        if (verbose) PO_DEBUG("%s", key);
        esp_err_t err = nvs_set_i8(handle, key, value);
        if (err != ESP_OK)
        {
            return error(err, __FUNCTION__, __LINE__, ns, key);
        }

        return true;
    }
    
    virtual bool set(const char *ns, const char *key, int16_t value) override
    {
        if (!open(ns)) return false;
        if (verbose) PO_DEBUG("%s", key);
        esp_err_t err = nvs_set_i16(handle, key, value);
        if (err != ESP_OK)
        {
            return error(err, __FUNCTION__, __LINE__, ns, key);
        }

        return true;
    }
    
    virtual bool set(const char *ns, const char *key, int32_t value) override
    {
        if (!open(ns)) return false;
        if (verbose) PO_DEBUG("%s", key);
        esp_err_t err = nvs_set_i32(handle, key, value);
        if (err != ESP_OK)
        {
            return error(err, __FUNCTION__, __LINE__, ns, key);
        }

        return true;
    }
 
    virtual bool set(const char *ns, const char *key, const char *value) override
    {
        ASSERT(key);
        ASSERT(value);

        if (!open(ns)) return false;
        if (verbose) PO_DEBUG("key=%s value=%s", key, value);
        esp_err_t err = nvs_set_str(handle, key, value);
        if (err != ESP_OK)
        {
            return error(err, __FUNCTION__, __LINE__, ns, key);
        }

        return true;
    }
 
    virtual bool set_blob(const char *ns, const char *key, void *data, size_t size) override
    {
        if (!open(ns)) return false;
        esp_err_t err = nvs_set_blob(handle, key, data, size);
        if (err != ESP_OK)
        {
            //PO_ERROR("k=%s", key);
            return error(err, __FUNCTION__, __LINE__, ns, key);
        }
        return true;
    }
  
    virtual bool get_blob(const char *ns, const char *key, void *data, size_t *size) override
    {
        if (!open(ns)) return false;
        esp_err_t err = nvs_get_blob(handle, key, data, size);
        if (err != ESP_OK)
        {
            //PO_ERROR("k=%s", key);
            return error(err, __FUNCTION__, __LINE__, ns, key);
        }
        return true;
    }
    

    virtual bool erase(const char *ns, const char *key) override
    {
        if (!open(ns)) return false;

        esp_err_t err = nvs_erase_key(handle, key);
        if (err != ESP_OK)
        {
            return error(err, __FUNCTION__, __LINE__, ns, key);
        }

        return true;
    }

    virtual bool commit() override
    {
        if (!handle) return false;

        if (verbose) PO_DEBUG("");
        esp_err_t err = nvs_commit(handle);
        if (err != ESP_OK)
        {
            return error(err, __FUNCTION__, __LINE__, "commit");
        }
        return err == ESP_OK;
    }

    class Iter : public StorageInterface::Iter
    {
        nvs_iterator_t iter;
    public: 
        Iter()
        :   iter(0)
        {
            esp_err_t err = nvs_entry_find("nvs", 0, NVS_TYPE_ANY, & iter);
            if (err != ESP_OK)
            {
                PO_ERROR("");
            }
        }

        virtual ~Iter()
        {
            del_iter();
        }

        void del_iter()
        {
            if (!iter) return;
            nvs_release_iterator(iter);
            iter = 0;
        }

        virtual bool next(char *_ns, char *key, StorageType *type, size_t max_sz)
        {
            if (max_sz > NVS_KEY_NAME_MAX_SIZE) max_sz = NVS_KEY_NAME_MAX_SIZE;
            if (!iter)
            {
                return false;
            }

            nvs_entry_info_t info;
            nvs_entry_info(iter, & info);

            ASSERT(key);
            strncpy(key, info.key, max_sz);
            if (_ns)
            {
                strncpy(_ns, info.namespace_name, max_sz);
            }
            if (type)
            {
                //PO_DEBUG("%s %s %d", info.namespace_name, info.key, info.type);
                *type = totype(info.type);
            }

            const bool ok = iter;
            esp_err_t err = nvs_entry_next(& iter);
            if (err != ESP_OK)
            {
                del_iter();
            }
            return ok;
        }
    };

    virtual StorageInterface::Iter *make_iter() override
    {
        return new Iter;
    }
};

static NvsStorage native;
StorageInterface *Storage::native_storage = & native;

}   //  namespace panglos

//  FIN
