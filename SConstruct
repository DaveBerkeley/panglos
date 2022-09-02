
files = [
    'src/dispatch.cpp',
    'src/event.cpp',
    'src/list.cpp',
    'src/time.cpp',
    'src/object.cpp',
    'src/vcd.cpp',
    'src/io.cpp',
    'src/logger.cpp',
    'src/thread.cpp',
    'src/device.cpp',
    'src/action.cpp',

    'src/drivers/i2c_bitbang.cpp',
    'src/drivers/keyboard.cpp',
    'src/drivers/spi.cpp',
    'src/drivers/mcp23s17.cpp',
    'src/drivers/motor.cpp',
    'src/drivers/aht25.cpp',

    # for test environment
    'src/linux/arch.cpp',
    'src/linux/thread.cpp',
    'src/linux/mutex.cpp',
    'src/linux/semaphore.cpp',
    'src/linux/queue.cpp',
    'src/linux/time.cpp',

    # https://github.com/eyalroz/printf
    'lib/printf/src/printf/printf.c',

    'unit-tests/stubs.cpp',
    'unit-tests/buffer_test.cpp',
    'unit-tests/deque_test.cpp',
    'unit-tests/dispatch_test.cpp',
    'unit-tests/event_test.cpp',
    'unit-tests/hal.cpp',
    'unit-tests/list_test.cpp',
    'unit-tests/mcp23s17_test.cpp',
    'unit-tests/mock.cpp',
    'unit-tests/motor_test.cpp',
    'unit-tests/msg_queue_test.cpp',
    'unit-tests/time.cpp',
    'unit-tests/object.cpp',
    'unit-tests/i2c.cpp',
    'unit-tests/logger.cpp',
    'unit-tests/io.cpp',
    'unit-tests/device.cpp',
    'unit-tests/action.cpp',
    'unit-tests/aht25.cpp',
    'unit-tests/queue.cpp',
    'unit-tests/thread.cpp',
]

ccflags = [
    '-Wswitch-default',
    #'-Wswitch-enum',
    '-Wconversion',
    #'-Wsign-conversion',
    '-Wunused-parameter',

    '-Wno-missing-field-initializers',
    '-Wno-format-zero-length',
    '-Wformat-security',

    # clang
    '-Wno-implicit-int-float-conversion',

    '-g',
    '-DGTEST=1',
    '-DARCH_LINUX=1',
]

cpppath = [
    'src',
    'lib/printf/src',
]

cflags = [
    '-Wall',
    '-Wextra',
    '-Werror',
]

cxxflags = [
    '-std=c++11',
    '-Wall',
    '-Wextra',
    '-Werror',
]

lflags = [
    '-lgtest_main',
    '-lgtest',
    '-lpthread',
]

libpath = [ ]

libs = []

import os
environ = os.environ
cc = environ.get('CC', 'gcc')
cxx = environ.get('CXX', 'g++')

if cc == 'clang':
    sane = [
        '-fsanitize=thread',
        '-fsanitize=alignment',
        '-fno-sanitize-recover=all',
    ]
    for x in sane:
        cflags  += [ x ]
        ccflags += [ x ]
        lflags  += [ x ]

#
#

env = Environment(CFLAGS=cflags, CCFLAGS=ccflags, CXXFLAGS=cxxflags, LINKFLAGS=lflags, CPPPATH=cpppath, CC=cc, CXX=cxx)
tdd = env.Program(target='tdd', source=files, LIBS=libs, LIBPATH=libpath)

# FIN
