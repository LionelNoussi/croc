# ------------------------------------------------------------------
# THIS SCRIPT DOES FLOORPLANNING, MACRO PLACEMENT AND PDN GENERATION
# ------------------------------------------------------------------

# The flows assumes it is beign executed in the openroad/ directory
# but just to be sure, we go there
if {[info script] ne ""} {
    cd "[file dirname [info script]]/../"
}

# Setup and reading in the netlist
source scripts/util_scripts/setup.tcl
read_verilog ../yosys/out/croc.v
link_design croc_chip

source scripts/helper_scripts/floorplanning.tcl
makeTracks
source scripts/helper_scripts/pdn_generation.tcl

save_checkpoint croc_floorplanned

gui::show