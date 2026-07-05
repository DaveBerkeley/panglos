
#if !defined(__PANGLOS_STORAGE__)
#define __PANGLOS_STORAGE__

namespace panglos {

    /*
     *
     */

typedef enum 
{ 
    VAL_NONE, 
    VAL_INT8, 
    VAL_INT16, 
    VAL_INT32, 
    VAL_STR, 
    VAL_BLOB,
    VAL_OTHER,
} StorageType;

const char *storage_type(StorageType ty);

    /*
     *
     */

class StorageInterface
{
public:
    virtual ~StorageInterface() { }

    virtual StorageType get_type(const char *ns, const char *key) = 0;

    virtual bool get(const char *ns, const char *key, int8_t *value) = 0;
    virtual bool get(const char *ns, const char *key, int16_t *value) = 0;
    virtual bool get(const char *ns, const char *key, int32_t *value) = 0;
    virtual bool get(const char *ns, const char *key, char *value, size_t *n) = 0;

    virtual bool set(const char *ns, const char *key, int8_t value) = 0;
    virtual bool set(const char *ns, const char *key, int16_t value) = 0;
    virtual bool set(const char *ns, const char *key, int32_t value) = 0;
    virtual bool set(const char *ns, const char *key, const char *value) = 0;

    virtual bool set_blob(const char *ns, const char *key, void *data, size_t size) = 0;
    virtual bool get_blob(const char *ns, const char *key, void *data, size_t *size) = 0;

    virtual bool erase(const char *ns, const char *key) = 0;

    virtual bool commit() { return true; };

    class Iter
    {
    public: 
        virtual ~Iter() { }
        virtual bool next(char *ns, char *key, StorageType *type, size_t max_sz) = 0;
    };

    virtual Iter *make_iter() = 0;
};

    /*
     *
     */

class Storage
{
    StorageInterface *flash;
    const char *ns;
    bool verbose;
public:
    class Handle;
    Handle *handle;

    Storage(const char *ns, bool verbose=false, StorageInterface *flash=0);
    ~Storage();

    const char *get_ns() { return ns; }
    StorageType get_type(const char *key);

    bool get(const char *key, int8_t *value);
    bool get(const char *key, int16_t *value);
    bool get(const char *key, int32_t *value);
    bool get(const char *key, char *value, size_t *n);

    bool set(const char *key, int8_t value);
    bool set(const char *key, int16_t value);
    bool set(const char *key, int32_t value);
    bool set(const char *key, const char *value);

    bool set_blob(const char *key, void *data, size_t size);
    bool get_blob(const char *key, void *data, size_t *size);

    bool erase(const char *key);

    bool commit();

    class List
    {
        const char *ns;

    public:
        class Iter;
        Iter *iter;

        List(const char *ns=0, StorageInterface *fs=0);
        ~List();

        // note: max len(ns) or len(key) on esp32 is 16
        bool get(char *ns, char *key, StorageType *type, size_t max_len);
    };

    //  Functions for unit tests
    static void clear_all();
    static bool load(const char *path);
    static bool save(const char *path);

    // Validate params

    static bool validate_range(int32_t v, const char *name, int32_t lo, int32_t hi);
    static bool validate_set(int32_t v, const char *name, const int32_t *set, size_t n);

    //  Get a block of params

    struct IntParam
    {
        const char *name;
        int32_t *value;
        bool (*validate)(int32_t v, const char *name);
    };

    void get_params(const struct IntParam *params);
    void show_params(const struct IntParam *params);

    static StorageInterface *native_storage;
};

}   //  namespace panglos

#endif // !defined(__PANGLOS_STORAGE__)

//  FIN
