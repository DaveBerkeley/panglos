
#include <string.h>

#include "panglos/cli.h"

namespace panglos {

  /*
  *
  */

void CLI::reset()
{
    memset(command, 0, sizeof(command));
    match = 0;
    memset(values, 0, sizeof(values));
    idx = 0;
    num_valid = false;
    negative = false;
    cursor = 0;
}

void CLI::run()
{
    if (match)
    {
        match->fn(match, idx, values);
    }
}

CLI::CLI(const char *delimit)
:   actions(0),
    delim(delimit),
    err_fn(0)
{
    reset();
}

void CLI::add_action(Action *action)
{
    action->next = actions;
    actions = action;
}

void CLI::set_error_fn(ErrorFn fn)
{
    err_fn = fn;
}

void CLI::error(enum Error err)
{
    if (err_fn)
    {
        err_fn(match, cursor, err);
    }
}

void CLI::apply_sign()
{
    if (negative)
    {
        values[idx] = -values[idx];
        negative = false;
    }
}

bool CLI::match_action(Action **a)
{
    for (Action *action = actions; action; action = action->next)
    {
        if (!strncmp(action->cmd, command, idx))
        {
            if (idx == (int) strlen(action->cmd))
            {
                *a = action;
            }
            return true;
        }
    }
    return false;
}

void CLI::process(char c)
{
    cursor += 1;

    // process any completed line
    if (c == '\r')
    {
        if (num_valid)
        {
            apply_sign();
            // make sure that the trailing value gets counted
            idx += 1;
        }
        if (idx > 0)
        {
            if (!num_valid)
            {
                error(ERR_NUM_EXPECTED);
                reset();
                return;
            }
        }
        run();
        reset();
        return;
    }

    if (c == '\n')
    {
        // just ignore '\n'
        reset();
        return;
    }

    // check if the char is a valid command
    if (!match)
    {
        // add char to cmd buffer
        command[idx++] = c;

        if (idx >= MAX_CMD)
        {
            error(ERR_TOO_LONG);
            reset();
            return;
        }

        Action *a = 0;
        if (match_action(& a))
        {
            if (a)
            {
                idx = 0;
                match = a;
            }
            return;
        }

        error(ERR_UNKNOWN_CMD);
        reset();
        return;
    }

    // handle numeric values
    if ((c >= '0') && (c <= '9'))
    {
        // numeric
        values[idx] *= 10;
        values[idx] += c - '0';
        num_valid = true;
        return;
    }

    // handle '-'/'+' for sign
    if ((c == '-') || (c == '+'))
    {
        if (!num_valid)
        {
            negative = c == '-';
        }
        else
        {
            error(ERR_BAD_SIGN);
            reset();
        }
        return;
    }

    // check for delimiter
    if (strchr(delim, c))
    {
        if (num_valid)
        {
            apply_sign();
            idx += 1;
        }

        num_valid = false;

        if (idx >= MAX_VALUES)
        {
            error(ERR_TOO_LONG);
            reset();
        }
        return;
    }

    // the char doesn't match any known options
    error(ERR_BAD_CHAR);
    reset();
}

void CLI::process(const char *line)
{
    while (*line)
    {
        process(*line++);
    }
}

}   //  namespace panglos

// FIN
