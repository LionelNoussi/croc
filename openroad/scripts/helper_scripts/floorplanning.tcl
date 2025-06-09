####################################################################
############# FLOORPLANNING SCRIPT #################################
####################################################################

# Some useful functions for floorplaning

# place_macro only allows R0, R180, MX, MY
# Example: placeInstance $sram 25 50 R90
proc placeInstance { name x y orient } {
  puts "placing $name at {$x $y} $orient"

  set block [ord::get_db_block]
  set inst [$block findInst $name]
  if {$inst == "NULL"} {
    error "Cannot find instance $name"
  }

  $inst setLocationOrient $orient
  $inst setLocation [ord::microns_to_dbu $x] [ord::microns_to_dbu $y]
  $inst setPlacementStatus FIRM
}

# Define chip dimensions
set chipW            2235;    # 2235 with bondpads
set chipH            2235;    # 2235 with bondpads

set padRing           180.0
set bondpad           70
set coreMargin [expr $padRing + $bondpad + 35]

# Initialize Floorplan
initialize_floorplan -die_area "0 0 $chipW $chipH" \
                    -core_area "$coreMargin $coreMargin [expr $chipW-$coreMargin] [expr $chipH-$coreMargin]" \
                    -site "CoreSite"

# Place IO pins nad pad ring
source scripts/helper_scripts/pin_placement.tcl

# Place macros
set bank0_sram0 {i_croc_soc/i_croc/gen_sram_bank\[0\].i_sram/gen_512x32xBx1.i_cut}
set bank1_sram0 {i_croc_soc/i_croc/gen_sram_bank\[1\].i_sram/gen_512x32xBx1.i_cut}
set coreArea      [ord::get_core_area]
set core_leftX    [lindex $coreArea 0]
set core_bottomY  [lindex $coreArea 1]
set core_rightX   [lindex $coreArea 2]
set core_topY     [lindex $coreArea 3]

set floorPaddingX      20
set floorPaddingY      20
set floor_leftX       [expr $core_leftX + $floorPaddingX]
set floor_bottomY     [expr $core_bottomY + $floorPaddingY]
set floor_rightX      [expr $core_rightX - $floorPaddingX]
set floor_topY        [expr $core_topY - $floorPaddingY]
set floor_midpointX   [expr $floor_leftX + ($floor_rightX - $floor_leftX)/2]
set floor_midpointY   [expr $floor_bottomY + ($floor_topY - $floor_bottomY)/2]

set RamMaster256x64   [[ord::get_db] findMaster "RM_IHPSG13_1P_256x64_c2_bm_bist"]
set RamSize256x64_W   [ord::dbu_to_microns [$RamMaster256x64 getWidth]]
set RamSize256x64_H   [ord::dbu_to_microns [$RamMaster256x64 getHeight]]

placeInstance $bank0_sram0 [expr $floor_midpointX - $RamSize256x64_W / 2] [expr $floor_topY - $RamSize256x64_H * 1] R0
placeInstance $bank1_sram0 [expr $floor_midpointX - $RamSize256x64_W / 2] [expr $floor_topY - $RamSize256x64_H * 2.25] R0

cut_rows -halo_width_x 2 -halo_width_y 1