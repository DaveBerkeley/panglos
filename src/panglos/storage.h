
#if !defined(__PANGLOS_STORAGE__)
#define __PANGLOS_STORAGE__

namespace panglos {

class Storage
{
    typedef uintptr_t Handle;

    const char *ns;
    Handle handle;
    bool verbose;
public:
    Storage(const char *ns, bool verbose=false);
    ~Storage();

    typedef enum 
    { 
        VAL_NONE, 
        VAL_INT8, 
        VAL_INT16, 
        VAL_INT32, 
        VAL_STR, 
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

    bool erase(const char *key);

    bool commit();

    class List
    {
        const char *ns;
        uintptr_t iter;

    public:
        List(const char *ns);
        ~List();

        bool get(char *ns, char *key, Type *type);
    };
};

}   //  namespace panglos

#endif // !defined(__PANGLOS_STORAGE__)

//  FIN
