# The flows assumes it is beign executed in the openroad/ directory
# but just to be sure, we go there
if {[info script] ne ""} {
    cd "[file dirname [info script]]/../"
}

source scripts/util_scripts/setup.tcl
load_checkpoint croc_placed

# DESIGNING THE CLOCK TREE

utl::report "Starting clock tree synthesis!"
set clock_nets [get_nets -of_objects [get_pins -of_objects "*_reg" -filter "name == CLK"]]
set_wire_rc -clock -layers {Metal2 Metal3 Metal4 Metal5};
set_wire_rc -signal -layers {Metal2 Metal3 Metal4 Metal5};
estimate_parasitics -placement
unset_dont_touch $clock_nets
repair_clock_inverters

utl::report "Creating the Clock Tree..."
configure_cts_characterization -max_cap 2
clock_tree_synthesis -buf_list [ list sg13g2_buf_16 sg13g2_buf_8 sg13g2_buf_4 sg13g2_buf_2 ] \
                     -root_buf $ctsBufRoot \
                     -sink_clustering_enable \
                     -obstruction_aware \
                     -clustering_unbalance_ratio 0.2 \
                     -sink_clustering_size 4 \
                     -sink_clustering_max_diameter 50 \
                     -balance_levels \
                     -sink_clustering_levels 4

repair_clock_nets
detailed_placement
estimate_parasitics -placement

set_propagated_clock [all_clocks]
# report_check_types  -violators > reports/croc_w_clock_tree_violations.rpt
report_metrics R3_croc_clock_tree


utl::report "Report before repair:"
report_cts
report_clock_latency -clock clk_sys
report_design_area
report_power -corner tt
report_checks -path_group clk_sys

utl::report "Repairing design..."
repair_design -verbose -max_wire_length 3070
repair_timing -setup -skip_pin_swap -verbose
repair_timing -hold -skip_pin_swap -verbose
detailed_placement
check_placement -verbose

utl::report "Report after repair:"
estimate_parasitics -placement
report_cts
report_clock_latency -clock clk_sys
report_design_area
report_power -corner tt
report_checks -path_group clk_sys
# report_check_types  -violators > reports/croc_repaired.rpt
report_metrics R4_croc_timing_reparired
puts "Violations after global placement: max_slew:[sta::max_slew_violation_count]  max_fanout:[sta::max_fanout_violation_count]  max_cap:[sta::max_capacitance_violation_count]"

save_checkpoint croc_w_clock_tree


gui::show