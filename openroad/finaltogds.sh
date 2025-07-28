#!/bin/bash

set -e  # Exit on error

# Argument check
if [ $# -ne 1 ]; then
    echo "Usage: $0 <checkpoint-name>"
    exit 1
fi

export LOAD_CHECKPOINT="$1"

# Run TCL script (assuming it's sourced by OpenROAD or similar)
# If you need to call OpenROAD manually:
# openroad -exit scripts/util_scripts/setup.tcl

source scripts/util_scripts/vtogds.tcl

# Continue with other steps
cd klayout
oseda -2024.10 ./def2gds.sh croc_chip ../openroad/out/croc.def

cd ../calibre/lvs
./verilog2spice ../../openroad/out/croc_lvs.v croc_chip.spice
