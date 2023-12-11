
#include <string.h>

#include "panglos/debug.h"

#include "cli/src/cli.h"

#include "panglos/io.h"
#include "panglos/socket.h"
#include "panglos/net.h"
#include "panglos/logger.h"
#include "panglos/cli_net.h"

namespace panglos {

    /*
     *
     */

class SocketOut : public panglos::Out
{
public:
    Socket *sock;

    virtual int tx(const char* data, int n) override
    {
        ASSERT(sock);
        return sock->send((const uint8_t*) data, n);
    }

    SocketOut() : sock(0) { }
};

    /*
     *
     */

static void net_exit(CLI *, CliCommand *);
static void net_log(CLI *, CliCommand *);

class CliClient : public Client
{
    CliCommand *commands;
    CLI cli;
    SocketOut out;
    FmtOut fmt_out;
    CliOutput cli_out;
    CliCommand cmd_exit;
    CliCommand cmd_log;
    bool dead;
    bool logging;

    void init_cli()
    {
        cmd_exit.cmd = "exit";
        cmd_exit.handler = net_exit;
        cmd_exit.help = "terminate session";
        cmd_exit.next = commands;
        cmd_exit.ctx = this;

        cmd_log.cmd = "log";
        cmd_log.handler = net_log;
        cmd_log.help = "enable log to output";
        cmd_log.next = & cmd_exit;
        cmd_log.ctx = this;

        cli_out.fprintf = FmtOut::xprintf;
        cli_out.ctx = & fmt_out;
        cli.output = & cli_out;
        cli.eol = "\r\n";
        cli.prompt = "> ";
        cli_init(& cli, 96, 0);
        cli.head = & cmd_log;
    }

public:
    CliClient(SocketServer *ss, CliCommand *_commands)
    :   Client(ss),
        commands(_commands),
        fmt_out(& out, 0),
        dead(false),
        logging(false)
    {
        PO_DEBUG("");
        memset(& cli, 0, sizeof(cli));
        memset(& cli_out, 0, sizeof(cli_out));
        memset(& cmd_exit, 0, sizeof(cmd_exit));
        memset(& cmd_log, 0, sizeof(cmd_log));
    }

    virtual void run() override
    {
        PO_DEBUG("");
        ASSERT(sock);

        out.sock = sock;
        init_cli();

        // read chars from socket, feed to the CLI
        while (!dead)
        {
            uint8_t buff[32];

            const int n = sock->recv(buff, sizeof(buff));

            if (n == -1)
            {
                break;
            }

            for (int i = 0; i < n; i++)
            {
                cli_process(& cli, buff[i]);
            }
        }

        if (logging)
        {
            logger->remove(& out);
            logging = false;
        }
    }

    void exit()
    {
        dead = true;
    }

    void log()
    {
        PO_DEBUG("");
        if (logging)
        {
            return;
        }
        logger->add(& out, S_DEBUG, 0);
        logging = true;
    }
};

static void net_exit(CLI *, CliCommand *cmd)
{
    CliClient *cc = (CliClient *) cmd->ctx;
    cc->exit();
}

static void net_log(CLI *, CliCommand *cmd)
{
    CliClient *cc = (CliClient *) cmd->ctx;
    cc->log();
}

    /*
     *
     */

class Factory : public Client::Factory
{
    CliCommand *commands;

    virtual Client *create_client(SocketServer *ss) override
    {
        return new CliClient(ss, commands);
    }
public:
    Factory(CliCommand *c) : commands(c) { }
};

    /*
     *
     */

void cli_server(struct CliServer *cs)
{
    Socket *socket = Socket::open_tcpip(cs->host, cs->port, Socket::SERVER);
    Factory factory(cs->commands);
    run_socket_server(socket, & factory);
    delete socket;
}

}   //  namespace panglos

//  FIN
