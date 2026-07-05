
#pragma once

#include <stdint.h>
#include <stddef.h>

#include "panglos/storage.h"

    /*
     *
     */

namespace panglos {
    
class Out;
class Mutex;

class Page
{
public:
    typedef enum {
        NONE,
        READ,
        WRITE,
        ALLOC,
    }   Error;

    typedef void* PhyAddr;
    typedef uint16_t PageAddr;

    virtual ~Page() { };

    virtual bool erase() = 0;
    virtual size_t round(size_t sz) = 0;
    virtual PageAddr alloc(size_t s) = 0;
    virtual bool write(PageAddr addr, const uint8_t *data, size_t size) = 0;
    virtual bool can_write(PageAddr addr, const uint8_t *data, size_t size) = 0;
    virtual bool read(PageAddr addr, uint8_t *data, size_t size) = 0;
    virtual PhyAddr get_addr() = 0;
    virtual Error get_error() = 0;
    static const char *strerror(Error);

    static Page *create_ram(PhyAddr addr, size_t s, bool simulate_flash=false);
    static Page *create_flash(PhyAddr addr, size_t s);

    // for debug
    virtual void dump(panglos::Out *, PageAddr start=0, PageAddr end=0) { (start=end); (end=start); }
};

    /*
     *
     */

class FlashStorage
{
    Page *page;
    Page *other;

public:
    typedef Page::PageAddr PageAddr;

    struct Header
    {
        PageAddr magic;
        PageAddr groups;
    };

    struct GroupEntry
    {
        PageAddr next;
        PageAddr items;
        PageAddr name;
        PageAddr size;
        char text[0];
    };

    struct ItemEntry
    {
        PageAddr next;
        PageAddr name;
        PageAddr name_size;
        PageAddr data_size;
        PageAddr type;
        uint8_t data[0];
    };

    static const PageAddr END = 0xffff;
    static const PageAddr ERASED = 0x0000;
    static const PageAddr MAGIC = 0xdb01;
    static const PageAddr MAX_NAME = 32;

    typedef enum {
        NONE = 0,
        BLOB = 1,
        STR  = 2,
        U32  = 3,
        U16  = 4,
        U8   = 5,
    }   Type;

    FlashStorage(Page *p1, Page *p2);

    Page *get_active_page() { return page; }

    PageAddr find_group(const char *name);
    PageAddr find_item(PageAddr group, const char *name);

    PageAddr write_group(const char *name);
    PageAddr write_item(PageAddr group, const char *name, Type ty, const uint8_t *data, size_t size=0);

    bool delete_group(PageAddr addr);
    bool delete_item(PageAddr addr);

    // iterators
    PageAddr group_iter(Page *page, GroupEntry *ge, PageAddr *iter);
    PageAddr item_iter(Page *page, ItemEntry *item, PageAddr group, PageAddr *iter);

    size_t get_total_used();
    Page::Error get_error(size_t page_idx);

    bool copy_to_other();
    bool swap_pages();

    class Visitor
    {
    public:
        virtual bool on_group(PageAddr addr, const char *name) = 0;
        virtual bool on_item(PageAddr addr, const char *name, Type ty, const uint8_t* data, size_t size) = 0;
        virtual bool skip_group(const char * /*name*/) { return false; }
        virtual bool skip_item(const char * /*name*/) { return false; }
    };
    friend class CopyVisitor;

    bool visit(Page *page, Visitor *visitor);

private:
    PageAddr get_type_size(Type);
    PageAddr write_new_item(Page *page, ItemEntry *item, const char *name, const uint8_t *data);
    PageAddr write_new_group(Page *page, const char *name);
    void set_item_entry(struct ItemEntry *item, const char *name, Type ty, const uint8_t *data, size_t size);
};

StorageInterface *make_storage(FlashStorage *fs);

}   //  namespace panglos

//  FIN
