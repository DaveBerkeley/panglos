
#include <stdlib.h>
#include <string.h>

#include "panglos/debug.h"
#include "panglos/io.h"
#include "panglos/mutex.h"

#include "panglos/flash_storage.h"

namespace panglos {

    /*
     *
     */

static panglos::Out *out = 0;

    /*
     *
     */

static LUT error_lut[] = {
    {   "NONE", Page::NONE },
    {   "READ", Page::READ },
    {   "WRITE", Page::WRITE },
    {   "ALLOC", Page::ALLOC },
    {   0 },
};

const char *Page::strerror(Page::Error err)
{
    return lut(error_lut, err);
}

    /*
     *  RAM Based Page
     */

class RamPage : public Page
{
public:
    Error error;
    const PhyAddr phy_addr;
    const PageAddr page_size;
    uint8_t *base;
    PageAddr end;
    bool sim_flash;

    RamPage(PhyAddr p, size_t s, bool simulate_flash)
    :   error(NONE),
        phy_addr(p), 
        page_size(PageAddr(s)),
        base(0), 
        end(0),
        sim_flash(simulate_flash)
    {
        base = (uint8_t *) malloc(s);
        erase();
    }

    ~RamPage()
    {
        free(base);
    }

    virtual Error get_error() override
    {
        return error;
    }

    virtual PhyAddr get_addr() override
    {
        return phy_addr;
    }

    virtual bool erase() override
    {
        memset(base, 0xff, page_size);
        end = 0;
        return true;
    }

    virtual size_t round(size_t sz) override
    {
        if (sz & 0x01) sz += 1; // round up to even number
        return sz;
    }

    virtual PageAddr alloc(size_t s) override
    {
        s = round(s);
        if ((end + s) > page_size)
        {
            error = ALLOC;
            return 0;
        }
        const PageAddr block = end;
        end += PageAddr(s);
        return block;
    }

    virtual bool write(PageAddr addr, const uint8_t *data, size_t size) override
    {
        if ((addr + size) > page_size)
        {
            error = WRITE;
            return false;
        }
        if (out) panglos::dump(out, data, size, true);
        memcpy(& base[addr], data, size);
        return true;
    }

    virtual bool can_write(PageAddr addr, const uint8_t *data, size_t size) override
    {
        if ((addr + size) > page_size) return false;
        // ram can always write

        if (sim_flash)
        {
            // simulate flash writes
            for (size_t i = 0; i < size; i++)
            {
                const uint8_t was = base[i+addr];
                const uint8_t d = *data++;
                // can't change 0 to 1
                if ((was & d) != d)
                {
                    //PO_DEBUG("was=%#x d=%#x", was, d);
                    return false;
                }
            }
        }

        return true;
    }

    virtual bool read(PageAddr addr, uint8_t *data, size_t size) override
    {
        if ((addr + size) > page_size)
        {
            error = READ;
            return false;
        }
        memcpy(data, & base[addr], size);
        return true;
    }

    virtual void dump(panglos::Out *out, PageAddr start, PageAddr _end) override
    {
        panglos::out = out;
        panglos::dump(out, base + start, _end ? _end : end, true);
    }
};

Page *Page::create_ram(PhyAddr addr, size_t s, bool simulate_flash)
{
    return new RamPage(addr, s, simulate_flash);
}

    /*
     *
     */

FlashStorage::PageAddr FlashStorage::get_type_size(Type ty)
{
    switch (ty)
    {
        case U32    : return sizeof(uint32_t);
        case U16    : return sizeof(uint16_t);
        case U8     : return sizeof(uint8_t);
        case BLOB   : return 0; // variable size
        case STR    : return 0; // variable size
        default : ASSERT(0); return 0;
    }
}

