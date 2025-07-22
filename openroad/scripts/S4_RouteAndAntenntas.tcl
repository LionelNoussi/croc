# The flows assumes it is beign executed in the openroad/ directory
# but just to be sure, we go there
if {[info script] ne ""} {
    cd "[file dirname [info script]]/../"
}

source scripts/util_scripts/setup.tcl
load_checkpoint croc_w_clock_tree

set_wire_rc -clock -layers {Metal2 Metal3 Metal4 Metal5};
set_wire_rc -signal -layers {Metal2 Metal3 Metal4 Metal5};
estimate_parasitics -placement

set_global_routing_layer_adjustment Metal2-Metal3 0.30
set_global_routing_layer_adjustment TopMetal1 0.20
set_routing_layers -signal Metal2-TopMetal1 -clock Metal2-TopMetal1

utl::report "Running Initial Global Routing..."
global_route -congestion_report_file reports/congestion.rpt -allow_congestion -guide_file reports/route_guide1.guide

# Do the following to view the coarse routing grid
# Display Control → Misc → GCell Grid

# Do the following to see the routing congestion
# Heat Maps → Routing Congestion

estimate_parasitics -global_routing

utl::report "Perform buffer insertion..."
repair_design -verbose

utl::report "Repairing the timing..."
repair_timing -skip_pin_swap -setup -setup_margin 0.01 -verbose 
repair_timing -skip_pin_swap -hold -hold_margin 0.1 -verbose -repair_tns 100

# check_placement -verbose

utl::report "Running incremental global routing..."
global_route -start_incremental -allow_congestion
detailed_placement
global_route -end_incremental -allow_congestion -verbose -guide_file reports/route_guide2.guide

estimate_parasitics -global_routing

# utl::report "Repairing Antennas..."
# repair_antennas -ratio_margin 30 -iterations 5 ;
check_antennas;
report_check_types  -violators > reports/croc_w_global_route.rpt
puts "Violations after global placement: max_slew:[sta::max_slew_violation_count]  max_fanout:[sta::max_fanout_violation_count]  max_cap:[sta::max_capacitance_violation_count]"

utl::report "Done!"
save_checkpoint croc_fixed_antennas;
gui::show
