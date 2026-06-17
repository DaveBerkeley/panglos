
#include <stdlib.h>
#include <string.h>

#include "panglos/debug.h"
#include "panglos/mutex.h"
#include "panglos/list.h"
#include "panglos/storage.h"

    /*
     *
     */

namespace panglos {

class KVPair
{
public:
    struct Keys
    {
        const char *ns;
        const char *key;

        static int match(const struct Keys *k1, const struct Keys *k2)
        {
            int cmp = strcmp(k1->ns, k2->ns);
            if (cmp != 0) return 0;
            return !strcmp(k1->key, k2->key);
        }
    };

    KVPair *next;
    struct Keys keys;
    Storage::Type type;
    size_t size;

    union {
        int8_t v8;
        int16_t v16;
        int32_t v32;
        const char *s;
    };

    static KVPair **get_next(KVPair *item) { return & item->next; }

    static int match(KVPair *item, struct Keys *keys)
    {
        return Keys::match(& item->keys, keys);
    }

    static KVPair *create(const char *ns, const char *key)
    {
        KVPair *pair = new KVPair;
        memset(pair, 0, sizeof(*pair));
        pair->keys.ns = strdup(ns);
        pair->keys.key = strdup(key);
        return pair;
    }

    void free_data()
    {
        switch (type)
        {
            case Storage::VAL_STR : free((void*) s); s = 0; break;
            case Storage::VAL_INT32 :  return;
            case Storage::VAL_INT16 :  return;
            case Storage::VAL_INT8  :  return;
            default: ASSERT(0);
        } 
    }

    void free_pair()
    {
        free_data();
        free((void*) keys.ns);
        free((void*) keys.key);
    }
};

class Helper
{
    Mutex *mutex;
    typedef List<KVPair*> Pairs;

    Pairs pairs;

    KVPair *find(const char *ns, const char *key, Mutex *mutex)
    {
        if (!ns) return 0;
        if (!key) return 0;
        KVPair::Keys keys = {
            .ns = ns,
            .key = key,
        };
        return pairs.find((int (*)(KVPair*,void*)) KVPair::match, & keys, mutex);
    }

    KVPair *find(const char *ns, const char *key, Storage::Type type, Mutex *mutex)
    {
        KVPair *pair = find(ns, key, mutex);
        if (!pair) return 0;
        if (pair->type != type) return 0;
        return pair;
    }

    KVPair *find_set(const char *ns, const char *key)
    {
        KVPair *pair = find(ns, key, 0);
        if (!pair)
        {
            pair = KVPair::create(ns, key);
            ASSERT(pair);
            pairs.push(pair, 0);
        }
        else
        {
            pair->free_data();
        }
        return pair;
    }

public:
    Helper()
    :   mutex(0),
        pairs(KVPair::get_next)
    {
        mutex = Mutex::create();
    }

    ~Helper()
    {
        while (true)
        {
            KVPair *pair = pairs.pop(0);
            if (!pair) break;
            pair->free_pair();
            delete pair;
        }

        delete mutex;
    }

    bool set(const char *ns, const char *key, const char *value)
    {
        ASSERT(value);
        Lock lock(mutex);

        KVPair *pair = find_set(ns, key);
        pair->type = Storage::VAL_STR;
        pair->s = strdup(value);
        return true;
    }

    bool set(const char *ns, const char *key, int32_t value)
    {
        Lock lock(mutex);

        KVPair *pair = find_set(ns, key);
        pair->type = Storage::VAL_INT32;
        pair->v32 = value;
        return true;
    }

    bool set(const char *ns, const char *key, int16_t value)
    {
        Lock lock(mutex);

        KVPair *pair = find_set(ns, key);
        pair->type = Storage::VAL_INT16;
        pair->v16 = value;
        return true;
    }

    bool set(const char *ns, const char *key, int8_t value)
    {
        Lock lock(mutex);

        KVPair *pair = find_set(ns, key);
        pair->type = Storage::VAL_INT8;
        pair->v8 = value;
        return true;
    }

    bool get(const char *ns, const char *key, char *value, size_t *n)
    {
        ASSERT(value);
        ASSERT(n);

        Lock lock(mutex);

        KVPair *pair = find(ns, key, Storage::VAL_STR, 0);
        if (!pair) return false;

        size_t mx = strlen(pair->s) + 1; // space for '\0'
        if (mx > *n) mx = *n;
        strncpy(value, pair->s, mx);
        *n = mx;
        return true;
    }

    bool get(const char *ns, const char *key, int32_t *value)
    {
        Lock lock(mutex);

        KVPair *pair = find(ns, key, Storage::VAL_INT32, 0);
        if (!pair) return false;

        if (value) *value = pair->v32;
        return true;
    }

    bool get(const char *ns, const char *key, int16_t *value)
    {
        Lock lock(mutex);

        KVPair *pair = find(ns, key, Storage::VAL_INT16, 0);
        if (!pair) return false;

        if (value) *value = pair->v16;
        return true;
    }

    bool get(const char *ns, const char *key, int8_t *value)
    {
        Lock lock(mutex);

        KVPair *pair = find(ns, key, Storage::VAL_INT8, 0);
        if (!pair) return false;

        if (value) *value = pair->v8;
        return true;
    }

    bool erase(const char *ns, const char *key)
    {
        Lock lock(mutex);

        KVPair *pair = find(ns, key, 0);
        if (!pair) return false;
        
        pairs.remove(pair, 0);
        pair->free_pair();
        delete pair;
        return true;
    }
};

static Helper helper;

    /*
     *
     */

Storage::Storage(const char *_ns, bool _verbose)
:   ns(_ns),
    verbose(_verbose)
{
}

Storage::~Storage()
{
}

Storage::Type Storage::get_type(const char *key)
{
    IGNORE(key);
    ASSERT(0);
    return VAL_NONE;
}

bool Storage::get(const char *key, int8_t *value)
{
    return helper.get(ns, key, value);
}

bool Storage::get(const char *key, int16_t *value)
{
    return helper.get(ns, key, value);
}

bool Storage::get(const char *key, int32_t *value)
{
    return helper.get(ns, key, value);
}

bool Storage::get(const char *key, char *value, size_t *n)
{
    return helper.get(ns, key, value, n);
}


bool Storage::set(const char *key, int8_t value)
{
    return helper.set(ns, key, value);
}

bool Storage::set(const char *key, int16_t value)
{
    return helper.set(ns, key, value);
}

bool Storage::set(const char *key, int32_t value)
{
    return helper.set(ns, key, value);
}

bool Storage::set(const char *key, const char *value)
{
    return helper.set(ns, key, value);
}


bool Storage::set_blob(const char *key, void *data, size_t size)
{
    IGNORE(key);
    IGNORE(data);
    IGNORE(size);
    ASSERT(0);
    return false;
}

bool Storage::get_blob(const char *key, void *data, size_t *size)
{
    IGNORE(key);
    IGNORE(data);
    IGNORE(size);
    ASSERT(0);
    return false;
}

bool Storage::erase(const char *key)
{
    return helper.erase(ns, key);
}

bool Storage::commit()
{
    ASSERT(0);
    return false;
}

    /*
     *
     */

Storage::List::List(const char *_ns)
:   ns(_ns),
    iter(0)
{
}
 
Storage::List::~List()
{
}
 
bool Storage::List::get(char *ns, char *key, Storage::Type *type)
{
    IGNORE(ns);
    IGNORE(key);
    IGNORE(type);
    ASSERT(0);
    return false;
}
 
}   //  panglos

//  FIN
