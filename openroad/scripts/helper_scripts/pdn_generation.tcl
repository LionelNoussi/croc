##########################################################################
# PDN Generation
##########################################################################

# std cells
add_global_connection -net {VDD} -inst_pattern {.*} -pin_pattern {VDD} -power
add_global_connection -net {VSS} -inst_pattern {.*} -pin_pattern {VSS} -ground
# pads
add_global_connection -net {VDD} -inst_pattern {.*} -pin_pattern {vdd} -power
add_global_connection -net {VSS} -inst_pattern {.*} -pin_pattern {vss} -ground
# fix for bondpad/port naming
add_global_connection -net {VDDIO} -inst_pattern {.*} -pin_pattern {.*vdd_RING} -power
add_global_connection -net {VSSIO} -inst_pattern {.*} -pin_pattern {.*vss_RING} -ground
# rams
add_global_connection -net {VDD} -inst_pattern {.*} -pin_pattern {VDDARRAY} -power
add_global_connection -net {VDD} -inst_pattern {.*} -pin_pattern {VDDARRAY!} -power
add_global_connection -net {VDD} -inst_pattern {.*} -pin_pattern {VDD!} -power
add_global_connection -net {VSS} -inst_pattern {.*} -pin_pattern {VSS!} -ground

# pads
add_global_connection -net {VDDIO} -inst_pattern {.*} -pin_pattern {iovdd} -power
add_global_connection -net {VSSIO} -inst_pattern {.*} -pin_pattern {iovss} -ground
# fix for bondpad/port naming
add_global_connection -net {VDDIO} -inst_pattern {.*} -pin_pattern {.*iovdd_RING} -power
add_global_connection -net {VSSIO} -inst_pattern {.*} -pin_pattern {.*iovss_RING} -ground

global_connect

# Create the voltage domain
set_voltage_domain -name {CORE} -power {VDD} -ground {VSS}

# set some useful variables
set macro RM_IHPSG13_1P_256x64_c2_bm_bist
set sram  [[ord::get_db] findMaster $macro]
set sramHeight  [ord::dbu_to_microns [$sram getHeight]]
set stripe_dist [expr $sramHeight - 50]
if {$stripe_dist > 100} {set stripe_dist [expr $stripe_dist/2]}

# DESIGN THE CORE POWER GRIDS

define_pdn_grid -name {core_grid} -voltage_domains {CORE}

add_pdn_ring -grid {core_grid}   \
   -layer        {TopMetal1 TopMetal2}       \
   -widths       "10 10"                     \
   -spacings     "6 6"                       \
   -pad_offsets  "6 6"                       \
   -add_connect                              \
   -connect_to_pads                          \
   -connect_to_pad_layers TopMetal2
   
add_pdn_stripe -grid {core_grid} \
  -layer {Metal1}                            \
  -width {0.44}                              \
  -offset {0}                                \
  -followpins                                \
  -extend_to_core_ring

add_pdn_stripe -grid {core_grid} -layer {TopMetal2} -width 6 \
               -pitch 204 -spacing 60 -offset 60 \
               -extend_to_core_ring -snap_to_grid -number_of_straps 9

add_pdn_connect -grid {core_grid} -layers {Metal1 TopMetal2}


# SRAM POWER

define_pdn_grid -macro \
   -cells $macro \
   -name sram_256x64_grid \
   -orient "R0 R180 MY MX" \
   -grid_over_boundary \
   -voltage_domains {CORE} \
   -halo {1 1}

add_pdn_ring -grid sram_256x64_grid \
   -layer        {Metal3 Metal4} \
   -widths       "2 2" \
   -spacings     "0.6 0.6" \
   -core_offsets "2.4 0.6" \
   -add_connect

add_pdn_stripe -grid sram_256x64_grid -layer {TopMetal1} -width 6 -spacing 4 \
               -pitch $stripe_dist -offset 20 -extend_to_core_ring -starts_with POWER -snap_to_grid

add_pdn_connect -grid sram_256x64_grid -layers {Metal4 TopMetal1}
add_pdn_connect -grid sram_256x64_grid -layers {Metal3 TopMetal1}
add_pdn_connect -grid sram_256x64_grid -layers {TopMetal1 TopMetal2}

pdngen -failed_via_report reports/croc_pdngen.rpt