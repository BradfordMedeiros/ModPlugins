#!/bin/sh

dotconfig=$(./plugin)
(cd ../../ModEngine/res/tools && ./showscene.sh "$dotconfig")
