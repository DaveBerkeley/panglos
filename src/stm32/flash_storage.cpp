
#include <string.h>

#include "stm32f4xx_hal.h"

#include "panglos/debug.h"
#include "panglos/io.h"
#include "panglos/mutex.h"

#include "panglos/flash_storage.h"
#include "panglos/storage.h"

namespace panglos {

static const LUT hal_lut[] = {
    {   "HAL_OK", HAL_OK },
    {   "HAL_ERROR", HAL_ERROR },
    {   "HAL_BUSY", HAL_BUSY },
    {   "HAL_TIMEOUT", HAL_TIMEOUT },
    {   0   },
};

static const LUT hal_flash_lut[] =
{
    {   "Read Protection error", HAL_FLASH_ERROR_RD },
    {   "Programming Sequence error", HAL_FLASH_ERROR_PGS },
    {   "Programming Parallelism error", HAL_FLASH_ERROR_PGP },
    {   "Programming Alignment error", HAL_FLASH_ERROR_PGA },
    {   "Write protection error", HAL_FLASH_ERROR_WRP },
    {   "Operation Error", HAL_FLASH_ERROR_OPERATION },
    { 0 },
};

const char *strerr(HAL_StatusTypeDef err)
{
    return lut(hal_lut, err);
}

bool error(HAL_StatusTypeDef err)
{
    if (err == HAL_OK) return false;

    const uint32_t code = HAL_FLASH_GetError();

    PO_ERROR("err=%d '%s' code=%d", err, strerr(err), (int) code);

    // error code is the OR of multiple errors
    for (const LUT *plut = hal_flash_lut; plut->text; plut++)
    {
        if ((code & plut->code) == uint32_t(plut->code))
            PO_ERROR("%s", plut->text);
    }

    return true;
}

class FlashLock : public Mutex
{
    virtual void lock() override
    {
        HAL_StatusTypeDef err = HAL_FLASH_Unlock();
        error(err);
    }

    virtual void unlock() override
    {
        HAL_StatusTypeDef err = HAL_FLASH_Lock();
        error(err);
    }
};

    /*
     *  Map physical addresses to flash sector numbers
     */

struct SectorMap
{
    intptr_t addr;
    uint32_t sector;
};

#if defined(STM32F446xx)

struct SectorMap sector_map[] = {
    {   0x08000000, FLASH_SECTOR_0, },
    {   0x08004000, FLASH_SECTOR_1, },
    {   0x08008000, FLASH_SECTOR_2, },
    {   0x0800c000, FLASH_SECTOR_3, },
    {   0x08010000, FLASH_SECTOR_4, },
    {   0x08020000, FLASH_SECTOR_5, },
    {   0x08040000, FLASH_SECTOR_6, },
    {   0x08060000, FLASH_SECTOR_7, },
    { 0 },
};

#endif

uint32_t get_sector(Page::PhyAddr addr)
{
    for (const struct SectorMap *s = sector_map; s->addr; s++)
    {
        if (intptr_t(addr) == s->addr) return s->sector;
    }
    PO_ERROR("no sector found at %p for this processor", addr);
    ASSERT(0);
    return 0;
}

    /*
     *
     */

class FlashPage : public Page
{
    PhyAddr phy_addr;
    PageAddr sbreak;
    size_t page_size;
    Error last_error;
    FlashLock flash_lock;
 
    PageAddr find_start()
    {
        uint8_t *start = (uint8_t*) phy_addr;

        // if the magic field of the header is 0xffff
        // we assume that the flash is empty
        if ((start[0] == 0xff) && (start[1] == 0xff))
            return 0;

        int32_t addr;
        for (addr = page_size-1; addr >= 0; addr--)
        {
            uint8_t* s = (uint8_t*)(intptr_t(phy_addr) + addr);
            if ((*s) != 0xff)
                break;
        }
        return addr ? round(addr+1) : 0;
    }

public:

