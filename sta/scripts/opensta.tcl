# Copyright (c) 2025 ETH Zurich and University of Bologna.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0
#
# OpenSTA script template for VLSI-2 EX04           
#                                               
# Author: Philippe Sauter <phsauter@iis.ee.ethz.ch>
#         Bowen Wang      <bowwang@iis.ee.ethz.ch>
#         Enrico Zelioli  <ezelioli@iis.ee.ethz.ch>  
#
# Last Modification: 19.02.2025

set netlist_name $::env(YS_NETLIST)
puts "Loading netlist: yosys/out/${netlist_name}.v"

# Read library files
set lib_dir "../technology/lib"
read_liberty ${lib_dir}/sg13g2_stdcell_typ_1p20V_25C.lib
read_liberty ${lib_dir}/RM_IHPSG13_1P_256x64_c2_bm_bist_typ_1p20V_25C.lib
read_liberty ${lib_dir}/sg13g2_io_typ_1p2V_3p3V_25C.lib

# Load netlist
read_verilog ../yosys/out/${netlist_name}.v
link_design croc_chip

# Set constraints
create_clock -name clk_sys -period 10 [get_ports clk_i]

# Generate timing reports

# Report setup violations
report_checks -path_group clk_sys -path_delay max > "reports/sta_setup_${netlist_name}.rpt"

# Report hold violations
report_checks -path_group clk_sys -path_delay min > "reports/sta_hold_${netlist_name}.rpt"


# Print the final slack to the console.
# -------------------------------------

# Helper proc to parse slack and status from a report file
proc parse_slack_status {filename} {
    set fileId [open $filename r]
    set slack ""
    set status ""

    while {[gets $fileId line] >= 0} {
        # Match lines like: "    -0.36   slack (VIOLATED)" or " 0.36 slack (MET)"
        if {[regexp {^\s*([-+]?[0-9]*\.?[0-9]+)\s+slack\s+\((\w+)\)} $line -> val stat]} {
            set slack $val
            set status $stat
            break
        }
    }
    close $fileId
    return [list $slack $status]
}

# Parse setup slack and status
set setup_report "reports/sta_setup_${netlist_name}.rpt"
set setup_info [parse_slack_status $setup_report]
set setup_slack [lindex $setup_info 0]
set setup_status [lindex $setup_info 1]

if {$setup_slack != ""} {
    puts "Setup Slack = $setup_slack ns ($setup_status)"
} else {
    puts "Failed to parse setup slack"
}

# Parse hold slack and status
set hold_report "reports/sta_hold_${netlist_name}.rpt"
set hold_info [parse_slack_status $hold_report]
set hold_slack [lindex $hold_info 0]
set hold_status [lindex $hold_info 1]

if {$hold_slack != ""} {
    puts "Hold Slack = $hold_slack ns ($hold_status)"
} else {
    puts "Failed to parse hold slack"
}

exit

