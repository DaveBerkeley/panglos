
#include <string.h>

#include "stm32f4xx_hal.h"

#include "panglos/debug.h"
#include "panglos/io.h"
#include "panglos/mutex.h"

#include "panglos/flash_storage.h"
#include "panglos/storage.h"

namespace panglos {

static LUT hal_lut[] = {
    {   "HAL_OK", HAL_OK },
    {   "HAL_ERROR", HAL_ERROR },
    {   "HAL_BUSY", HAL_BUSY },
    {   "HAL_TIMEOUT", HAL_TIMEOUT },
    {   0   },
};

const char *strerr(HAL_StatusTypeDef err)
{
    return lut(hal_lut, err);
}

bool error(HAL_StatusTypeDef err)
{
    if (err == HAL_OK) return false;
    PO_ERROR("err=%d '%s'", err, strerr(err));
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
     *
     */

class FlashPage : public Page
{
    PhyAddr phy_addr;
    PageAddr sbreak;
    size_t size;
    Error last_error;
    FlashLock flash_lock;
    
    PageAddr find_start()
    {
        uint8_t *start = (uint8_t*) phy_addr;

        // if the magic field of the header is 0xffff
        // we assume that the flash is empty
        if ((start[0] == 0xff) && (start[1] == 0xff))
            return 0;

        PageAddr addr;
        for (addr = size-1; addr; addr--)
        {
            uint8_t* s = (uint8_t*)(intptr_t(phy_addr) + addr);
            if ((*s) != 0xff)
                break;
        }
        addr = round(addr+1);
        return addr;
    }

public:

    FlashPage(Page::PhyAddr _phy_addr, size_t _size)
    :   phy_addr(_phy_addr),
        sbreak(0),
        size(_size),
        last_error(NONE)
    {
        sbreak = find_start();
        PO_DEBUG("sbreak=%#x", ( int) sbreak);
    }

    virtual Error get_error() override
    {
        return last_error;
    }

#if defined(STM32F446xx)
    uint32_t get_sector()
    {
        switch((intptr_t) phy_addr)
        {
            case 0x08000000 : return FLASH_SECTOR_0;
            case 0x08004000 : return FLASH_SECTOR_1;
            case 0x08008000 : return FLASH_SECTOR_2;
            case 0x0800c000 : return FLASH_SECTOR_3;
            case 0x08010000 : return FLASH_SECTOR_4;
            case 0x08020000 : return FLASH_SECTOR_5;
            case 0x08040000 : return FLASH_SECTOR_6;
            case 0x08060000 : return FLASH_SECTOR_7;
            default: break;
        }
        ASSERT(0);
        return 0;
    }
#endif

    virtual bool erase() override
    {
        PO_DEBUG("");

        Lock lock(& flash_lock);

        FLASH_EraseInitTypeDef def = {
            .TypeErase = FLASH_TYPEERASE_SECTORS,
            .Banks = 0, // unused
            .Sector = get_sector(),
            .NbSectors = 1,
            .VoltageRange = FLASH_VOLTAGE_RANGE_3,
        };
        uint32_t sector_error = 0;

        HAL_StatusTypeDef err = HAL_FLASHEx_Erase(& def, & sector_error);
        PO_DEBUG("done. sector_error=%d", (int) sector_error);
        return !error(err);
    }

    virtual size_t round(size_t sz) override
    {
        if (sz & 0x01) sz += 1; // round up to even number
        return sz;        
    }

    virtual PageAddr alloc(size_t s) override
    {
        s = round(s);
        if ((sbreak + s) > size) return false;
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

        while (wr->sz)
        {            
            PO_DEBUG("addr=%#x sz=%d", (int) wr->addr, (int) wr->sz);
            uint64_t value = 0;
            uint32_t address = wr->addr;
            uint32_t type_program = 0;
            if ((wr->sz < 2) || (address & 0x01))
            {
                value = *wr->data;
                type_program = FLASH_TYPEPROGRAM_BYTE;
                wr->data += 1;
                wr->addr += 1;
                wr->sz -= 1;
            }
            else if ((wr->sz < 4) || (address & 0x03))
            {
                value = * (uint16_t*) wr->data;
                type_program = FLASH_TYPEPROGRAM_HALFWORD;
                wr->data += 2;
                wr->addr += 2;
                wr->sz -= 2;
            }
            else if ((wr->sz < 8) || (address & 0x07))
            {
                value = * (uint32_t*) wr->data;
                type_program = FLASH_TYPEPROGRAM_WORD;
                wr->data += 4;
                wr->addr += 4;
                wr->sz -= 4;
            }
            else 
            {
                value = * (uint64_t*) wr->data;
                type_program = FLASH_TYPEPROGRAM_WORD;
                wr->data += 8;
                wr->addr += 8;
                wr->sz -= 8;
            }
            HAL_StatusTypeDef err = HAL_FLASH_Program(type_program, address, value);
            if (error(err))
                return false;
        }
        return true;
    }
    
    virtual bool write(PageAddr addr, const uint8_t *data, size_t sz) override
    {
        if ((sbreak + sz) > size) return false;

        struct Write wr = {
            .addr = uint32_t(addr + intptr_t(phy_addr)),
            .data = data,
            .sz = sz,
        };
        return write(& wr);
    }

    virtual bool can_write(PageAddr addr, const uint8_t *data, size_t sz) override
    {
        if ((addr + sz) > size) return false;
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
        if ((addr + sz) > size) return false;
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
        return StorageType(visitor.response.ty);
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
        UNUSED(ns);
        UNUSED(key);
        UNUSED(value);
        UNUSED(n);
        ASSERT(0);
        return false;
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
        return fs->write_item(gaddr, key, FlashStorage::U8, (uint8_t*) & value);
    }

    bool set(const char *ns, const char *key, int16_t value) override
    {
        PageAddr gaddr = set_group(ns);
        if (gaddr == 0) return false;
        return fs->write_item(gaddr, key, FlashStorage::U16, (uint8_t*) & value);
    }

    bool set(const char *ns, const char *key, int32_t value) override
    {
        PageAddr gaddr = set_group(ns);
        if (gaddr == 0) return false;
        return fs->write_item(gaddr, key, FlashStorage::U32, (uint8_t*) & value);
    }

    bool set(const char *ns, const char *key, const char *value) override
    {
        UNUSED(ns);
        UNUSED(key);
        UNUSED(value);
        ASSERT(0);
        return false;
    }

    bool set_blob(const char *ns, const char *key, void *data, size_t size) override
    {
        UNUSED(ns);
        UNUSED(key);
        UNUSED(data);
        UNUSED(size);
        ASSERT(0);
        return false;
    }

    bool get_blob(const char *ns, const char *key, void *data, size_t *size) override
    {
        UNUSED(ns);
        UNUSED(key);
        UNUSED(data);
        UNUSED(size);
        ASSERT(0);
        return false;
    }

    bool erase(const char *ns, const char *key) override
    {
        UNUSED(ns);
        UNUSED(key);
        ASSERT(0);
        return false;
    }

    bool commit() override
    {
        ASSERT(0);
        return false;
    }

    virtual Iter *make_iter() override
    {
        ASSERT(0);
        return 0;
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