    /*
     *
     */

FlashStorage::FlashStorage(Page *p1, Page *p2)
:   page(p1),
    other(p2)
{
    ASSERT(page);
}

FlashStorage::PageAddr FlashStorage::group_iter(Page *p, GroupEntry *ge, PageAddr *iter)
{
    p = p ? p : page;
    
    if (*iter == 0)
    {
        // first iteration, read the header
        struct Header header;
        if (!p->read(*iter, (uint8_t*) & header, sizeof(header)))
            return false;
        if (header.magic != MAGIC)
            return false;
        *iter = header.groups;
    }

    while (true)
    {
        if (*iter == END) return 0; // no more entries

        if (!p->read(*iter, (uint8_t*) ge, sizeof(*ge)))
            return 0;

        const PageAddr addr = *iter;
        *iter = ge->next;

        // if group.items or group.name has been erased, skip this group
        if (ge->items == ERASED) continue;
        if (ge->name == ERASED) continue;

        return addr;
    }
}

bool FlashStorage::swap_pages()
{
    Page *p = other;
    other = page;
    page = p;
    return true;
}

FlashStorage::PageAddr FlashStorage::item_iter(Page *p, ItemEntry *item, PageAddr gaddr, PageAddr *iter)
{
    p = p ? p : page;

    if (*iter == 0)
    {
        struct GroupEntry group;
        if (!p->read(gaddr, (uint8_t*) & group, sizeof(group)))
            return 0;

        *iter = group.items;
    }

    while (true)
    {
        const PageAddr addr = *iter;
        if (addr == END) return 0;

        if (!p->read(addr, (uint8_t*) item, sizeof(*item)))
            return 0;

        *iter = item->next;

        if (item->name == ERASED) continue;

        return addr;
    }
}

FlashStorage::PageAddr FlashStorage::find_group(const char *name)
{
    PageAddr iter = 0;

    if (!name) return 0;

    while (true)
    {
        struct GroupEntry ge;
        PageAddr addr = group_iter(page, & ge, & iter);
        if (addr == 0) break;

        if (ge.name == ERASED) continue;

        char buff[MAX_NAME];
        if (!page->read(ge.name, (uint8_t*) buff, ge.size))
            return 0;
 
        if (!strcmp(buff, name))
        {
            return addr;
        }
    }
    return 0;
}

FlashStorage::PageAddr FlashStorage::find_item(PageAddr group, const char *name)
{
    PageAddr iter = 0;

    if (!name) return 0;

    while (true)
    {
        struct ItemEntry item;
        PageAddr addr = item_iter(page, & item, group, & iter);
        if (addr == 0) break;
        if (addr == END) break;

        if (item.name == ERASED) continue;

        char buff[MAX_NAME];
        if (!page->read(item.name, (uint8_t*) buff, item.name_size))
            return 0;

        if (!strcmp(buff, name))
        {
            return addr;
        }
    }
    return 0;
}

FlashStorage::PageAddr FlashStorage::write_new_group(Page *p, const char *name)
{
    const size_t len = strlen(name) + 1;
    const size_t size = sizeof(GroupEntry) + len;
    PageAddr addr = p->alloc(size);
    if (!addr)
    {
        PO_ERROR("%s", Page::strerror(p->get_error()));
        return 0;
    }

    GroupEntry group = {
        .next = END,
        .items = END,
        .name = PageAddr(addr + sizeof(GroupEntry)),
        .size = PageAddr(len),
    };

    bool ok = p->write(addr, (uint8_t*) & group, sizeof(group));
    if (!ok)
    {
        PO_ERROR("%s", Page::strerror(p->get_error()));
        return 0;
    }

    ok = p->write(group.name, (uint8_t*) name, len);
    if (!ok)
    {
        PO_ERROR("%s", Page::strerror(p->get_error()));
        return 0;
    }

    return addr;
}

    /*
     *
     */

#define FIELD_OFFSET(addr, type, field) \
    FlashStorage::PageAddr(addr + intptr_t(& ((type*) 0)->field))

FlashStorage::PageAddr get_header_groups()
{
    return FIELD_OFFSET(0, struct FlashStorage::Header, groups);
}

FlashStorage::PageAddr get_group_next(FlashStorage::PageAddr addr)
{
    return FIELD_OFFSET(addr, struct FlashStorage::GroupEntry, next);
}

FlashStorage::PageAddr get_group_items(FlashStorage::PageAddr addr)
{
    return FIELD_OFFSET(addr, struct FlashStorage::GroupEntry, items);
}

FlashStorage::PageAddr get_item_next(FlashStorage::PageAddr addr)
{
    return FIELD_OFFSET(addr, struct FlashStorage::ItemEntry, next);
}

