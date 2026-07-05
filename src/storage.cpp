
#include <stdlib.h>
#include <string.h>

#include "panglos/debug.h"
#include "panglos/storage.h"
#include "panglos/flash_storage.h"

using namespace panglos;

namespace panglos {

static LUT st_lut[] = { 
    {   "NONE",     StorageType::VAL_NONE,  },
    {   "INT8",     StorageType::VAL_INT8,  }, 
    {   "INT16",    StorageType::VAL_INT16, },
    {   "INT32",    StorageType::VAL_INT32, },
    {   "STR",      StorageType::VAL_STR,   },
    {   "BLOB",     StorageType::VAL_BLOB,  },
    {   "OTHER",    StorageType::VAL_OTHER, },
    { 0, 0 },
};

const char *storage_type(StorageType ty)
{
    return lut(st_lut, ty);
}

    /*
     *
     */

void Storage::get_params(const struct Storage::IntParam *params)
{
    for (const struct IntParam *p = params; p->name; p++)
    {
        int32_t v = 0;
        if (!get(p->name, & v)) continue;
        if (p->validate && !p->validate(v, p->name)) continue;

        ASSERT(p->value);
        *p->value = v;
        //PO_DEBUG("%s.%s=%d", get_ns(), p->name, *p->value);
    }
}

void Storage::show_params(const struct Storage::IntParam *params)
{
    for (const struct IntParam *p = params; p->name; p++)
    {
        ASSERT(p->value);
        PO_DEBUG("%s.%s=%d", get_ns(), p->name, (int) *p->value);
    }
}

    /*
     *
     */

bool Storage::validate_range(int32_t v, const char *name, int32_t lo, int32_t hi)
{
    if ((v >= lo) && (v <= hi)) return true;
    PO_WARNING("%s %d not in range %d .. %d", name, (int) v, (int) lo, (int) hi);
    return false;
}

bool Storage::validate_set(int32_t v, const char *name, const int32_t *set, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        if (v == set[i]) return true;
    }
    PO_WARNING("%s %d not in set[%d]", name, (int) v, (int) n);
    return false;
}

    /*
     *
     */

Storage::Storage(const char *_ns, bool _verbose, StorageInterface *_flash)
:   flash(_flash ? _flash : native_storage),
    ns(_ns),
    verbose(_verbose)
{
}

Storage::~Storage()
{
}

StorageType Storage::get_type(const char *key)
{
    return StorageType(flash->get_type(ns, key));
}

bool Storage::get(const char *key, int8_t *value)
{
    return flash->get(ns, key, value);
}

bool Storage::get(const char *key, int16_t *value)
{
    return flash->get(ns, key, value);
}

bool Storage::get(const char *key, int32_t *value)
{
    return flash->get(ns, key, value);
}

bool Storage::get(const char *key, char *value, size_t *n)
{
    return flash->get(ns, key, value, n);
}


bool Storage::set(const char *key, int8_t value)
{
    return flash->set(ns, key, value);
}

bool Storage::set(const char *key, int16_t value)
{
    return flash->set(ns, key, value);
}

bool Storage::set(const char *key, int32_t value)
{
    return flash->set(ns, key, value);
}

bool Storage::set(const char *key, const char *value)
{
    return flash->set(ns, key, value);
}

bool Storage::set_blob(const char *key, void *data, size_t size)
{
    return flash->set_blob(ns, key, data, size);
}

bool Storage::get_blob(const char *key, void *data, size_t *size)
{
    return flash->get_blob(ns, key, data, size);
}

bool Storage::erase(const char *key)
{
    return flash->erase(ns, key);
}

bool Storage::commit()
{
    return flash->commit();
}

    /*
     *  Storage iterator
     */

Storage::List::List(const char *_ns, StorageInterface *fs)
:   ns(_ns),
    iter(0)
{
    StorageInterface *flash = fs ? fs : native_storage;
    iter = (Storage::List::Iter*) flash->make_iter();
}
 
Storage::List::~List()
{
    delete (StorageInterface::Iter*) iter;
}
 
bool Storage::List::get(char *_ns, char *key, StorageType *type, size_t max_sz)
{
    while (true)
    {
        StorageInterface::Iter* i = (StorageInterface::Iter*) iter;
        if (!i->next(_ns, key, type, max_sz)) return false;
        if (ns && strcmp(_ns, ns)) continue;
        return true;
    }
}

}   //  panglos

//  FIN
