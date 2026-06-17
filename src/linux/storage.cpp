
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
        void *blob;
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
            case Storage::VAL_BLOB  : free(blob); blob = 0; break; 
            case Storage::VAL_STR   : free((void*) s); s = 0; break;
            case Storage::VAL_INT32 : return;
            case Storage::VAL_INT16 : return;
            case Storage::VAL_INT8  : return;
            case Storage::VAL_NONE  :
            case Storage::VAL_OTHER :
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
        clear(mutex);
        delete mutex;
    }

    void clear(Mutex *m=0)
    {
        Lock lock(m ? m : mutex);

        while (true)
        {
            KVPair *pair = pairs.pop(0);
            if (!pair) break;
            pair->free_pair();
            delete pair;
        }
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

    bool set_blob(const char *ns, const char *key, void *data, size_t sz)
    {
        Lock lock(mutex);

        KVPair *pair = find_set(ns, key);
        pair->type = Storage::VAL_BLOB;

        ASSERT(data);
        void *blob = malloc(sz);
        memcpy(blob, data, sz);

        pair->blob = blob;
        pair->size = sz;
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

    bool get_blob(const char *ns, const char *key, void *data, size_t *sz)
    {
        Lock lock(mutex);

        KVPair *pair = find(ns, key, Storage::VAL_BLOB, 0);
        if (!pair) return false;

        ASSERT(data);
        ASSERT(sz);
        if (pair->size > *sz) return false; // blob too big to copy
        memcpy(data, pair->blob, pair->size);
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

    Storage::Type get_type(const char *ns, const char *key)
    {
        Lock lock(mutex);

        KVPair *pair = find(ns, key, 0);
        if (!pair) return Storage::VAL_NONE;
        return pair->type;
    }

    class Iterator
    {
        KVPair *item;
    public:
        Iterator(KVPair *p)
        :   item(p)
        {
        }

        bool next(char *ns, char *key, Storage::Type *type, size_t max_sz)
        {
            if (!item) return false;
            strncpy(ns, item->keys.ns, max_sz);
            strncpy(key, item->keys.key, max_sz);
            *type = item->type;
            item = item->next;
            return true;
        }
    };

    KVPair *start()
    {
        return pairs.head;
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
    return helper.get_type(ns, key);
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
    return helper.set_blob(ns, key, data, size);
}

bool Storage::get_blob(const char *key, void *data, size_t *size)
{
    return helper.get_blob(ns, key, data, size);
}

bool Storage::erase(const char *key)
{
    return helper.erase(ns, key);
}

bool Storage::commit()
{
    return true;
}

    /*
     *
     */

class Storage::List::Iter : public Helper::Iterator
{
public:
    Iter(KVPair *p)
    :   Iterator(p)
    {
    }
};

Storage::List::List(const char *_ns)
:   ns(_ns),
    iter(0)
{
    iter = new Storage::List::Iter(helper.start());
}
 
Storage::List::~List()
{
    delete iter;
}
 
bool Storage::List::get(char *_ns, char *key, Storage::Type *type, size_t max_sz)
{
    while (true)
    {
        if (!iter->next(_ns, key, type, max_sz)) return false;
        if (ns && strcmp(_ns, ns)) continue;
        return true;
    }
}

    /*
     *
     */

void Storage::clear_all()
{
    helper.clear();
}
 
}   //  panglos

//  FIN
