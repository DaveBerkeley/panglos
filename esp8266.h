
#if !defined(__ESP8266_H__)
#define __ESP8266_H__

#include "uart.h"
#include "buffer.h"
#include "list.h"
#include "gpio.h"

namespace panglos {

class ESP8266;

class Radio
{
public:
    class Command;

    virtual void request_command(Command *) = 0;
    virtual void end_command(Command *, Semaphore *) = 0;
    virtual int write(const uint8_t *data, int size) = 0;
    virtual int read(uint8_t *data, int size) = 0;
};

class ESP8266 : public Radio
{
private:
    Output *uart;
    UART::Buffer *rb;
    // three types of event: rd_data from uart, timeout, new_command
    Semaphore *rd_sem;
    Semaphore *wait_sem;
    Semaphore *cmd_sem;

    GPIO *gpio_reset;

    uint8_t *buff;
    int in, size;

    bool dead, is_running;
    Mutex *mutex;
    
    void reset();
    void process(const uint8_t *cmd);
    void process(uint8_t data);
    void run_command();
    void send_at(const char *cmd);
    void create_rx_buffer(const uint8_t *ipd);

public:
    typedef Radio::Command Command;

    // Hook for unit tests
    class Hook
    {
    public:
        virtual ~Hook() { }
        virtual void on_command(Command *cmd) = 0;
    };
    Hook *hook;
    void set_hook(Hook *h) { hook = h; }

private:
    List<Command*> commands;
    List<Command*> delete_queue;
    Command *command;
public:
    // store for rx data via "+IPD,<length>:" message
    Buffers buffers;
    bool reading;

    ESP8266(Output *uart, UART::Buffer *b, Semaphore *rd_sem, GPIO *reset);
    ~ESP8266();

    // implement Radio interface
    virtual void request_command(Command *);
    virtual void end_command(Command *, Semaphore *);
    virtual int write(const uint8_t *data, int size);
    virtual int read(uint8_t *data, int size);

    bool start();
    bool connect_to_ap(const char* ssid, const char *pw);
    int connect(const char *ip, int port);
    int socket_send(int sock, const uint8_t *d, int size);

    Command *read(Semaphore *s, uint8_t *buffer, int len, int *count);
    void cancel(Command *cmd);

    void run();

    void kill();
    bool running() { return is_running; };
};

}   //  namespace panglos

#endif // __ESP8266_H__

// FIN
