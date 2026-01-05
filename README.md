

# Panglos - Real-time OS framework

Panglos is a set of C++ classes 
designed to be used as a framework to build real-time applications around.

Panglos has been used as a template for a successful commercial project. 
The techniques used in Panglos were ported to both STM32 and RISC-V targets.
Code could be compiled and run on either target interchangably and unit and system tested on Linux.

The source is available on [github](https://github.com/DaveBerkeley/panglos).

The aim is to remove all the low level OS and HAL calls from the application code.
Ideally application code should not 
include any hardware specific or OS specfic code.
Doing this allows you to write code that can be tested on your PC instead of the target system. This gives you access to excellent tools like 
[valgrind](https://valgrind.org/) or the thread sanitizer capabilities of __clang__. Compiling the code with multiple compilers also gives you better compile time checking. __gcc__ and __clang__ have different strengths and weakneses.

The intention is to abstract the underlying processor and OS specific code away 
and provide a clean, simple C++ interface.
The core code is intended to be simple, elegant, easy to use, easy to test and easy to read.

If the code is littered with HAL and [FreeRTOS][1] calls, it is harder to write unit tests.
By making a simple C++ abstract layer, which can be easily stubbed or mocked,
it becomes possible to use TDD for almost all of the application code.

You can also run the unit tests on the target, for better coverage.

It is written partly as a wrapper around [FreeRTOS][1] 
and the STM32 and ESP32 HALs. 
However, it can be used without an OS.
It makes a very effective bare metal framework.
You can use low level GPIO, UART etc. drivers.
It works well with my [CLI](https://github.com/DaveBerkeley/cli) library.
If you don't need a scheduler, you may not need an OS.

The name @ref panglos is taken from the character Dr. Pangloss in Voltaire's satirical book ['Candide'](https://en.wikipedia.org/wiki/Candide).
It is intended as a warning about boundless optimism.
Optimism in the face of evidence to the contrary is dangerous in software development.
I have seen it kill projects and companies.
It is not __The best of all possible worlds__. It's not even close.

----
OS Abstraction
====

There are a number of OS related classes used to absract the underlying RTOS - so far only FreeRTOS is supported on the target, 
but it is trivial to add others. Linux is supported for test.

Mutex
====

The class panglos::Mutex provides mutex support. In [FreeRTOS][1] there are 
different mutex functions for use in interrupts and tasks. These are hidden by the Mutex class.
There are different sorts of Mutex. 
A task mutex is currently implemented by disabling the scheduler.
This prevents the task being scheduled out by another task.
An interrupt mutex is for use when one of the execution threads is an interrupt.
This is implemented using a critical section.
A recursive Mutex is sometimes needed when the same task can have nested locks on a mutex.
You need to know which to use, which depends on the application. 
eg. panglos::Mutex::create(Mutex::RECURSIVE) 

The class panglos::Lock is used as a simple way of locking / unlocking the mutex
automatically.

    void function()
    {
        Lock lock(mutex);

        ... code protected by Mutex
    }

This works even if the mutex is null (it does nothing).
It doesn't care if it is a critical section or a task lock.
All my mutexes work like this. See mutex.h.

Thread
====

The Thread class allows the management of threads in an OS agnostic way.

Queue
====

The Queue class is a convenient way to communicate between threads, or between interrupt handlers and threads.

Semaphore
====

The Semephore class wraps the underlying RTOS semaphore.

----
Time
====

Time keeping functions are key to most real-time systems. There are a set of utility functions for accessing underlying system data and for sleeping a thread.

        Time::sleep(int secs);
        Time::msleep(int msecs);

A common source of bugs can be timers wrapping. On worked on a customer's code base once where they used 32-bit timers all over the place, with cut and pasted copies of the code. About half had a roll-over bug.

To avoid this problem, always used the panglos function Time::elapsed(tick_t t, tick_t period) to check if a time has expired.

----

Hardware Abstraction
====

Hardware devices should be wrapped by abstract base classes. This allows you to test the calling code without having to run on the target.

There are base classes for eg. GPIO, SPI, I2C, UART, DAC and ADC devices.
These will have target / hardware specific implementations which make real HAL calls,
but the application code can use the high level API.

This allows you to develop higher level drivers, which get passed the IO devices. For example, the DS3231 is an I2C RTC. It is passed an I2C device in its ctor. It implements an RTC interface. There is no RTOS or HAL code in the implementation. So the same driver can be run on any hardware. The application code doesn't care which RTC chip is used.

![RTC class diagram](images/mutex.png)

The diagram shows how the classes interrelate. The init code in main is the only
part of the applicaton that deals with real target issues. It creates a Mutex, a I2C interface,
and a DS3231 RTC object. The I2C class doesn't need to have any OS/HAL dependencies. It uses the Mutex to lock the I2C resource. The DS3231 doesn't have any OS/HAL dependencies. All it needs is the I2C object. The application code just needs the RTC object.

You can have a hardware I2C interface or a bit-banged software I2C interface. The code doesn't care. You just create whatever you need and pass it in the ctor of the DS3231 class.

----
Low level Drivers
====

There are a number of device classes that wrap hardware devices.
When I started developing Panglos I thought I needed to abstract the device creation as well as the devices themselves.
I now realise this is not the best way.
When you are creating the main.cpp code you are probably targetting a specific microcontroller.
It is okay, indeed best, to have target specific creation and initialisation.
Abstracting the differences between different processors turned out to be difficult and unproductive.
So the approach I now take is to have target specific device creation that creates Panglos abstract objects.
You can abstract the whole app code, which also means that you can unit test it very easily.

So, an example project that I've been working on, using the STM32F103, has initialisation that looks like this :

    #include "panglos/debug.h"
    #include "panglos/logger.h"
    #include "panglos/stm32/gpio_f1.h"
    #include "panglos/stm32/uart_f1.h"

    using namespace panglos;

        /*
         *  IO configuration
         */

    #define DEBUG_UART (USART1)
    #define COMMS_UART (USART2)

    GPIO *led;

    typedef STM32F1_GPIO::IO IODEF;

    typedef struct {
        GPIO_TypeDef *port;
        uint32_t pin;
        IODEF io;
        GPIO **gpio;
    } IoDef;

    static const IODEF ALT_IN  = IODEF(STM32F1_GPIO::INPUT  | STM32F1_GPIO::ALT);
    static const IODEF ALT_OUT = IODEF(STM32F1_GPIO::OUTPUT | STM32F1_GPIO::ALT);

    static const IoDef gpios[] = {
        {   GPIOC, 13, STM32F1_GPIO::OUTPUT, & led, }, // LED
        {   GPIOA, 9,  ALT_OUT, }, // UART Tx
        {   GPIOA, 10, ALT_IN,  }, // UART Rx
        {   GPIOA, 2,  ALT_OUT, }, // Comms Tx
        {   GPIOA, 3,  ALT_IN,  }, // Comms Rx
        {   GPIOA, 0,  ALT_IN,  }, // TIM2 counter/timer input
        {   GPIOA, 6,  ALT_OUT, }, // TIM3 phase output
        {   GPIOB, 6,  ALT_OUT, }, // TIM4 triac output
        {   0 },
    };

    void init_gpio(const IoDef *gpios)
    {
        for (const IoDef *def = gpios; def->port; def++)
        {
            STM32F1_GPIO::IO io = def->io;
            if (!def->gpio) io = IODEF(io | STM32F1_GPIO::INIT_ONLY);
            GPIO *gpio = STM32F1_GPIO::create(def->port, def->pin, io);
            if (def->gpio) *def->gpio = gpio;
        }
    }

    ...

    int main(void)
    {
        // initialise all the GPIO pins
        init_gpio(gpios);

        // create the main UART
        UART *uart = STM32F1_UART::create(DEBUG_UART, 32);
        // create a logger
        logger = new Logging(S_DEBUG, 0);
        logger->add(uart, S_DEBUG, 0);

        ...
    }

The panglos::UART and panglos::GPIO devices are abstract. The creation of them is not.
The creation uses the low level #include "stm32f1xx.h" header. 
It doesn't even need the STM32 HAL.

The GPIO initialisation is data driven and all defined in a single table.
The UART can be passed to the logging system and/or the CLI and it just works.

----

Linked Lists
===

The single linked list code (@ref list.cpp) is from a C project I wrote a while ago. 
It has the advantage over the std::list<> class; it does not have to use any dynamic memory allocation.
List items can be a mix of static, automatic, dynamic, it doesn't matter. Items can be in multiple
lists at the same time, as they can have multiple 'next' pointers.

The trick is to provide a function for the object that returns the address of the 'next' pointer.
You need to implement this function for each object type. 
You pass the function into the List<> constructor. The list functions then use it to traverse the list. 
If you have more than one 'next' pointer, you can provide a different function for each pointer.
This allows the same object to potentially be in multiple lists.

I tried for years to come up with a good abstraction of linked lists.
The standard Linux API is horrible (and non portable).
I've seen many efforts to write clean libraries at places I have worked.
None of them was any good.
All had problems and defects.
One was so bad that it drove me to write my own.
When I realised that the key abstraction is the address of the next pointer I was able to write a clean implementation.

    class Thing {
        Thing *next;
        Thing *other;
        int stuff;
    public:
        static Thing **get_next(Thing *thing) { return & thing->next; }
        static Thing **get_other(Thing *thing) { return & thing->other; }
    };

    List<Thing*> things(Thing::get_next);
    List<Thing*> other_things(Thing::get_other);

    ...

    // Add an object to both lists
    Thing *thing = new Thing;
    things.append(thing, mutex);
    other_things.push(thing, mutex);

The advantage of passing the Mutex to the list functions is that it can be used to lock multiple list functions. 
eg. moving an item from one list to the other :

    void fn(int value)
    {
        Lock lock(mutex);

        // the lists are both locked, so you don't need to pass mutex to the list calls
        Thing *thing = things.find(match_fn, & value, 0);
        if (thing)
        {
            other_things.push(thing, 0);
        }
    }

This also removes the need to have a recursive Mutex for this case.

The Mutex pointer can, of course, be null.
If you don't have any threading or interrupts to worry about, you probably don't need locking.
But the code is the same regardless.

----

Tracing / IO
====

I originally wrote my own printf primitives, but later used an available 
[printf](https://github.com/eyalroz/printf) library on GitHub.

I used this library as it provides a __void\*__ context pointer to all its calls. This allows you to use it in C++ OOP.
It is a trick I learned early on : if you have a C API with callbacks, always provide a __void\*__ arg as well. 
Failure to do this limits the capabilities of lots of libraries.

Tracing / logging is really important during development and for diagnostics / monitoring.

        PO_DEBUG("dir=%d speed=%f", wind.direction, wind.speed);

My tracing lib uses a simple ___printf()___ style of formatting. It prints the time (as a tick), the thread name, the severity is 
the same as [syslog](https://en.wikipedia.org/wiki/Syslog) severity levels.
Then it prints the file name, line number and function name, followed by any passed parameters. eg:

    8570648 main DEBUG src/targets/esp32_c3_supermini.cpp +188 on_weather() : dir=296 speed=5.810000

I've seen lots of different approaches to logging in my career.
Many of the ones I've seen have been terrible.
I have tried to make mine as easy to use as possible.
In the simplest case, you simply have an empty format string.

        PO_DEBUG("");

This prints the tick / thread / file / line / function and tells you that the code is being run.

    817 main DEBUG src/cli_server.cpp +48 net_cli_init() :

There is a Logger class that allows one or more logging targets to be specified, so logging can be sent to eg. a UART and a network interface, logged to disk etc.

The path / line number are formatted so that you can cut and paste the line onto the command line and run an editor, eg gvim, with the details and it will open the editor on the offending line.

----
JSON
====

I added a [JSON](https://www.json.org/json-en.html) parser as part of an application I was writing and ended up moving it into the panglos repo. The parser works using callbacks in a Handler class.

The JSON protocol is very simple so parsers are easy to write. This one is quite efficient. It works internally using a Section, which is simply a slice of the input data. The only limitation is that it doesn't work with stream data. You have to load the whole JSON text into memory pefore parsing it.

But I found it useful for many applications.

There is a Match class that can be given a list of paths and handler functions. This makes it easy to extract selected fields from JSON data. There is an example below.

        void on_weather(const char *data, int len)
        {
            json::Section sec(data, & data[len-1]);
            json::Match match;

            const char *dir_keys[] = { "wind", "deg", 0 };
            const char *speed_keys[] = { "wind", "speed", 0 };

            wind.direction = 0;
            wind.speed = 0.0;

            json::Match::Item direction = {
                .keys = dir_keys,
                .on_match = on_dir,
                .arg = & wind,
            };
            json::Match::Item speed = {
                .keys = speed_keys,
                .on_match = on_speed,
                .arg = & wind,
            };

            match.add_item(& direction);
            match.add_item(& speed);

            json::Parser parser(& match);
            parser.parse(& sec);

            PO_DEBUG("dir=%d speed=%f", wind.direction, wind.speed);
        }

----

work in progress ...

[1]: https://www.freertos.org/        "FreeRTOS"
