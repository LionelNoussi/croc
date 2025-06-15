#################
#    IO Site    #
#################

make_io_sites -horizontal_site sg13g2_ioSite \
    -vertical_site sg13g2_ioSite \
    -corner_site sg13g2_ioSite \
    -offset 70 \
    -rotation_horizontal R0 \
    -rotation_vertical R0 \
    -rotation_corner R0

#################
# Pad Placement #
#################

set chipW 2135 ;    # 2235 with sealring spacing
set chipH 2135 ;    # 2235 with sealring spacing

set bondpadW 70;
set bondpadH 70;

set padW 80 ;
set padD 180 ;

set IO_row_w 1735 ;
set IO_row_h 1735 ;

set numPads 16 ;
# set offset 62.5 ;
set offset 12.5
set pitch 102 ;

# set startLeft [expr 1985 - $offset - $padW]
# set startBottom [expr 250 + $offset]
# set startRight [expr 250 + $offset]
# set startTop [expr 1985 - $offset - $padW]
set startLeft [expr $chipH - $bondpadH - $padD - $offset - $padW]
set startBottom [expr $bondpadW + $padD + $offset]
set startRight [expr $bondpadH + $padD + $offset]
set startTop [expr $chipW - $bondpadW - $padD - $offset - $padW]

# Edge: LEFT (top to bottom)

place_pad -row IO_WEST  -location [expr $startLeft -  0*$pitch] "pad_vssio0"       ; # pin no:  1
place_pad -row IO_WEST  -location [expr $startLeft -  1*$pitch] "pad_vddio0"       ; # pin no:  2
place_pad -row IO_WEST  -location [expr $startLeft -  2*$pitch] "pad_uart_rx_i"    ; # pin no:  3
place_pad -row IO_WEST  -location [expr $startLeft -  3*$pitch] "pad_uart_tx_o"    ; # pin no:  4
place_pad -row IO_WEST  -location [expr $startLeft -  4*$pitch] "pad_fetch_en_i"   ; # pin no:  5
place_pad -row IO_WEST  -location [expr $startLeft -  5*$pitch] "pad_status_o"     ; # pin no:  6
place_pad -row IO_WEST  -location [expr $startLeft -  6*$pitch] "pad_clk_i"        ; # pin no:  7
place_pad -row IO_WEST  -location [expr $startLeft -  7*$pitch] "pad_ref_clk_i"    ; # pin no:  8
place_pad -row IO_WEST  -location [expr $startLeft -  8*$pitch] "pad_rst_ni"       ; # pin no:  9
place_pad -row IO_WEST  -location [expr $startLeft -  9*$pitch] "pad_jtag_tck_i"   ; # pin no: 10
place_pad -row IO_WEST  -location [expr $startLeft - 10*$pitch] "pad_jtag_trst_ni" ; # pin no: 11
place_pad -row IO_WEST  -location [expr $startLeft - 11*$pitch] "pad_jtag_tms_i"   ; # pin no: 12
place_pad -row IO_WEST  -location [expr $startLeft - 12*$pitch] "pad_jtag_tdi_i"   ; # pin no: 13
place_pad -row IO_WEST  -location [expr $startLeft - 13*$pitch] "pad_jtag_tdo_o"   ; # pin no: 14
place_pad -row IO_WEST  -location [expr $startLeft - 14*$pitch] "pad_vss0"         ; # pin no: 15
place_pad -row IO_WEST  -location [expr $startLeft - 15*$pitch] "pad_vdd0"         ; # pin no: 16

# Edge: BOTTOM (left to right)
place_pad -row IO_SOUTH  -location [expr $startBottom +  0*$pitch] "pad_vssio1"       ; # pin no:  1
place_pad -row IO_SOUTH  -location [expr $startBottom +  1*$pitch] "pad_vddio1"       ; # pin no:  2
place_pad -row IO_SOUTH  -location [expr $startBottom +  2*$pitch] "pad_gpio0_io"     ; # pin no:  3
place_pad -row IO_SOUTH  -location [expr $startBottom +  3*$pitch] "pad_gpio1_io"     ; # pin no:  4
place_pad -row IO_SOUTH  -location [expr $startBottom +  4*$pitch] "pad_gpio2_io"     ; # pin no:  5
place_pad -row IO_SOUTH  -location [expr $startBottom +  5*$pitch] "pad_gpio3_io"     ; # pin no:  6
place_pad -row IO_SOUTH  -location [expr $startBottom +  6*$pitch] "pad_gpio4_io"     ; # pin no:  7
place_pad -row IO_SOUTH  -location [expr $startBottom +  7*$pitch] "pad_gpio5_io"     ; # pin no:  8
place_pad -row IO_SOUTH  -location [expr $startBottom +  8*$pitch] "pad_gpio6_io"     ; # pin no:  9
place_pad -row IO_SOUTH  -location [expr $startBottom +  9*$pitch] "pad_gpio7_io"     ; # pin no: 10
place_pad -row IO_SOUTH  -location [expr $startBottom + 10*$pitch] "pad_gpio8_io"     ; # pin no: 11
place_pad -row IO_SOUTH  -location [expr $startBottom + 11*$pitch] "pad_gpio9_io"     ; # pin no: 12
place_pad -row IO_SOUTH  -location [expr $startBottom + 12*$pitch] "pad_gpio10_io"    ; # pin no: 13
place_pad -row IO_SOUTH  -location [expr $startBottom + 13*$pitch] "pad_gpio11_io"    ; # pin no: 14
place_pad -row IO_SOUTH  -location [expr $startBottom + 14*$pitch] "pad_vss1"         ; # pin no: 15
place_pad -row IO_SOUTH  -location [expr $startBottom + 15*$pitch] "pad_vdd1"         ; # pin no: 16

