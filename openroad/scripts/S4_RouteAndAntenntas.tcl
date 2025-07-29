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
global_route -congestion_report_file reports/R5_congestion.rpt -allow_congestion; # -guide_file reports/route_guide1.guide

# Do the following to view the coarse routing grid
# Display Control → Misc → GCell Grid

# Do the following to see the routing congestion
# Heat Maps → Routing Congestion

estimate_parasitics -global_routing

utl::report "Perform buffer insertion..."
repair_design -verbose

utl::report "Repairing the timing..."
repair_timing -skip_pin_swap -setup -setup_margin 0.06 -verbose -repair_tns 100
repair_timing -skip_pin_swap -hold -hold_margin 0.1 -verbose -repair_tns 100

# check_placement -verbose

utl::report "Running incremental global routing..."
global_route -start_incremental -allow_congestion
detailed_placement
global_route -end_incremental -allow_congestion -verbose -guide_file reports/route_guide2.guide

estimate_parasitics -global_routing
save_checkpoint croc_global_route;

report_metrics R6_croc_global_route
puts "Violations after global routing: max_slew:[sta::max_slew_violation_count]  max_fanout:[sta::max_fanout_violation_count]  max_cap:[sta::max_capacitance_violation_count]"
utl::report "Done with GlobalRoute Script!"







utl::report "Checking fixing antennas and doing detailed routing twice!"

utl::report "Repairing Antennas, design and timing..."

utl::report "Running first detailed routing..."
set_global_routing_layer_adjustment Metal2-Metal3 0.30
set_global_routing_layer_adjustment TopMetal1 0.20
set_routing_layers -signal Metal2-TopMetal1 -clock Metal2-TopMetal1

set_thread_count 12;
detailed_route -output_drc reports/croc_route_drc1.rpt \
              -bottom_routing_layer Metal2 \
              -top_routing_layer TopMetal1 \
              -droute_end_iter 30 \
              -drc_report_iter_step 5 \
              -save_guide_updates \
              -clean_patches \
              -verbose 1 \

save_checkpoint croc_one_detailed_route
estimate_parasitics -global_routing;
report_metrics most_accurate

utl::report "Repairing antennas again..."
repair_antennas -ratio_margin 30 -iterations 1;
estimate_parasitics -global_routing;

report_metrics FixedAntennasDetailedRouteIter1

utl::report "Running second detailed routing..."
set_thread_count 12;
detailed_route -output_drc reports/croc_route_drc2.rpt \
              -bottom_routing_layer Metal2 \
              -top_routing_layer TopMetal1 \
              -droute_end_iter 30 \
              -drc_report_iter_step 5 \
              -save_guide_updates \
              -clean_patches \
              -verbose 1 \


# FINISHING

utl::report "Placing filler cells..."
filler_placement {sg13g2_fill_8 sg13g2_fill_4 sg13g2_fill_2 sg13g2_fill_1};
global_connect;

# Final checkpoint
report_metrics R7_croc_final
save_checkpoint croc_final -lvs;

# Final Output
write_verilog out/croc.v
write_db out/croc.db
write_sdc out/croc.sdc
write_def out/croc.def
set stdfill [ list sg13g2_fill_8 sg13g2_fill_4 sg13g2_fill_2 sg13g2_fill_1 ]
write_verilog -include_pwr_gnd -remove_cells "${stdfill} bondpad*" out/croc_lvs.v
write_verilog -remove_cells "${stdfill} bondpad*" out/croc_sta.v


utl::report "Done"
gui::show