    /*
     *
     */

FlashStorage::PageAddr FlashStorage::write_group(const char *name)
{
    bool ok;
    PageAddr iter = 0;
    PageAddr last = 0;
    while (true)
    {
        GroupEntry ge;
        PageAddr addr = group_iter(page, & ge, & iter);
        // TODO : check for duplicate group?
        if (addr == 0)
            break;
        last = addr;
    }

    if (last == 0)
    {
        // no groups in flash yet, write header
        struct Header header = {
            .magic = MAGIC,
            .groups = sizeof(struct Header),
        };
        PageAddr start = page->alloc(sizeof(header));
        ASSERT(start == 0); // should be the first entry

        ok = page->write(start, (uint8_t*) & header, sizeof(header));
        if (!ok)
        {
            PO_ERROR("%s", Page::strerror(page->get_error()));
            return 0;
        }
    }

    PageAddr start = write_new_group(page, name);
    if (!last) return start;

    // Write the next pointer to the previous group entry
    PageAddr addr = get_group_next(last);
    ok = page->write(addr, (uint8_t*) & start, sizeof(start));
    if (!ok)
    {
        PO_ERROR("%s", Page::strerror(page->get_error()));
        return 0;
    }

    return start;
}

FlashStorage::PageAddr FlashStorage::write_new_item(Page *p, ItemEntry *item, const char *name, const uint8_t *data)
{
    PageAddr sz = PageAddr(sizeof(ItemEntry) + item->data_size + item->name_size);
    PageAddr addr = p->alloc(sz);
    if (!addr) return 0;

    item->name += addr;

    if (!p->write(addr, (uint8_t*) item, sizeof(ItemEntry)))
    {
        PO_ERROR("%s", Page::strerror(p->get_error()));
        return 0;
    }
    if (!p->write(addr + sizeof(ItemEntry), data, item->data_size))
    {
        PO_ERROR("%s", Page::strerror(p->get_error()));
        return 0;
    }
    if (!p->write(item->name, (uint8_t*) name, item->name_size))
    {
        PO_ERROR("%s", Page::strerror(p->get_error()));
        return 0;
    }
    return addr;
}

void FlashStorage::set_item_entry(struct ItemEntry *item, const char *name, FlashStorage::Type ty, const uint8_t *data, size_t size)
{
    PageAddr data_size = get_type_size(ty);
    if (ty == STR)
    {
        data_size = PageAddr(strlen((char*) data) + 1);
    }
    if (ty == BLOB)
    {
        data_size = PageAddr(size);
    }
    const PageAddr name_size = PageAddr(strlen(name) + 1);

    item->next = END;
    item->name = PageAddr(sizeof(ItemEntry) + data_size);
    item->name_size = name_size;
    item->data_size = data_size;
    item->type = ty;
}

FlashStorage::PageAddr FlashStorage::write_item(PageAddr group, const char *name, Type ty, const uint8_t *data, size_t size)
{
    // append new item to the end of the group's list
    struct ItemEntry item;
    set_item_entry(& item, name, ty, data, size);

    GroupEntry ge;
    if (!page->read(group, (uint8_t*) & ge, sizeof(ge)))
        return 0;

    if (ge.items == END)
    {
        // Not items in the group yet, so write the initial one
        PageAddr addr = write_new_item(page, & item, name, data);
        if (!addr) return 0;
        // Update the group to point to the new item
        PageAddr next = get_group_items(group);
        if (!page->write(next, (uint8_t*) & addr, sizeof(addr)))
            return 0;
        return addr;
    }
 
    // iterate through the items to see if the item already exists.
    // it may be possible to update it.
    // Always find the last item on the list, so we can append

    struct ItemEntry extant;
    PageAddr addr = ge.items;
    PageAddr match = 0;
    PageAddr last = 0;
    while (!last)
    {
        // search existing items in the list
        if (!page->read(addr, (uint8_t*) & extant, sizeof(extant)))
            break;
        if (extant.next == END)
        {
            last = addr;
        }
        if ((extant.name != ERASED) && !match)
        {
            char buff[MAX_NAME];
            if (!page->read(extant.name, (uint8_t*) buff, extant.name_size))
                return 0;
            if (!strcmp(name, buff))
                match = addr;
        }
        addr = extant.next;
    }
    // we should have found the end of the list
    ASSERT(last);

    //PO_DEBUG("match=%#x last=%#x", match, last);

    bool erase = false;

    if (match)
    {
        // can we overwrite the existing entry?
        if (extant.data_size != item.data_size) erase = true;
        if (extant.type != ty) erase = true;
        if (extant.type == STR) erase = true;
        if (!erase)
        {
            // can we overwrite the data?
            PageAddr data_addr = PageAddr(match + sizeof(extant));
            const bool can_write = page->can_write(data_addr, data, extant.data_size);
            erase = !can_write;
        }

        if (!erase)
        {
            // update the existing data
            if (!page->write(PageAddr(match + sizeof(extant)), data, extant.data_size))
                return 0;
            return match;
        }

        if (match != last)
        {
            // Erase the existing record
            struct ItemEntry erased;
            if (!page->read(match, (uint8_t*) & erased, sizeof(erased)))
                return 0;
            erased.name = ERASED;
            if (!page->write(match, (uint8_t*) & erased, sizeof(erased)))
                return 0;
        }
    }

    ASSERT(last);
    // append new item and update the last item
    addr = write_new_item(page, & item, name, data);
    if (!addr) return 0;
    // Update the last item to point to the new item
    extant.next = addr;
    if (erase && (match == last))
    {
        // we need to erase the match
        extant.name = ERASED;
    }
    if (!page->write(last, (uint8_t*) & extant, sizeof(extant)))
        return 0;
    return addr;
}

Page::Error FlashStorage::get_error(size_t page_idx)
{
    switch (page_idx)
    {
        case 0 : return page->get_error();
        case 1 : return other->get_error();
        default : ASSERT(0); return Page::NONE;
    }
}

