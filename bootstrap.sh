#!/bin/bash

# Make use of already present Gnulib headers, if any.
if [ -f "`command -v gnulib-tool`" ]; then
        echo "Found Gnulib headers, continuing . . ."
        gnulib-tool --import human
else
        echo "Unable to find Gnulib headers. Preparing Gnulib submodule . . ."
        git submodule update --init
        ./gnulib/gnulib-tool --import human
fi
