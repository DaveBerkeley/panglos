
#include "gtest/gtest.h"

#include "panglos/debug.h"
#include "panglos/io.h"

#include "panglos/flash_storage.h"

using namespace panglos;

#if defined(ARCH_LINUX)

class FlashOut : public panglos::Out
{
    virtual int tx(const char* data, int n) override
    {
        char buff[n+1];
        strncpy(buff, data, n);
        buff[n] = '\0';
        printf("%s", buff);
        return n;
    }
};

FlashOut fo;
panglos::Out *log_uart = & fo;

#else

extern panglos::Out *log_uart;

#endif

TEST(Flash, Test)
{
    bool sim_flash = true;
    size_t size = 128; // 1024 * 4;
    Page *p1 = Page::create_ram((Page::PhyAddr) 0x10000000, size, sim_flash);
    Page *p2 = Page::create_ram((Page::PhyAddr) 0x10000000, size, sim_flash);
    FlashStorage flash(p1, p2);

    //p1->dump(log_uart);
 
    const char *group = "app";
    const char *group2 = "longer string";
    const char *item = "item";
    const char *item2 = "item2";
    const char *item3 = "item3";
    const char *item4 = "item4";

    FlashStorage::PageAddr gaddr;
    FlashStorage::PageAddr gaddr2;
    FlashStorage::PageAddr iaddr;
    FlashStorage::PageAddr iaddr2;
    bool ok;

    gaddr = flash.find_group(group);
    EXPECT_EQ(gaddr, 0);

    ok = flash.write_group(group);
    EXPECT_TRUE(ok);

    gaddr = flash.find_group(group);
    EXPECT_NE(gaddr, 0);

    iaddr = flash.find_item(gaddr, item);
    EXPECT_EQ(iaddr, 0);

    uint32_t data = 0x12345678;
    iaddr = flash.write_item(gaddr, item, FlashStorage::U32, (const uint8_t*) & data);
    EXPECT_TRUE(iaddr);

    iaddr = flash.find_item(gaddr, item);
    EXPECT_NE(iaddr, 0);

    iaddr = flash.find_item(gaddr, item2);
    EXPECT_EQ(iaddr, 0);

    uint16_t data2 = 0x3456;
    iaddr = flash.write_item(gaddr, item2, FlashStorage::U16, (const uint8_t*) & data2);
    EXPECT_TRUE(iaddr);

    gaddr2 = flash.find_group(group2);
    EXPECT_EQ(gaddr2, 0);

    ok = flash.write_group(group2);
    EXPECT_TRUE(ok);

    gaddr2 = flash.find_group(group2);
    EXPECT_NE(gaddr2, 0);

    gaddr = flash.find_group(group);
    EXPECT_NE(gaddr, 0);

    uint16_t data3 = 0xabcd;
    iaddr = flash.write_item(gaddr2, item3, FlashStorage::U16, (const uint8_t*) & data3);
    EXPECT_TRUE(iaddr);

    iaddr = flash.find_item(gaddr2, item3);
    EXPECT_NE(iaddr, 0);

    // overwrite the data in data3
    data3 &= 0xff0f;
    iaddr2 = flash.write_item(gaddr2, item3, FlashStorage::U16, (const uint8_t*) & data3);
    EXPECT_NE(iaddr2, 0);
    // check that the addresses are the same
    EXPECT_EQ(iaddr, iaddr2);

    // overwrite the data in data3
    data3 &= 0xf00f;
    iaddr2 = flash.write_item(gaddr2, item3, FlashStorage::U16, (const uint8_t*) & data3);
    EXPECT_NE(iaddr2, 0);
    // check that the addresses are the same
    EXPECT_EQ(iaddr, iaddr2);
    size_t used = flash.get_total_used();

    // should need new data for data3
    data3 = 0x1234;
    iaddr2 = flash.write_item(gaddr2, item3, FlashStorage::U16, (const uint8_t*) & data3);
    EXPECT_NE(iaddr2, 0);
    // check that the addresses are different
    EXPECT_NE(iaddr, iaddr2);
    // the additional item should replace the eariler one, but take the same space
    EXPECT_EQ(flash.get_total_used(), used);

    // should need new data for data3, this will cause an alloc fail
    uint16_t data4 = 0xdbdb;
    iaddr2 = flash.write_item(gaddr2, item4, FlashStorage::U16, (const uint8_t*) & data4);
    EXPECT_EQ(iaddr2, 0);

    EXPECT_EQ(flash.get_error(0), Page::ALLOC);
    EXPECT_EQ(flash.get_error(1), Page::NONE);

    //p1->dump(log_uart);

    delete p1;
    delete p2;
}

    /*
     *
     */

