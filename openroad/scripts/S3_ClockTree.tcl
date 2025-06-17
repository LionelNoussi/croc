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
set_wire_rc -clock -layer Metal4
set_wire_rc -signal -layer Metal4
estimate_parasitics -placement
unset_dont_touch $clock_nets
repair_clock_inverters

utl::report "Creating the Clock Tree..."
# clock_tree_synthesis -buf_list $ctsBuf -root_buf $ctsBufRoot -obstruction_aware
clock_tree_synthesis -buf_list $ctsBuf -root_buf $ctsBufRoot -sink_clustering_enable -balance_levels -obstruction_aware
# -sink_clustering_max_diameter 50

utl::report "Report before repair:"
report_cts
report_clock_latency -clock clk_sys
report_design_area
report_power -corner tt
report_checks -path_group clk_sys

utl::report "Repairing design..."
repair_design -verbose
repair_timing -setup -skip_pin_swap -verbose

utl::report "Report after repair:"
set_wire_rc -clock -layer Metal4
set_wire_rc -signal -layer Metal4
estimate_parasitics -placement
report_cts
report_clock_latency -clock clk_sys
report_design_area
report_power -corner tt
report_checks -path_group clk_sys
report_check_types  -violators > reports/croc_w_clock_tree_violations.rpt

save_checkpoint croc_w_clock_tree


gui::show