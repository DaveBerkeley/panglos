
#include "panglos/net.h"
#include "panglos/socket.h"

namespace panglos {

class TxFactory : public Client::Factory
{
public:
    virtual Socket *get_socket() = 0;

    static TxFactory *create();
};

}   //  namespace panglos

//      FIN
