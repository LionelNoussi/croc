# The flows assumes it is beign executed in the openroad/ directory
# but just to be sure, we go there
if {[info script] ne ""} {
    cd "[file dirname [info script]]/../"
}

source scripts/util_scripts/setup.tcl
load_checkpoint croc_global_route

set_wire_rc -clock -layer Metal4;
set_wire_rc -signal -layers {Metal2 Metal3 Metal4 Metal5};
estimate_parasitics -global_routing

set_global_routing_layer_adjustment Metal2-Metal3 0.30
set_global_routing_layer_adjustment TopMetal1 0.20
set_routing_layers -signal Metal2-TopMetal1 -clock Metal2-TopMetal1

utl::report "Running detailed routing..."
set_thread_count 6;
detailed_route -output_drc reports/croc_route_drc.rpt \
              -bottom_routing_layer Metal2 \
              -top_routing_layer TopMetal1 \
              -droute_end_iter 30 \
              -drc_report_iter_step 5 \
              -save_guide_updates \
              -clean_patches \
              -verbose 1;

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

# Write and read back parasitics, and create simple netlist for sta
define_process_corner -ext_model_index 0 X
extract_parasitics -ext_model_file IHP_rcx_patterns.rules
write_spef out/croc.spef
read_spef  out/croc.spef; # readback parasitics makes things a lot worse

utl::report "Done!"

gui::show