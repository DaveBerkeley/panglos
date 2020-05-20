
CC = g++
LL = g++
CFLAGS = -Wall -Wextra -Werror -g -I src -I unit-tests -DGTEST=1
LDFLAGS = -pthread -g

# Link with google test
GTEST = /usr/src/gtest/lib
LDFLAGS += $(GTEST)/libgtest_main.a
LDFLAGS += $(GTEST)/libgtest.a

TESTDIR=unit-tests
PANGLOS=src

SRC = 
XSRC = $(TESTDIR)/motor_test.cpp $(TESTDIR)/mock.cpp $(TESTDIR)/hal.cpp \
	   $(TESTDIR)/cli_test.cpp $(TESTDIR)/motor_test.cpp \
	   $(TESTDIR)/printf_test.cpp $(TESTDIR)/buffer_test.cpp \
	   $(TESTDIR)/msg_queue_test.cpp $(TESTDIR)/event_test.cpp \
	   $(TESTDIR)/dispatch_test.cpp $(TESTDIR)/select_test.cpp \
	   $(TESTDIR)/mcp23s17_test.cpp $(TESTDIR)/list_test.cpp \
	   $(TESTDIR)/deque_test.cpp \
	   $(PANGLOS)/motor.cpp $(PANGLOS)/event.cpp $(PANGLOS)/list.cpp \
	   $(PANGLOS)/cli.cpp $(PANGLOS)/sprintf.cpp \
	   $(PANGLOS)/mcp23s17.cpp $(PANGLOS)/spi.cpp \
	   $(PANGLOS)/select.cpp \
	   $(PANGLOS)/dispatch.cpp 

APP = a.out
ODIR = obj
OBJDIR = $(ODIR)/c
XOBJDIR = $(ODIR)/cpp

OBJS = $(SRC:%.c=$(OBJDIR)/%.o)
DEPS = $(SRC:%.c=$(OBJDIR)/%.d)

XOBJS = $(XSRC:%.cpp=$(XOBJDIR)/%.o)
XDEPS = $(XSRC:%.cpp=$(XOBJDIR)/%.d)

MAKEDEPEND  = $(CC) -MM $(CPPFLAGS) -MT $(OBJDIR)/$*.o  -o $(OBJDIR)/$*.d $<
XMAKEDEPEND = $(CC) -MM $(CPPFLAGS) -MT $(XOBJDIR)/$*.o -o $(XOBJDIR)/$*.d $<

.SECONDARY:
	echo "x"

all: $(APP) $(DEPS)

clean:
	rm -r $(ODIR) -f $(APP) html latex

test: $(APP)
	./$(APP)

doxygen: $(APP)
	doxygen

valgrind: $(APP)
	valgrind ./$(APP)

$(APP): $(OBJS) $(XOBJS)
	$(LL) $(LDFLAGS) -o $@ $^

$(OBJDIR)/%.d : %.c Makefile
	@mkdir -p $(dir $(OBJDIR)/$<)
	$(MAKEDEPEND)

$(XOBJDIR)/%.d : %.cpp Makefile
	@mkdir -p $(dir $(XOBJDIR)/$<)
	$(XMAKEDEPEND)

$(OBJDIR)/%.o : %.c $(OBJDIR)/%.d
	$(CC) $(CFLAGS) -c -o $@ $<

$(XOBJDIR)/%.o : %.cpp $(XOBJDIR)/%.d
	$(CC) $(CFLAGS) -c -o $@ $<

-include $(DEPS) 

# FIN
