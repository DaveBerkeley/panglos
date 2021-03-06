

# Panglos - Real-time OS framework

Panglos is a set of C++ classes 
designed to be used as a framework to build real-time applications around.

The source is available on [github](https://github.com/DaveBerkeley/panglos).

It is written as a wrapper around [FreeRTOS][1] 
and the STM32 HAL. I hope to port it to other platforms.

The intention is to abstract the underlying processor and OS specific code away 
and provide a clean, simple C++ interface.
The core code is intended to be simple, elegant, easy to use, easy to test and easy to read.

The name @ref panglos is taken from the character Dr. Pangloss in Voltaire's satirical book ['Candide'](https://en.wikipedia.org/wiki/Candide).
It is intended as a warning about boundless optimism.
Optimism in the face of evidence to the contrary is dangerous in software development.
I have seen it kill projects and companies.
It is not __The best of all possible worlds__. It's not even close.

----

The key classes are :

* hardware IO : panglos::GPIO, panglos::UART, panglos::SPI (and panglos::SpiDevice)
* high resolution timer : panglos::Event
* task / scheduling control : panglos::Mutex, panglos::Semaphore
* task communication : panglos::MsgQueue, panglos::Select, panglos::Dispatch
* miscellaneous : panglos::CLI, debug.h

----

It was designed using test driven development (TDD), as far as possible.
This is often difficult to do with embedded systems, but by implementing
virtual base classes for primitives like panglos::Semaphore, you can test on a development
system under linux, to a very high level.
If the code is littered with HAL and [FreeRTOS][1] calls, it is harder to write unit tests.
By making a simple C++ abstract layer, which can be easily stubbed or mocked,
it becomes possible to use TDD for most of the application code.
The unit tests are in the subdirectory 'unit-tests'.

I started trying to solve the problem of driving multiple stepper motors. The stepper
code began life as a single threaded program, using blocking waits, running on a different processor.
I wanted to drive multiple motors, so had to allow each thread, or task, to have
blocking waits that allowed multiple tasks to co-exist.
I also wanted higher resolution than the tick time delays generally available on 
an RTOS like [FreeRTOS][1].
I used classes I'd written for a previous project, panglos::MotorIo and panglos::Stepper.

----

I started with the classes panglos::Mutex, panglos::Semaphore. In [FreeRTOS][1] there are 
different functions for use in interrupts and tasks. These are hidden by the Semaphore class.
There are two sorts of Mutex. 
A task mutex is currently implemented by disabling the scheduler.
This prevents the task being scheduled out by another task.
An interrupt mutex is for use when one of the execution threads is an interrupt.
This is implemented using a critical section.
You need to know which to use, which depends on the application. 
See panglos::Mutex::create() and panglos::Mutex::create_critical_section().
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

----

Next I implemented an Event queue. The panglos::EventQueue class
provides a high resolution timer that is key to high resolution scheduling.
It is implemented using an ordered list of panglos::Event objects, each one of which contains a 
Semaphore. The list is simply waited on, in sequence. 
Any event that becomes due is signalled and removed from the event list.

The single linked list code (@ref list.cpp) is from a C project I wrote a while ago. 
It has the advantage over the std::list<> class; it does not have to use any dynamic memory allocation.
List items can be a mix of static, automatic, dynamic, it doesn't matter. Items can be in multiple
lists at the same time, as they can have multiple 'next' pointers.

The trick is to provide a function for the object that returns the address of the 'next' pointer.
You need to implement this function for each object type. You pass the function into the list_xxx()
functions which use it to traverse the list. If you have more than one 'next' pointer, you can provide
a different function for each pointer.
This allows the same object to potentially be in multiple lists.
I may write a similar double ended linked list to replace the use of std::deque<> in panglos::MsgQueue.

----

The other core classes are panglos::MsgQueue and panglos::Select, which provide a thread-safe queue
and a multiple Semaphore select (similar to posix [select()](https://linux.die.net/man/3/select)). 
I started by using the [FreeRTOS][1] QueueSet functions, which are horrible and I couldn't get them to work
reliably. I then wrote panglos::Select, which is a very simple MsgQueue with a hook to the Semaphore object.
This is simple, fast, portable (no underlying OS dependency), and easy to test.
It demonstrates that with the right API you can make powerful functions very simply.
See [select.cpp](https://github.com/DaveBerkeley/panglos/blob/master/select.cpp).

I added a panglos::Dispatch class to allow actions to be passed from one task to another,
or possibly from an interrupt to a task.
Dispatch uses a panglos::Dispatch::Callback object, added to a MsgQueue, to be executed in the Dispatch task.
This allows, for example, an interrupt to push a Callback to the dispatch task,
where the action can be executed.
An example would be where an SPI interrupt needs to be responded to, but we don't want
to execute long-running operations in the interrupt context. These can be passed to the dispatch
task for execution.
It can be used to implement a simple event handler, which could be used at the heart of
an application to process messages from other tasks or interrupts. 
It uses the same simple building blocks of panglos::Semaphore, panglos::Mutex and panglos::MsgQueue.
See dispatch.h.

----

On the hardware side I needed a panglos::GPIO class. This is a simple wrapper for hardware IO functions.
It also supports interrupt callbacks.

Several years ago I bought some [MCP23S17](https://www.microchip.com/wwwproducts/en/MCP23S17) 
16-Bit SPI I/O Expander chips, intending to use them on projects using small micros with little IO.
This meant I had to add the panglos::SPI and panglos::SpiDevice classes. Because the code uses just my 
panglos primitives, I could unit-test the driver without needing any hardware present.
I added a method on the panglos::MCP23S17 class to create panglos::GPIO objects - which look the same
as the standard hardware panglos::GPIO, but work through the SPI interface.
They also support interrupts in the same way as the hardware GPIO class.

In the application code, it doesn't matter if you have a hardware GPIO or a serial expander GPIO.
If you were using the HAL to talk to the IO, you would have to rewrite your code to use different IO.
And it would be difficult to unit-test it.

----

To aid debugging I added some code to produce formatted output. The base code is in sprintf.cpp. 
It provides a simple subset of [printf()](https://linux.die.net/man/3/printf) formatting.
It uses the panglos::Output class, which is implemented by the panglos::UART class.
This allows powerful debugging traces to be used, see debug.h.
I've always found that printf debugging and logging is incredibly useful during development,
giving insight into the running of a program.

work in progress ...

[1]: https://www.freertos.org/        "FreeRTOS"