class _VisitorFind : public FlashStorage::Visitor
{
public:
    typedef FlashStorage::PageAddr Addr;

    const char *group;
    const char *item;
    bool in_group;
    bool found;
    int count;

    virtual bool on_group(Addr addr, const char *name) override
    {
        UNUSED(addr);        
        if (!group) return false;
        return in_group = !strcmp(name, group);
    }

    virtual bool skip_group(const char *name) override
    {
        if (!group) return false;
        return strcmp(name, group);
    }

    virtual bool skip_item(const char *name) override
    {
        if (!item) return false;
        return strcmp(name, item);
    }

    virtual bool on_item(Addr addr, const char *name, FlashStorage::Type ty, const uint8_t* data, size_t size) override
    {
        count += 1;
        if (!item) return false;
        EXPECT_TRUE(addr);
        EXPECT_TRUE(in_group);
        if (!in_group) return false;
        if (strcmp(name, item)) return false;
        // FOUND!
        UNUSED(ty);
        UNUSED(data);
        UNUSED(size);
        //PO_DEBUG("%s.%s %d %p %d", group, name, (int) ty, data, (int) size);
        return found = true;
    }

    _VisitorFind(const char *g, const char *i)
    :   group(g),
        item(i),
        in_group(false),
        found(false),
        count(0)
    {
    }
};

    /*
     *
     */

static int num_items(FlashStorage *flash)
{
    _VisitorFind visitor(0, 0);
    bool ok = flash->visit(0, & visitor);
    EXPECT_TRUE(ok);
    return visitor.count;
}

static void write_items(FlashStorage *flash)
{
    typedef FlashStorage::PageAddr Addr;
    Addr app, more, empty, addr;

    int n = num_items(flash);
    EXPECT_EQ(n, 0);

    app = flash->write_group("app");
    EXPECT_TRUE(app);

    n = num_items(flash);
    EXPECT_EQ(n, 0);

    uint16_t d16 = 0x1234;
    addr = flash->write_item(app, "d16", FlashStorage::U16, (const uint8_t*) & d16);
    EXPECT_TRUE(addr);
 
    n = num_items(flash);
    EXPECT_EQ(n, 1);

    uint32_t d32 = 0x12345678;
    addr = flash->write_item(app, "d32", FlashStorage::U32, (const uint8_t*) & d32);
    EXPECT_TRUE(addr);
 
    n = num_items(flash);
    EXPECT_EQ(n, 2);

    d32 = 0x23456789;
    addr = flash->write_item(app, "d32", FlashStorage::U32, (const uint8_t*) & d32);
    EXPECT_TRUE(addr);
 
    n = num_items(flash);
    EXPECT_EQ(n, 2);

    d16 = 0x2345;
    addr = flash->write_item(app, "d16", FlashStorage::U16, (const uint8_t*) & d16);
    EXPECT_TRUE(addr);
 
    n = num_items(flash);
    EXPECT_EQ(n, 2);

    empty = flash->write_group("empty");
    EXPECT_TRUE(empty);

    n = num_items(flash);
    EXPECT_EQ(n, 2);

    more = flash->write_group("more");
    EXPECT_TRUE(more);

    n = num_items(flash);
    EXPECT_EQ(n, 2);

    const char *more_data = "more data";
    addr = flash->write_item(more, "d16", FlashStorage::STR, (const uint8_t*) more_data);
    EXPECT_TRUE(addr);

    n = num_items(flash);
    EXPECT_EQ(n, 3);
}

TEST(Flash, Visitor)
{
    bool sim_flash = true;
    size_t size = 1024 * 4;
    Page *p1 = Page::create_ram((Page::PhyAddr) 0x10000000, size, sim_flash);
    Page *p2 = Page::create_ram((Page::PhyAddr) 0x10010000, size, sim_flash);
    FlashStorage flash(p1, p2);

    write_items(& flash);

    bool ok;

    {
        _VisitorFind visitor("more", "d16");
        ok = flash.visit(0, & visitor);
        EXPECT_TRUE(visitor.found);
        EXPECT_TRUE(ok);
    }

    {
        _VisitorFind visitor("not there", "d16");
        ok = flash.visit(0, & visitor);
        EXPECT_FALSE(visitor.found);
        EXPECT_TRUE(ok);
    }

    flash.copy_to_other();

    delete p1;
    delete p2;
}

    /*
     *
     */

