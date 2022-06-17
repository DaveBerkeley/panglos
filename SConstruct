
files = [
    'src/cli.cpp',
    'src/dispatch.cpp',
    'src/event.cpp',
    'src/list.cpp',
    'src/motor.cpp',
    'src/select.cpp',
    'src/spi.cpp',
    'src/sprintf.cpp',
    'src/mcp23s17.cpp',

    'unit-tests/stubs.cpp',
    'unit-tests/buffer_test.cpp',
    'unit-tests/cli_test.cpp',
    'unit-tests/deque_test.cpp',
    'unit-tests/dispatch_test.cpp',
    'unit-tests/event_test.cpp',
    'unit-tests/hal.cpp',
    'unit-tests/list_test.cpp',
    'unit-tests/mcp23s17_test.cpp',
    'unit-tests/mock.cpp',
    'unit-tests/motor_test.cpp',
    'unit-tests/msg_queue_test.cpp',
    'unit-tests/printf_test.cpp',
    #'unit-tests/rfm12b_test.cpp',
    'unit-tests/select_test.cpp',
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

    '-g',
    '-DGTEST=1',
]

cpppath = [
    'src',
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

env = Environment(CFLAGS=cflags, CCFLAGS=ccflags, CXXFLAGS=cxxflags, LINKFLAGS=lflags, CPPPATH=cpppath)
tdd = env.Program(target='tdd', source=files, LIBS=libs, LIBPATH=libpath)