# Edge: RIGHT (bottom to top)
place_pad -row IO_EAST  -location [expr $startRight +  0*$pitch] "pad_vssio2"       ; # pin no:  1
place_pad -row IO_EAST  -location [expr $startRight +  1*$pitch] "pad_vddio2"       ; # pin no:  2
place_pad -row IO_EAST  -location [expr $startRight +  2*$pitch] "pad_gpio12_io"    ; # pin no:  3
place_pad -row IO_EAST  -location [expr $startRight +  3*$pitch] "pad_gpio13_io"    ; # pin no:  4
place_pad -row IO_EAST  -location [expr $startRight +  4*$pitch] "pad_gpio14_io"    ; # pin no:  5
place_pad -row IO_EAST  -location [expr $startRight +  5*$pitch] "pad_gpio15_io"    ; # pin no:  6
place_pad -row IO_EAST  -location [expr $startRight +  6*$pitch] "pad_gpio16_io"    ; # pin no:  7
place_pad -row IO_EAST  -location [expr $startRight +  7*$pitch] "pad_gpio17_io"    ; # pin no:  8
place_pad -row IO_EAST  -location [expr $startRight +  8*$pitch] "pad_gpio18_io"    ; # pin no:  9
place_pad -row IO_EAST  -location [expr $startRight +  9*$pitch] "pad_gpio19_io"    ; # pin no: 10
place_pad -row IO_EAST  -location [expr $startRight + 10*$pitch] "pad_gpio20_io"    ; # pin no: 11
place_pad -row IO_EAST  -location [expr $startRight + 11*$pitch] "pad_gpio21_io"    ; # pin no: 12
place_pad -row IO_EAST  -location [expr $startRight + 12*$pitch] "pad_gpio22_io"    ; # pin no: 13
place_pad -row IO_EAST  -location [expr $startRight + 13*$pitch] "pad_gpio23_io"    ; # pin no: 14
place_pad -row IO_EAST  -location [expr $startRight + 14*$pitch] "pad_vss2"         ; # pin no: 15
place_pad -row IO_EAST  -location [expr $startRight + 15*$pitch] "pad_vdd2"         ; # pin no: 16

# Edge: TOP (right to left)
place_pad -row IO_NORTH  -location [expr $startTop -  0*$pitch] "pad_vssio3"          ; # pin no:  1
place_pad -row IO_NORTH  -location [expr $startTop -  1*$pitch] "pad_vddio3"          ; # pin no:  2
place_pad -row IO_NORTH  -location [expr $startTop -  2*$pitch] "pad_gpio24_io"       ; # pin no:  3
place_pad -row IO_NORTH  -location [expr $startTop -  3*$pitch] "pad_gpio25_io"       ; # pin no:  4
place_pad -row IO_NORTH  -location [expr $startTop -  4*$pitch] "pad_gpio26_io"       ; # pin no:  5
place_pad -row IO_NORTH  -location [expr $startTop -  5*$pitch] "pad_gpio27_io"       ; # pin no:  6
place_pad -row IO_NORTH  -location [expr $startTop -  6*$pitch] "pad_gpio28_io"       ; # pin no:  7
place_pad -row IO_NORTH  -location [expr $startTop -  7*$pitch] "pad_gpio29_io"       ; # pin no:  8
place_pad -row IO_NORTH  -location [expr $startTop -  8*$pitch] "pad_gpio30_io"       ; # pin no:  9
place_pad -row IO_NORTH  -location [expr $startTop -  9*$pitch] "pad_gpio31_io"       ; # pin no: 10
place_pad -row IO_NORTH  -location [expr $startTop - 10*$pitch] "pad_unused0_o"       ; # pin no: 11
place_pad -row IO_NORTH  -location [expr $startTop - 11*$pitch] "pad_unused1_o"       ; # pin no: 12
place_pad -row IO_NORTH  -location [expr $startTop - 12*$pitch] "pad_unused2_o"       ; # pin no: 13
place_pad -row IO_NORTH  -location [expr $startTop - 13*$pitch] "pad_unused3_o"       ; # pin no: 14
place_pad -row IO_NORTH  -location [expr $startTop - 14*$pitch] "pad_vss3"            ; # pin no: 15
place_pad -row IO_NORTH  -location [expr $startTop - 15*$pitch] "pad_vdd3"            ; # pin no: 16

#################
# Corner Cells  #
#################

place_corners \
    $iocorner

#################
# Filler Cells  #
#################

place_io_fill -row IO_NORTH \
    {*}$iofill

place_io_fill \
    -row IO_SOUTH \
    {*}$iofill

place_io_fill \
    -row IO_WEST \
    {*}$iofill

place_io_fill \
    -row IO_EAST \
    {*}$iofill

#################
# Ring Signals  #
#################

connect_by_abutment

##################
#  Bonding Pads  #
##################

place_bondpad -bond bondpad_70x70 -offset {5.0 -70.0} pad_*

##################
# Remove IO rows #
##################

remove_io_rows