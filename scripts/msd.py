#!/bin/env python

import json
import sys

clients = []
messages = []

for line in sys.stdin:
    line = line.strip()
    j = json.loads(line)
    messages.append(j)
    thread = j['thread']
    if not thread in clients:
        clients.append(thread)

print("msc {")
print("\t", ", ".join(clients), ";")

for msg in messages:
    fn = msg['fn']
    thread = msg['thread']
    client = msg.get('client', '')
    if client and client.startswith('0x'):
        continue

    print(f'\t{thread} box {thread} [ label="{fn}, client={client}" ];')
    #if thread == "server":
    #    print(f'\t{thread} box {thread} [ label="{fn}, client={client}" ];')
    #else:
    #    print(f'\t{thread} box {thread} [ label="{fn}" ];')

print("}")

#   FIN
