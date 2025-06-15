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

utl::report "Creating Floorplan..."
source scripts/helper_scripts/floorplanning.tcl
makeTracks

utl::report "Starting PDN generation..."
source scripts/helper_scripts/pdn_generation.tcl

utl::report "Finished!"
save_checkpoint croc_floorplanned

gui::show