    FlashPage(Page::PhyAddr _phy_addr, size_t _size)
    :   phy_addr(_phy_addr),
        sbreak(0),
        page_size(_size),
        last_error(NONE)
    {
        sbreak = find_start();
        PO_DEBUG("sbreak=%#x", ( int) sbreak);
    }

    virtual Error get_error() override
    {
        return last_error;
    }

    virtual bool erase() override
    {
        PO_DEBUG("");

        Lock lock(& flash_lock);

        FLASH_EraseInitTypeDef def = {
            .TypeErase = FLASH_TYPEERASE_SECTORS,
            .Banks = 0, // unused
            .Sector = get_sector(phy_addr),
            .NbSectors = 1,
            .VoltageRange = FLASH_VOLTAGE_RANGE_3,
        };
        uint32_t sector_error = 0;

        HAL_StatusTypeDef err = HAL_FLASHEx_Erase(& def, & sector_error);
        if (err != HAL_OK)
        {
            PO_ERROR("sector_error=%d", (int) sector_error);
        }
        if (error(err)) return false;
        PO_DEBUG("done");
        sbreak = 0;
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
        if ((sbreak + s) > page_size) return false;
        PageAddr a = sbreak;
        sbreak += s;
        return a;
    }

    struct Write
    {
        uint32_t addr;
        const uint8_t *data;
        size_t sz;
    };

    bool write(struct Write *wr)
    {
        ASSERT(wr);
        Lock lock(& flash_lock);
        //PO_DEBUG("addr=%#x sz=%d", (int) wr->addr, (int) wr->sz);

        while (wr->sz)
        {            
            uint64_t value = 0;
            uint32_t address = wr->addr;
            uint32_t type_program = 0;
            uint32_t inc = 0;
            if ((wr->sz < 2) || (address & 0x01))
            {
                value = *wr->data;
                type_program = FLASH_TYPEPROGRAM_BYTE;
                inc = 1;
            }
            else if ((wr->sz < 4) || (address & 0x03))
            {
                ASSERT(!(address & 0x01));
                value = * (uint16_t*) wr->data;
                type_program = FLASH_TYPEPROGRAM_HALFWORD;
                inc = 2;
            }
            else 
            {
                ASSERT(!(address & 0x03));
                value = * (uint32_t*) wr->data;
                type_program = FLASH_TYPEPROGRAM_WORD;
                inc = 4;
            }
            // note: FLASH_TYPEPROGRAM_DOUBLEWORD doesn't seem to work on the stm32f4xx
            HAL_StatusTypeDef err = HAL_FLASH_Program(type_program, address, value);
            if (error(err))
                return false;
            
            wr->data += inc;
            wr->addr += inc;
            wr->sz -= inc;
        }
        return true;
    }
 
    virtual bool write(PageAddr addr, const uint8_t *data, size_t sz) override
    {
        if ((sbreak + sz) > page_size) return false;

        struct Write wr = {
            .addr = uint32_t(addr + intptr_t(phy_addr)),
            .data = data,
            .sz = sz,
        };
        return write(& wr);
    }

    virtual bool can_write(PageAddr addr, const uint8_t *data, size_t sz) override
    {
        if ((addr + sz) > page_size) return false;
        const uint8_t* base = (const uint8_t*) phy_addr;

        for (size_t i = 0; i < sz; i++)
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

        return true;
    }

    virtual bool read(PageAddr addr, uint8_t *data, size_t sz) override
    {
        if ((addr + sz) > page_size) return false;
        memcpy(data, (uint8_t*) (addr + uint32_t(phy_addr)), sz);
        return true;
    }

    virtual PhyAddr get_addr() override
    {
        return phy_addr;
    }

    virtual void dump(panglos::Out *out, PageAddr start, PageAddr _end) override
    {
        panglos::dump(out, (const uint8_t*) (intptr_t(phy_addr) + start), _end ? _end : sbreak, true);
    }
};

    /*
     *
     */

class FlashVisitor : public FlashStorage::Visitor
{
    const char *group;
    const char *item;

