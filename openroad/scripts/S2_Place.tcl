# THIS SCRIPT DOES PLACEMENT, TIMING, CLOCK TREE AND ROUTING

# The flows assumes it is beign executed in the openroad/ directory
# but just to be sure, we go there
if {[info script] ne ""} {
    cd "[file dirname [info script]]/../"
}

source scripts/util_scripts/setup.tcl
load_checkpoint croc_floorplanned

utl::report "Loading constraints..."
read_sdc src/constraints.sdc

utl::report "Setting parasitics..."
set_wire_rc -clock -layers {Metal2 Metal3 Metal4 Metal5};
set_wire_rc -signal -layers {Metal2 Metal3 Metal4 Metal5};

# don't touch any clock-tree related nets as
# repair_timing can insert a 'split0000' buffer which then prevents CTS from running
set clock_nets [get_nets -of_objects [get_pins -of_objects "*_reg" -filter "name == CLK"]]
set_dont_touch $clock_nets
set_dont_use $dont_use_cells

utl::report "Repair tie fanout"
repair_tie_fanout sg13g2_tielo/L_LO
repair_tie_fanout sg13g2_tiehi/L_HI

utl::report "Remove buffers"
remove_buffers

utl::report "Repair design"
repair_design -verbose

utl::report "Starting global placement..."
set_thread_count 8
global_placement -density 0.60 \
                -routability_driven \
                -routability_check_overflow 0.30 \
                -timing_driven
estimate_parasitics -placement

utl::report "Done with global placement. Reporting usage and violations:"
report_cell_usage
report_design_area
puts "Violations after global placement: max_slew:[sta::max_slew_violation_count]  max_fanout:[sta::max_fanout_violation_count]  max_cap:[sta::max_capacitance_violation_count]"

utl::report "Setting parasitics..."

utl::report "Repairing design..."
repair_design -verbose -max_wire_length 3070
repair_timing -setup -skip_pin_swap -verbose

utl::report "Running second Global placement"
global_placement -density 0.60 \
                -routability_driven \
                -routability_check_overflow 0.30 \
                -timing_driven

utl::report "Done repairing design. Reporting usage and violations again:"
estimate_parasitics -placement
report_cell_usage
puts "Violations after repair: max_slew:[sta::max_slew_violation_count]  max_fanout:[sta::max_fanout_violation_count]  max_cap:[sta::max_capacitance_violation_count]"

utl::report "Starting detailed placement..."
detailed_placement
optimize_mirroring
check_placement

estimate_parasitics -placement

report_check_types  -violators > reports/drv_placement.rpt

utl::report "Finished!"
save_checkpoint croc_placed

gui::show