# THIS SCRIPT DOES PLACEMENT, TIMING, CLOCK TREE AND ROUTING

# The flows assumes it is beign executed in the openroad/ directory
# but just to be sure, we go there
if {[info script] ne ""} {
    cd "[file dirname [info script]]/../"
}

source scripts/util_scripts/setup.tcl
load_checkpoint croc_floorplanned

read_sdc src/constraints.sdc

set_wire_rc -clock -layer Metal4
set_wire_rc -signal -layer Metal4
estimate_parasitics -placement

set_thread_count 8
global_placement -density 0.6

report_cell_usage
report_design_area
puts "Violations after global placement: max_slew:[sta::max_slew_violation_count]  max_fanout:[sta::max_fanout_violation_count]  max_cap:[sta::max_capacitance_violation_count]"
report_check_types  -violators > reports/drv_global_placement.rpt


set_wire_rc -clock -layer Metal4
set_wire_rc -signal -layer Metal4
estimate_parasitics -placement

repair_design -verbose

report_cell_usage
puts "Violations after repair: max_slew:[sta::max_slew_violation_count]  max_fanout:[sta::max_fanout_violation_count]  max_cap:[sta::max_capacitance_violation_count]"
report_check_types  -violators > reports/drv_global_placement_repaired.rpt

detailed_placement
save_checkpoint croc_chip_placed

gui::show