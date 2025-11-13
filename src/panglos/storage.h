
#if !defined(__PANGLOS_STORAGE__)
#define __PANGLOS_STORAGE__

namespace panglos {

class Storage
{
    const char *ns;
    bool verbose;
public:
    class Handle;
    Handle *handle;

    Storage(const char *ns, bool verbose=false);
    ~Storage();

    typedef enum 
    { 
        VAL_NONE, 
        VAL_INT8, 
        VAL_INT16, 
        VAL_INT32, 
        VAL_STR, 
        VAL_BLOB,
        VAL_OTHER,
    } Type;

    Type get_type(const char *key);

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

        List(const char *ns);
        ~List();

        bool get(char *ns, char *key, Type *type);
    };
};

}   //  namespace panglos

#endif // !defined(__PANGLOS_STORAGE__)

//  FIN