    typedef FlashStorage::PageAddr PageAddr;
    typedef FlashStorage::Type Type;
public:
    struct Request
    {
        Type ty;
        uint8_t* data;
        size_t size;
    };
    struct Response
    {
        PageAddr addr;
        Type ty;
        size_t size;
        bool copied;
    };

    struct Request request;
    struct Response response;

    FlashVisitor(const char *g, const char *i)
    :   group(g),
        item(i)
    {
    }

    void set_request(Type ty, uint8_t* data, size_t size)
    {
        request.ty = ty;
        request.data = data;
        request.size = size;
        memset(& response, 0, sizeof(response));
    }

    virtual bool on_group(PageAddr, const char *) override 
    {
        return true;
    }

    virtual bool on_item(PageAddr addr, const char *, Type ty, const uint8_t* data, size_t sz) override
    {
        response.addr = addr;
        response.ty = ty;
        response.size = sz;

        // this is just a query
        if (request.ty == FlashStorage::NONE) return true;
        
        // we've requested a different type
        if (request.ty != ty) return true;

        if (request.size && request.data)
        {
            // copy the data
            if (sz > request.size) return true;

            memcpy(request.data, data, sz);
            response.copied = true;
        }

        return true;
    }

    virtual bool skip_group(const char *name) override 
    {
        return group ? strcmp(group, name) : false;
    }

    virtual bool skip_item(const char *name) override 
    {
        return item ? strcmp(item, name) : false;
    }
};

    /*
     *  Implement Storage interface using FlashStorage
     */

class FlashInterface : public StorageInterface
{
    FlashStorage *fs;

    typedef FlashStorage::PageAddr PageAddr;

    static StorageType to_storage_type(FlashStorage::Type type)
    {
        switch (type)
        {
            case FlashStorage::NONE : return StorageType::VAL_NONE;
            case FlashStorage::U8 : return StorageType::VAL_INT8;
            case FlashStorage::U16 : return StorageType::VAL_INT16;
            case FlashStorage::U32 : return StorageType::VAL_INT32;
            case FlashStorage::STR : return StorageType::VAL_STR;
            case FlashStorage::BLOB : return StorageType::VAL_BLOB;
            default : return StorageType::VAL_OTHER;
        }
    }
 
public:
    FlashInterface(FlashStorage *_fs)
    :   fs(_fs)
    {
    }

    virtual StorageType get_type(const char *ns, const char *key) override
    {
        FlashVisitor visitor(ns, key);
        visitor.set_request(FlashStorage::NONE, 0, 0);

        if (!fs->visit(0, & visitor))
            return StorageType::VAL_NONE;

        return to_storage_type(visitor.response.ty);
    }

    bool get(const char *ns, const char *key, int8_t *value) override
    {
        FlashVisitor visitor(ns, key);
        visitor.set_request(FlashStorage::U8, (uint8_t*) value, sizeof(*value));

        fs->visit(0, & visitor);
        return visitor.response.copied;
    }

    bool get(const char *ns, const char *key, int16_t *value) override
    {
        FlashVisitor visitor(ns, key);
        visitor.set_request(FlashStorage::U16, (uint8_t*) value, sizeof(*value));

        fs->visit(0, & visitor);
        return visitor.response.copied;
    }

    bool get(const char *ns, const char *key, int32_t *value) override
    {
        FlashVisitor visitor(ns, key);
        visitor.set_request(FlashStorage::U32, (uint8_t*) value, sizeof(*value));

        fs->visit(0, & visitor);
        return visitor.response.copied;
    }

    bool get(const char *ns, const char *key, char *value, size_t *n) override
    {
        FlashVisitor visitor(ns, key);
        visitor.set_request(FlashStorage::STR, (uint8_t*) value, *n);

        fs->visit(0, & visitor);
        return visitor.response.copied;
    }

    bool get_blob(const char *ns, const char *key, void *data, size_t *size) override
    {
        FlashVisitor visitor(ns, key);
        visitor.set_request(FlashStorage::BLOB, (uint8_t*) data, *size);

        fs->visit(0, & visitor);
        return visitor.response.copied;
    }

