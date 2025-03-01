
#include <string.h>
#include <stdio.h>

#include "panglos/debug.h"
#include "panglos/list.h"

#include "panglos/vcd.h"

    /*
     * see https://zipcpu.com/blog/2017/07/31/vcd.html
     * https://en.wikipedia.org/wiki/Value_change_dump
     */

namespace panglos {

class VcdWriter::Trace
{
public:
    Trace *next;
    const char *name;
    int width;
    bool state;
    char id;
};

static VcdWriter::Trace **trace_next(VcdWriter::Trace *trace)
{
    return & trace->next;
}

static int trace_match(VcdWriter::Trace *trace, void *arg)
{
    ASSERT(trace);
    ASSERT(arg);
    const char *name = (const char*) arg;
    return strcmp(name, trace->name) == 0;
}

    /*
     *
     */

VcdWriter::VcdWriter(const char *_path, const char *_sr_path)
:   file(0),
    close_file(0),
    id('!'),
    time(0),
    sr_path(_sr_path ? _sr_path : ""),
    traces(trace_next)
{
    if (_path)
    {
        path = _path;
        close_file = file = fopen(_path, "w");
    }
    else
    {
        file = stdout;
    }
}

VcdWriter::~VcdWriter()
{
    close();

    if (!sr_path.empty())
    {
        sigrok_write(sr_path.c_str());
    }

    // Remove all the traces
    while (true)
    {
        Trace *trace = traces.pop(0);
        if (!trace)
        {
            break;
        }
        delete trace;
    }
}

void VcdWriter::close()
{
    // terminate the trace
    if (file)
    {
        fprintf(file, "#%d\n", time+100);
    }

    if (close_file)
    {
        fclose(close_file);
        close_file = 0;
        file = 0;
    }
}

bool VcdWriter::sigrok_write(const char *sr_path)
{
    ASSERT(sr_path);
    close();

    char cmd[64];
    snprintf(cmd, sizeof(cmd), "sigrok-cli -I vcd -i %s -o %s", path.c_str(), sr_path);
    PO_DEBUG("calling '%s'", cmd);
    const int err = system(cmd);
    if (err < 0)
    {
        PO_ERROR("sigrok error %d '%s'", errno, strerror(errno));
        return false;
    }
    return true;
}

void VcdWriter::print(Trace *t, bool state)
{
    fprintf(file, "#%d\n", time);
    fprintf(file, "%d%c\n", state, t->id);
}

void VcdWriter::tick()
{
    time += 1000;
}

VcdWriter::Trace *VcdWriter::find(const char *name)
{
    return traces.find(trace_match, (void*) name, 0);
}

void VcdWriter::write_header()
{
    fprintf(file, "$version Generated by panglos $end\n");
    fprintf(file, "$date Sat Jun  11 12:00:00 2022\n"); // TODO
    fprintf(file, " $end\n");
    fprintf(file, "$timescale 1ns $end\n");

    // Print signals
    fprintf(file, "$scope module TOP $end\n");
    for (Trace *t = traces.head; t; t = t->next)
    {
        // $var vary_type size identifier_code reference $end
        fprintf(file, "$var wire %d %c %s $end\n", t->width, t->id, t->name);            
    }
    fprintf(file, "$upscope $end\n");
    fprintf(file, "$enddefinitions $end\n");

    // print initial state
    fprintf(file, "#0\n");
    fprintf(file, "$dumpvars\n");
    for (Trace *t = traces.head; t; t = t->next)
    {
        print(t, t->state);
    }    
    fprintf(file, "$end\n");
}

void VcdWriter::add(const char *name, bool state, int width)
{
    Trace *trace = new Trace;
    trace->next = 0;
    trace->name = name;
    trace->state = state;
    trace->width = width;
    trace->id = id++;
    traces.push(trace, 0);
}

void VcdWriter::set(const char *name, bool state)
{
    Trace *t = find(name);
    ASSERT(t);

    print(t, state);
}

}   //  namespace panglos

//  FIN
