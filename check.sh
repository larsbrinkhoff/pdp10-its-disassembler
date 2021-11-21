#!/bin/sh

set -e

compare() {
    if cmp out/"$1" test/"$1"; then
        echo "OK: $1"
    else
        echo "FAIL: $1"
    fi
}

test_dis10() {
    opts="$2"
    ./dis10 ${opts:--Wits} samples/"$1" > out/"$1".dasm
    compare "$1.dasm"
}

test_itsarc() {
    ./itsarc -t samples/"$1" 2> out/"$1".list
    compare "$1.list"
}

test_ipak() {
    ./ipak -t -Wascii samples/"$1" 2> out/"$1".ipak
    compare "$1.ipak"
}

test_dart() {
    ./dart -x9f samples/"$1" -C.tmp.
    (cd .tmp.; ../dart -c7 reg/1/*) | ./dart -t7 | sed 's/RECORDED ....-..-.. ..:..,/RECORDED XXXX-XX-XX XX:XX/' > out/"$1".dart
    rm -rf .tmp.
    compare "$1.dart"
}

test_scrmbl() {
    ./scrmbl -Wbin "$1" samples/zeros.scrmbl out/"$1".scrmbl
    ./cat36 -Wits -Xbin out/"$1".scrmbl | cmp - samples/zeros."$1".scrmbl || \
        echo "FAIL: $1.scrmbl"
    ./scrmbl -d -Wits "$1" out/"$1".scrmbl out/"$1".unscrm
    ./cat36 -Wits -Xbin out/"$1".unscrm | cmp - samples/zeros.scrmbl || \
        echo "FAIL: $1.scrmbl"
    echo "OK: $1.scrmbl"
}

test_cat36() {
    ./cat36 -W"$2" -X"$3" samples/"$1"."$2" > out/"$1"."$2"."$3"
    compare "$1"."$2"."$3"
}

test_dump() {
    ./dump $2 samples/"$1" > out/"$1".dump 2> /dev/null
    compare "$1.dump"
}

test_dis10 ts.obs
test_dis10 ts.ksfedr
test_dis10 ts.name        "-Sall"
test_dis10 ts.srccom
test_dis10 atsign.tcp
test_dis10 macro.low      "-r -Wascii"
test_dis10 pt.rim         "-Frim10 -Wpt -mka10its"
test_dis10 visib1.bin     "-Sddt"
test_dis10 visib2.bin     "-Sddt"
test_dis10 visib3.bin     "-Sall"
test_dis10 @.midas        "-D774000 -Sall"
test_dis10 srccom.exe     "-mka10 -Wascii"
test_dis10 dart.dmp       "-6 -mka10sail -Wdata8"
test_dis10 @.its          "-Sall -mka10_its"
test_dis10 its.bin        "-mkl10_its"
test_dis10 its.rp06       "-mks10_its"
test_dis10 system.dmp     "-Fdmp -Woct -mka10sail -Sall"
test_dis10 dired.dmp      "-6 -mka10sail -Wascii -Sddt"
test_dis10 two.tapes      "-r -Wtape"

test_itsarc arc.code
test_ipak stink.-ipak-
test_dart dart.tape

test_scrmbl thirty
test_scrmbl sixbit
test_scrmbl pdpten
test_scrmbl aaaaaa
test_scrmbl 0s

test_cat36 chars.pub oct sail
test_cat36 chars.pub sail ascii

test_dump pt.rim      "-Frim10 -Wpt -Osblk"
test_dump system.dmp  "-Fdmp -Woct -Xoct -Odmp"
test_dump ts.srccom   "-Wits -Opdump"
test_dump l.bin       "-Fpalx -Xoct -Oraw"
test_dump logo.ptp    "-Fhex -Xoct -Oraw"

exit 0
