

# Panglos - Real-time OS framework

Panglos is a set of C++ classes 
designed to be used as a framework to build real-time applications around.

It is written as a wrapper round [FreeRTOS](https://www.freertos.org/) 
and the STM32 HAL. I hope to port it to other platforms.

The intention is to abstract the underlying processor and OS specific code away 
and provide a clean, simple C++ interface.
The core code is intended to be simple, elegant, easy to use, easy to test and easy to read.

The name @ref panglos is after the character Dr. Pangloss in Voltaire's satirical book ['Candide'](https://en.wikipedia.org/wiki/Candide).
It is intended as a warning about boundless optimism.

It was designed using test driven development (TDD), as far as possible.
This is often difficult to do with embedded systems, but by implementing
virtual base classes for primitives like panglos::Semaphore, you can test on a development
system under linux, to a very high level.

I started trying to solve the problem of driving multiple stepper motors. The stepper
code began life as a single threaded program, using blocking waits.
I wanted to drive multiple motors, so had to allow each thread, or task, to have
blocking waits that allowed multiple tasks to co-exist.
I also wanted hgher resolution than the tick time delays generally available on 
cores like FreeRTOS.

So I started with the classes panglos::Mutex, panglos::Semaphore and an Event queue. The panglos::EventQueue class
provides a high resolution timer that is key to high resolution scheduling.
It is implemented using an ordered list of panglos::Event objects, each one of which contains a 
Semaphore. The list is simply waited on, in order, any event that becomes due is signalled.

The single linked list code (@ref list.cpp) is from a C project I wrote a while ago. 
It has the advantage over the std::list class; it does not have to use any dynamic memory allocation.
List items can be static, automatic, dynamic, it does't matter. Items can be in multiple
lists at the same time, as they can have multiple next pointers.

TODO ...
