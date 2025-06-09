# The flows assumes it is beign executed in the openroad/ directory
# but just to be sure, we go there
if {[info script] ne ""} {
    cd "[file dirname [info script]]/../"
}

source scripts/util_scripts/setup.tcl
load_checkpoint croc_w_clock_tree

set_wire_rc -clock -layer Metal4
set_wire_rc -signal -layer Metal4
estimate_parasitics -placement

set_routing_layers -signal Metal2-Metal5 -clock Metal2-Metal5

utl::report "Running Initial Global Routing..."
global_route -congestion_report_file reports/congestion.rpt -allow_congestion

# Do the following to view the coarse routing grid
# Display Control → Misc → GCell Grid

# Do the following to see the routing congestion
# Heat Maps → Routing Congestion

utl::report "Repairing the timing..."
estimate_parasitics -global_routing

# repair_timing -setup -repair_tns 100
# repair_timing -hold -hold_margin 0.05 -repair_tns 100
repair_timing -skip_pin_swap -setup -verbose -repair_tns 100
repair_timing -skip_pin_swap -hold -hold_margin 0.1 -verbose -repair_tns 100

# check_placement -verbose

utl::report "Running incremental global routing..."
global_route -start_incremental
detailed_placement
global_route -end_incremental -allow_congestion -verbose

estimate_parasitics -global_routing

utl::report "Repairing Antennas..."
repair_antennas -iterations 5 ; # repair_antennas
check_antennas;

utl::report "Running detailed routing..."
set_thread_count 6;
detailed_route -output_drc reports/croc_route_drc.rpt \
              -bottom_routing_layer Metal2 \
              -top_routing_layer Metal5 \
              -droute_end_iter 10 \
              -clean_patches \
              -verbose 1;

utl::report "Placing filler cells..."
filler_placement {sg13g2_fill_8 sg13g2_fill_4 sg13g2_fill_2 sg13g2_fill_1};
global_connect;

save_checkpoint croc_routed -lvs;
write_verilog -include_pwr_gnd checkpoints/croc_routed/croc_routed_lvs.v

utl::report "Done!"

gui::show