TEST(Flash, DelGroup)
{
    bool sim_flash = true;
    size_t size = 1024 * 4;
    Page *p1 = Page::create_ram((Page::PhyAddr) 0x10000000, size, sim_flash);
    Page *p2 = Page::create_ram((Page::PhyAddr) 0x10010000, size, sim_flash);
    FlashStorage flash(p1, p2);

    typedef FlashStorage::PageAddr Addr;
    Addr app, empty, more;

    write_items(& flash);

    app = flash.find_group("app");
    EXPECT_TRUE(app);
    empty = flash.find_group("empty");
    EXPECT_TRUE(empty);
    more = flash.find_group("more");
    EXPECT_TRUE(more);

    bool ok = flash.delete_group(empty);
    EXPECT_TRUE(ok);

    app = flash.find_group("app");
    EXPECT_TRUE(app);
    empty = flash.find_group("empty");
    EXPECT_FALSE(empty);
    more = flash.find_group("more");
    EXPECT_TRUE(more);

    ok = flash.delete_group(more);
    EXPECT_TRUE(ok);

    app = flash.find_group("app");
    EXPECT_TRUE(app);
    empty = flash.find_group("empty");
    EXPECT_FALSE(empty);
    more = flash.find_group("more");
    EXPECT_FALSE(more);

    ok = flash.delete_group(app);
    EXPECT_TRUE(ok);

    app = flash.find_group("app");
    EXPECT_FALSE(app);
    empty = flash.find_group("empty");
    EXPECT_FALSE(empty);
    more = flash.find_group("more");
    EXPECT_FALSE(more);

    size_t used = flash.get_total_used();
    EXPECT_EQ(used, sizeof(FlashStorage::Header));

    delete p1;
    delete p2;
}

    /*
     *
     */

TEST(Flash, DelItem)
{
    bool sim_flash = true;
    size_t size = 1024 * 4;
    Page *p1 = Page::create_ram((Page::PhyAddr) 0x10000000, size, sim_flash);
    Page *p2 = Page::create_ram((Page::PhyAddr) 0x10010000, size, sim_flash);
    FlashStorage flash(p1, p2);

    typedef FlashStorage::PageAddr Addr;
    Addr group, item;
    bool ok;
    int n;
    
    write_items(& flash);

    n = num_items(& flash);
    EXPECT_EQ(n, 3);
    
    group = flash.find_group("app");
    EXPECT_TRUE(group);
    
    // check null pointer works
    item = flash.find_item(group, 0);
    EXPECT_FALSE(item);

    item = flash.find_item(group, "d32");
    EXPECT_TRUE(item);
    ok = flash.delete_item(item);
    EXPECT_TRUE(ok);
    item = flash.find_item(group, "d32");
    EXPECT_FALSE(item);

    n = num_items(& flash);
    EXPECT_EQ(n, 2);

    item = flash.find_item(group, "d16");
    EXPECT_TRUE(item);
    ok = flash.delete_item(item);
    EXPECT_TRUE(ok);
    item = flash.find_item(group, "d16");
    EXPECT_FALSE(item);

    n = num_items(& flash);
    EXPECT_EQ(n, 1);

    group = flash.find_group("more");
    EXPECT_TRUE(group);
 
    item = flash.find_item(group, "d16");
    EXPECT_TRUE(item);
    ok = flash.delete_item(item);
    EXPECT_TRUE(ok);
    item = flash.find_item(group, "d16");
    EXPECT_FALSE(item);

    // all items are now gone
    n = num_items(& flash);
    EXPECT_EQ(n, 0);

    // the groups are still there. delete them too

    const char *groups[] = {
        "app", "more", "empty", 0,
    };

    for (const char **name = groups; *name; name++)
    {
        group = flash.find_group(*name);
        EXPECT_TRUE(group);
        ok = flash.delete_group(group);
        EXPECT_TRUE(ok);
        group = flash.find_group(*name);
        EXPECT_FALSE(group);
    } 

    //p1->dump(log_uart);

    delete p1;
    delete p2;
}

    /*
     *
     */

TEST(Flash, Storage)
{
}

// Allows linker command to force inclusion of the tests in this module
void force_test_flash_storage(void){}

//  FIN
