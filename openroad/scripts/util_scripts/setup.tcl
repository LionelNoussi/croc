# Copyright 2024 ETH Zurich and University of Bologna.
# Solderpad Hardware License, Version 0.51, see LICENSE for details.
# SPDX-License-Identifier: SHL-0.51
#
# Authors:
# - Tim Fischer <fischeti@iis.ee.ethz.ch>


if {[info script] ne ""} {
    cd "[file dirname [info script]]/../../"
}

set proj_name "croc_chip"
set report_dir "reports"
set save_dir "checkpoints"
set step_by_step_debug 0

utl::report "Setting up project $proj_name"
utl::report " - Report directory: $report_dir"
utl::report " - Save directory: $save_dir"

# helper scripts
# source scripts/old_scripts/reports.tcl
source scripts/util_scripts/checkpoint.tcl

# initialize technology data
source scripts/util_scripts/init_tech.tcl
