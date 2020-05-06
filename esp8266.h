
#if !defined(__ESP8266_H__)
#define __ESP8266_H__

#include "uart.h"
#include "buffer.h"
#include "list.h"
#include "gpio.h"

namespace panglos {

class ESP8266
{
private:
    Output *uart;
    UART::Buffer *rb;
    Semaphore *rd_sem;
    Semaphore *wait_sem;
    GPIO *gpio_reset;
    Semaphore *cmd_sem;
    uint8_t *buff;
    int in, size;
    bool dead, is_running;
    Mutex *mutex;
    
    void reset();
    void send_at(const char *cmd);
    void process(const uint8_t *cmd);
    void process(uint8_t data);
    void run_command();

public:
    class Command
    {
    public:

        enum Result {
            OK = 0,
            ERR,
        };

        Command *next;
        const char *cmd;
        Result result;
        Semaphore *done;

        Command(Semaphore *s, const char* at)
        : next(0), cmd(at), result(ERR), done(s)
        {
        }
    };

    class Hook {
    public:
        virtual void on_command(Command *) = 0;
    };

    Hook *hook;
private:
    List<Command*> commands;
    Command *command;
public:

    ESP8266(Output *uart, UART::Buffer *b, Semaphore *rd_sem, GPIO *reset);
    ~ESP8266();

    void push_command(Command *cmd);

    bool start();
    bool connect(const char* ssid, const char *pw);
    void kill();

    void set_hook(Hook *);
    bool running() { return is_running; };

    void run();
};

}   //  namespace panglos

#endif // __ESP8266_H__

// FIN
