if {[info script] ne ""} {
    cd "[file dirname [info script]]/../"
}

source scripts/util_scripts/setup.tcl
load_checkpoint croc_routed

set_wire_rc -clock -layer Metal4
set_wire_rc -signal -layer Metal4
estimate_parasitics -placement

set extRules ./src/IHP_rcx_patterns.rulesa
define_process_corner -ext_model_index 0 tt
extract_parasitics -ext_model_file $extRules
write_spef ./checkpoints/croc_final/croc.spef

utl::report "Static power report"
set_power_activity -input -activity 0.1
set_power_activity -input_port rst_ni -activity 0
report_power -corner tt
report_power -corner ff

utl::report "Dynamic power report"

# Load the VCD file and define the simulation scope
read_vcd -scope tb_croc_soc/i_croc_soc ../vsim/croc.vcd
report_power -corner tt;  # Finally, you can generate the VCD-based power report

analyze_power_grid -vsrc src/Vsrc_croc_vdd.loc -net VDD -corner tt

utl::report "IR Drop"

set_pdnsim_net_voltage -net VDD -voltage 1.2
analyze_power_grid -vsrc src/Vsrc_croc_vdd.loc -net VDD -corner tt

# set_pdnsim_net_voltage -net VSS -voltage 0
# analyze_power_grid -vsrc src/Vsrc_croc_vss.loc -net VSS -corner tt

gui::set_display_controls "Heat Maps/IR Drop" visible true
gui::set_heatmap IRDrop Layer Metal1
gui::set_heatmap IRDrop ShowLegend 1

save_checkpoint croc_final

gui::show