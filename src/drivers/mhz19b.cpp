
#include "panglos/debug.h"

#include "panglos/drivers/mhz19b.h"

namespace panglos {

#define PACKET 9

MHZ19B::MHZ19B(UART *_uart)
:   uart(_uart)
{
    ASSERT(uart);
}

uint8_t MHZ19B::checksum(const uint8_t *data)
{
    char total = 0;
    for (size_t i = 1; i < PACKET-1; i++)
    {
        total += (char) data[i];
    }
    total = char(0xff - total);
    total += 1;
    return total;
}

bool MHZ19B::request()
{
    uint8_t packet[PACKET] = { 0 };

    packet[0] = START;
    packet[1] = 1;
    packet[2] = READ;

    packet[PACKET-1] = checksum(packet);

    int sent = uart->tx((const char *) packet, sizeof(packet));
    return sent == sizeof(packet);
}

bool MHZ19B::read(struct Data *data)
{
    uint8_t packet[PACKET];

    int rx = uart->rx((char *) packet, sizeof(packet));
    if (rx != sizeof(packet))
    {
        return false;
    }

    if (!checksum(packet))
    {
        return false;
    }

    if (packet[0] != START)
    {
        return false;
    }

    if (packet[1] != READ)
    {
        return false;
    }

    ASSERT(data);
    data->co2 = packet[2] << 8;
    data->co2 += packet[3];

    return true;
}

}   //  namespace panglos

//  FIN
