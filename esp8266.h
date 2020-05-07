
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
    void process(const uint8_t *cmd);
    void process(uint8_t data);
    void run_command();
    void send_at(const char *cmd);
    void create_rx_buffer(uint8_t *ipd);

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
        const char *name;

        Command(Semaphore *s, const char* at, const char *name)
        : next(0), cmd(at), result(ERR), done(s), name(name)
        {
        }
        virtual ~Command() { }

        virtual void start() { }
        virtual bool process(const uint8_t *line) = 0;
        virtual bool process(uint8_t c) { IGNORE(c); return false; };
    };

    class Hook {
    public:
        virtual void on_command(Command *) = 0;
    };

    Hook *hook;
private:
    List<Command*> commands;
    List<Command*> delete_queue;
    Command *command;
public:
    Buffers buffers;
    bool reading;

    ESP8266(Output *uart, UART::Buffer *b, Semaphore *rd_sem, GPIO *reset);
    ~ESP8266();

    void push_command(Command *cmd);
    void push_delete(Command *cmd);
    void end_command();
    int send(const uint8_t *cmd, int size);

    bool start();
    bool connect_to_ap(const char* ssid, const char *pw);
    int connect(const char *ip, int port);
    int socket_send(int sock, const uint8_t *d, int size);

    Command *read(Semaphore *s, uint8_t *buffer, int len, int *count);
    void cancel(Command *cmd);

    void kill();

    void set_hook(Hook *);
    bool running() { return is_running; };

    void run();
};

}   //  namespace panglos

#endif // __ESP8266_H__

// FIN
