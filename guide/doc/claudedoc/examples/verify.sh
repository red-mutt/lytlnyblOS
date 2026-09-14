#!/bin/bash
# Build every example from scratch and report. Used to make sure the
# examples still compile after any change to the OS they overlay.
cd "$(dirname "$0")"
fail=0
for d in ch*/; do
    d=${d%/}
    printf "%-24s " "$d"
    ( cd "$d" && make clean >/dev/null 2>&1 && make build ) > /tmp/lyt-verify.log 2>&1
    if [ $? -eq 0 ] && [ -f "$d/build/os/build/kernel.img" ]; then
        echo "OK   ($(stat -c%s "$d/build/os/build/kernel.bin") bytes of kernel)"
    else
        echo "FAILED"
        sed -n '/[Ee]rror/p' /tmp/lyt-verify.log | head -3
        fail=1
    fi
done
echo
[ $fail -eq 0 ] && echo "all examples build" || echo "SOME EXAMPLES FAILED"
exit $fail
