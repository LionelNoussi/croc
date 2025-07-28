source scripts/util_scripts/setup.tcl
load_checkpoint $::env(LOAD_CHECKPOINT)


write_verilog out/croc.v
write_db out/croc.db
write_sdc out/croc.sdc
write_def out/croc.def
set stdfill [ list sg13g2_fill_8 sg13g2_fill_4 sg13g2_fill_2 sg13g2_fill_1 ]
write_verilog -include_pwr_gnd -remove_cells "${stdfill} bondpad*" out/croc_lvs.v
write_verilog -remove_cells "${stdfill} bondpad*" out/croc_sta.v