    /*
     *
     */

size_t FlashStorage::get_total_used()
{
    size_t total = 0;
    // read the total storage used by non-erased fields
    struct Header header;
    struct GroupEntry group;
    struct ItemEntry item;

    // Read the header
    if (!page->read(0, (uint8_t*) & header, sizeof(header)))
        return 0;

    if (header.magic != MAGIC)
        return total;

    total += sizeof(header);
    PageAddr gaddr = header.groups;

    //  Iterate through the non-erased groups

    while (gaddr != END)
    {
        bool ok = page->read(gaddr, (uint8_t*) & group, sizeof(group));
        if (!ok) return 0;
        if (group.name == ERASED)
        {
            // skip erased group
            gaddr = group.next;
            continue;
        }

        // add the header and name size for this group
        total += sizeof(header);
        total += page->round(group.size);
        gaddr = group.next; // read for the next iteration

        // read the non-erased items
        PageAddr iaddr = group.items;
        while (iaddr != END)
        {
            ok = page->read(iaddr, (uint8_t*) & item, sizeof(item));
            iaddr = item.next;
            if (!ok) return 0;
            if (item.name == ERASED)
            {
                // skip erased item
                continue;
            }

            total += sizeof(item);
            total += page->round(item.data_size);
            total += page->round(item.name_size);
        }
    }

    return total;
}

    /*
     *
     */

class CopyVisitor : public FlashStorage::Visitor
{
public:
    typedef FlashStorage::PageAddr PageAddr;

    enum Error
    {
        NONE,
        ALLOC,
        WRITE_GROUP,
        WRITE_ITEM,
        WRITE,
    };
    
    FlashStorage *flash;
    bool verify;
    enum Error last_error;
    Page *page;
    PageAddr glast;
    PageAddr ilast;
    int count;

    bool error(enum Error err)
    {
        PO_ERROR("err=%d", err);
        return true;
    }

    virtual bool on_group(PageAddr, const char *name) override
    {
        //PO_DEBUG("name=%s", name);
        if (verify) return false;
        ASSERT(page);

        // write new group
        PageAddr addr = flash->write_new_group(page, name);
        if (!addr) return error(WRITE_GROUP);

        PageAddr ok = page->write(glast, (uint8_t*) & addr, sizeof(addr));
        if (!ok) return error(WRITE);
        // move the *next of the group list
        glast = get_group_next(addr);
        // add subsequent items to this group
        ilast = get_group_items(addr);
        return false; // continue visiting
    }

    virtual bool on_item(PageAddr, const char *name, FlashStorage::Type ty, const uint8_t* data, size_t size) override
    {
        count += 1;
        if (verify) return false;
        ASSERT(page);

        // write new item
        FlashStorage::ItemEntry item;
        flash->set_item_entry(& item, name, ty, data, size);
        PageAddr addr = flash->write_new_item(page, & item, name, data);
        if (!addr) error(WRITE_ITEM);
        // update the last item pointer
        PageAddr ok = page->write(ilast, (uint8_t*) & addr, sizeof(addr));
        if (!ok) return error(WRITE);
        ilast = get_item_next(addr);
        return false;
    }    

