#!/bin/bash

#scons
./tdd --gtest_filter="Socket.Test" > /tmp/a.txt

export M="client:='client=(\S*)'"
cat /tmp/a.txt | lex.py -M panglos -m "${M}" -j > /tmp/b.txt
cat /tmp/b.txt | scripts/msd.py > /tmp/a.msc
mscgen -i /tmp/a.msc -T png -o ~/tmp/a.png

#   FIN
