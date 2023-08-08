#!/bin/bash
starver SL21b

# root4star -l -b -q $STAR/StRoot/macros/mudst/genDst.C\(-1,\"picoDst,PicoVtxMode:PicoVtxVpdOrDefault,PicoCovMtxMode:PicoCovMtxWrite\",\"$1\"\)

root4star -l -b -q genDst.C\(-1,\"picoDst,PicoVtxMode:PicoVtxVpdOrDefault,PicoCovMtxMode:PicoCovMtxWrite\",\"$1\"\)

#root4star -l -b -q makePicoDst.C\(\"$1\"\)