    CopyVisitor(FlashStorage *f, bool v, Page *p=0)
    :   flash(f),
        verify(v),
        last_error(NONE),
        page(p),
        glast(0),
        ilast(0),
        count(0)
    {
    }

    bool start()
    {
        PageAddr addr = page->alloc(sizeof(FlashStorage::Header));
        if (addr) return error(ALLOC); // first alloc must be at start of Page
        // last pointer to groups is in the header
        glast = get_header_groups();
        return true;
    }

    bool end()
    {
        // Write the header
        struct FlashStorage::Header header;
        header.magic = FlashStorage::MAGIC;
        header.groups = PageAddr(sizeof(header));
        PageAddr addr = page->write(0, (uint8_t*) & header, sizeof(header));
        return addr == 0;
    }
};

    /*
     *
     */

bool FlashStorage::copy_to_other()
{
    int old_count = 0;
    {
        // check we can visit all the items
        CopyVisitor visitor(this, true);
        if (!visit(page, & visitor))
        {
            return false;
        }
        old_count = visitor.count;
    }

    //PO_DEBUG("found %d items", old_count);

    CopyVisitor visitor(this, false, other);

    if (!visitor.start())
        return false;

    if (!visit(page, & visitor))
        return false;

    if (!visitor.end())
        return false;

    int new_count = 0;
    // validate the new data : just count the entries
    {
        // check we can visit all the items
        CopyVisitor visitor(this, true);
        if (!visit(other, & visitor))
        {
            return false;
        }
        new_count = visitor.count;
    }
    if (old_count != new_count)
    {
        PO_ERROR("!!");
        return false;
    }

    // swap pages, erase the old one
    swap_pages();
    return other->erase();
}

    /*
     *
     */

class FsBuffer
{
public:
    uint8_t *data;
    size_t size;

    FsBuffer(size_t s)
    :   data(0),
        size(0)
    {
        ensure(s);
    }

    ~FsBuffer()
    {
        free(data);
    }

    void ensure(size_t sz)
    {
        if (sz <= size) return;
        free(data);
        size = sz;
        data = (uint8_t*) malloc(size);
    }
};

    /*
     *
     */

bool FlashStorage::visit(Page *p, FlashStorage::Visitor *visitor)
{
    p = p ? p : page;
    PageAddr giter = 0;
    char name[MAX_NAME];
    FsBuffer buffer(64);

    while (true)
    {
        GroupEntry group;

        PageAddr gaddr = group_iter(p, & group, & giter);
        if (!gaddr) break;

        if (!p->read(group.name, (uint8_t*) name, group.size))
            return false;

        if (visitor->skip_group(name))
            continue;

        const bool found_group = visitor->on_group(gaddr, name);

        PageAddr iiter = 0;
        while (true)
        {
            ItemEntry item;
 
            PageAddr iaddr = item_iter(p, & item, gaddr, & iiter);
            if (!iaddr) break;

            if (!p->read(item.name, (uint8_t*) name, item.name_size))
                return false;

            if (visitor->skip_item(name))
                continue;

            PageAddr daddr = iaddr + sizeof(item);
            buffer.ensure(item.data_size);
            if (!p->read(daddr, (uint8_t*) buffer.data, item.data_size))
                return false;

            bool exit = visitor->on_item(iaddr, name, (FlashStorage::Type) item.type, buffer.data, item.data_size);
            if (exit)
            {
                return true;
            }
        }

        if (found_group) break;
    }

    return true;
}

    /*
     *
     */

bool FlashStorage::delete_group(FlashStorage::PageAddr addr)
{
    GroupEntry group;

    if (!page->read(addr, (uint8_t*) & group, sizeof(group)))
        return false;
    group.name = ERASED;
    group.items = ERASED;
    return page->write(addr, (uint8_t*) & group, sizeof(group));
}

    /*
     *
     */

bool FlashStorage::delete_item(FlashStorage::PageAddr addr)
{
    ItemEntry item;

    if (!page->read(addr, (uint8_t*) & item, sizeof(item)))
        return false;
    item.name = ERASED;
    return page->write(addr, (uint8_t*) & item, sizeof(item));
}

}   //  namespace panglos

//  FIN
