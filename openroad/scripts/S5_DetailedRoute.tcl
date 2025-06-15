# The flows assumes it is beign executed in the openroad/ directory
# but just to be sure, we go there
if {[info script] ne ""} {
    cd "[file dirname [info script]]/../"
}

source scripts/util_scripts/setup.tcl
load_checkpoint croc_fixed_antennas

set_wire_rc -clock -layer Metal4
set_wire_rc -signal -layer Metal4
estimate_parasitics -placement

set_routing_layers -signal Metal2-Metal5 -clock Metal2-Metal5

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
# write_verilog -include_pwr_gnd checkpoints/croc_routed/croc_routed_lvs.v

utl::report "Done!"

gui::show