    PageAddr set_group(const char *ns)
    {
        PageAddr gaddr = fs->find_group(ns);
        if (gaddr) return gaddr;
        return fs->write_group(ns);
    }

    bool set(const char *ns, const char *key, int8_t value) override
    {
        PageAddr gaddr = set_group(ns);
        if (gaddr == 0) return false;
        return fs->write_item(gaddr, key, FlashStorage::U8, (uint8_t*) & value, 0);
    }

    bool set(const char *ns, const char *key, int16_t value) override
    {
        PageAddr gaddr = set_group(ns);
        if (gaddr == 0) return false;
        return fs->write_item(gaddr, key, FlashStorage::U16, (uint8_t*) & value, 0);
    }

    bool set(const char *ns, const char *key, int32_t value) override
    {
        PageAddr gaddr = set_group(ns);
        if (gaddr == 0) return false;
        return fs->write_item(gaddr, key, FlashStorage::U32, (uint8_t*) & value, 0);
    }

    bool set(const char *ns, const char *key, const char *value) override
    {
        PageAddr gaddr = set_group(ns);
        if (gaddr == 0) return false;
        return fs->write_item(gaddr, key, FlashStorage::STR, (uint8_t*) value, 0);
    }

    bool set_blob(const char *ns, const char *key, void *data, size_t sz) override
    {
        PageAddr gaddr = set_group(ns);
        if (gaddr == 0) return false;
        return fs->write_item(gaddr, key, FlashStorage::BLOB, (uint8_t*) data, sz);
    }

    bool erase(const char *ns, const char *key) override
    {
        PageAddr gaddr = fs->find_group(ns);
        if (!gaddr) return false;
        PageAddr addr = fs->find_item(gaddr, key);
        if (!addr) return false;
        return fs->delete_item(addr);
    }

    bool commit() override
    {
        return true;
    }

    class Iter : public StorageInterface::Iter
    {
        FlashStorage *fs;
        PageAddr g_iter;
        PageAddr i_iter;
        PageAddr gaddr;
        char group_name[FlashStorage::MAX_NAME];
        char item_name[FlashStorage::MAX_NAME];
        FlashStorage::GroupEntry group;
        FlashStorage::ItemEntry item;

        PageAddr next_group()
        {
            i_iter = 0;
            gaddr = fs->group_iter(0, & group, & g_iter);

            if (gaddr)
            {
                Page *page = fs->get_active_page();
                if (!page->read(group.name, (uint8_t*) & group_name, group.size))
                    return false;
            }

            return gaddr;
        }

        virtual bool next(char *ns, char *key, StorageType *type, size_t max_sz) override
        {
            PageAddr addr = 0;

            if (!g_iter)
            {
                if (!next_group())
                {
                    return false;
                }
            }

            addr = fs->item_iter(0, & item, gaddr, & i_iter);
            if (!addr)
            {
                if (!next_group())
                {
                    return false;
                }
            }

            Page *page = fs->get_active_page();
            if (!page->read(item.name, (uint8_t*) & item_name, item.name_size))
                return false;

            if (ns) strncpy(ns, group_name, max_sz);
            if (key) strncpy(key, item_name, max_sz);
            if (type) *type = to_storage_type(FlashStorage::Type(item.type));
            return true;
        }
    public:
        Iter(FlashStorage *f)
        :   fs(f),
            g_iter(0),
            i_iter(0),
            gaddr(0)
        {
        }
    };    

    virtual StorageInterface::Iter *make_iter() override
    {
        return new Iter(fs);
    }
};

    /*
     *
     */

Page *Page::create_flash(Page::PhyAddr phy_addr, size_t size)
{
    return new FlashPage(phy_addr, size);
}

StorageInterface *make_storage(FlashStorage *fs)
{
    return new FlashInterface(fs);
}

StorageInterface *Storage::native_storage = 0;

}

//  FIN
