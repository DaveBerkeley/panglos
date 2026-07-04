
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
    StorageType type;
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
            case StorageType::VAL_BLOB  : free(blob); blob = 0; break; 
            case StorageType::VAL_STR   : free((void*) s); s = 0; break;
            case StorageType::VAL_INT32 : return;
            case StorageType::VAL_INT16 : return;
            case StorageType::VAL_INT8  : return;
            case StorageType::VAL_NONE  :
            case StorageType::VAL_OTHER :
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

class NativeStorage : public StorageInterface
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

    KVPair *find(const char *ns, const char *key, StorageType type, Mutex *mutex)
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
    NativeStorage()
    :   mutex(0),
        pairs(KVPair::get_next)
    {
        mutex = Mutex::create();
    }

    ~NativeStorage()
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

    virtual bool set(const char *ns, const char *key, const char *value) override
    {
        ASSERT(value);
        Lock lock(mutex);

        KVPair *pair = find_set(ns, key);
        pair->type = StorageType::VAL_STR;
        pair->s = strdup(value);
        return true;
    }

    virtual bool set(const char *ns, const char *key, int32_t value) override
    {
        Lock lock(mutex);

        KVPair *pair = find_set(ns, key);
        pair->type = StorageType::VAL_INT32;
        pair->v32 = value;
        return true;
    }

    virtual bool set(const char *ns, const char *key, int16_t value) override
    {
        Lock lock(mutex);

        KVPair *pair = find_set(ns, key);
        pair->type = StorageType::VAL_INT16;
        pair->v16 = value;
        return true;
    }

    virtual bool set(const char *ns, const char *key, int8_t value) override
    {
        Lock lock(mutex);

        KVPair *pair = find_set(ns, key);
        pair->type = StorageType::VAL_INT8;
        pair->v8 = value;
        return true;
    }

    virtual bool set_blob(const char *ns, const char *key, void *data, size_t sz) override
    {
        Lock lock(mutex);

        KVPair *pair = find_set(ns, key);
        pair->type = StorageType::VAL_BLOB;

        ASSERT(data);
        void *blob = malloc(sz);
        memcpy(blob, data, sz);

        pair->blob = blob;
        pair->size = sz;
        return true;
    }

    virtual bool get(const char *ns, const char *key, char *value, size_t *n) override
    {
        ASSERT(value);
        ASSERT(n);

        Lock lock(mutex);

        KVPair *pair = find(ns, key, StorageType::VAL_STR, 0);
        if (!pair) return false;

        size_t mx = strlen(pair->s) + 1; // space for '\0'
        if (mx > *n) mx = *n;
        strncpy(value, pair->s, mx);
        *n = mx;
        return true;
    }

    virtual bool get(const char *ns, const char *key, int32_t *value) override
    {
        Lock lock(mutex);

        KVPair *pair = find(ns, key, StorageType::VAL_INT32, 0);
        if (!pair) return false;

        if (value) *value = pair->v32;
        return true;
    }

    virtual bool get(const char *ns, const char *key, int16_t *value) override
    {
        Lock lock(mutex);

        KVPair *pair = find(ns, key, StorageType::VAL_INT16, 0);
        if (!pair) return false;

        if (value) *value = pair->v16;
        return true;
    }

    virtual bool get(const char *ns, const char *key, int8_t *value) override
    {
        Lock lock(mutex);

        KVPair *pair = find(ns, key, StorageType::VAL_INT8, 0);
        if (!pair) return false;

        if (value) *value = pair->v8;
        return true;
    }

    virtual bool get_blob(const char *ns, const char *key, void *data, size_t *sz) override
    {
        ASSERT(sz);
        Lock lock(mutex);

        KVPair *pair = find(ns, key, StorageType::VAL_BLOB, 0);
        if (!pair) return false;

        if (data)
        {
            if (pair->size > *sz) return false; // blob too big to copy
            memcpy(data, pair->blob, pair->size);
        }
        *sz = pair->size;
        return true;
    }

    virtual bool erase(const char *ns, const char *key) override
    {
        Lock lock(mutex);

        KVPair *pair = find(ns, key, 0);
        if (!pair) return false;
        
        pairs.remove(pair, 0);
        pair->free_pair();
        delete pair;
        return true;
    }

    virtual StorageType get_type(const char *ns, const char *key) override
    {
        Lock lock(mutex);

        KVPair *pair = find(ns, key, 0);
        if (!pair) return StorageType::VAL_NONE;
        return pair->type;
    }

    class Iterator : public StorageInterface::Iter
    {
        KVPair *item;
    public:
        Iterator(KVPair *p)
        :   item(p)
        {
        }
        virtual ~Iterator() {}

        virtual bool next(char *ns, char *key, StorageType *type, size_t max_sz) override
        {
            if (!item) return false;
            strncpy(ns, item->keys.ns, max_sz);
            strncpy(key, item->keys.key, max_sz);
            *type = item->type;
            item = item->next;
            return true;
        }
    };

    virtual Iter *make_iter() override
    {
        return new Iterator(pairs.head);
    }    
};

static NativeStorage helper;
StorageInterface *Storage::native_storage = & helper;

    /*
     *
     */

void Storage::clear_all()
{
    helper.clear();
}
 
}   //  panglos

//  FIN
