#!/bin/bash
sudo socat stdio /tmp/qga.sock <<EOF
{"execute":"get-ksymbol", "arguments":{"symbol":"hoge"}}
{"execute":"kmod-addr", "arguments":{"symbol":"hoge"}}
EOF
scp -P 10022 r0ktex@localhost:/tmp/kinfo.kval /tmp/kinfo.val
scp -P 10022 r0ktex@localhost:/tmp/ksym.kval /tmp/ksym.val
