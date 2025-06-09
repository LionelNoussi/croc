# Copyright 2023 ETH Zurich and University of Bologna.
# Solderpad Hardware License, Version 0.51, see LICENSE for details.
# SPDX-License-Identifier: SHL-0.51

# Authors:
# - Jannis Schönleber <janniss@iis.ee.ethz.ch>
# - Philippe Sauter   <phsauter@iis.ee.ethz.ch>

# Helper macros to save and load checkpoints
set time [elapsed_run_time]
if { ![info exists save_dir] } {set save_dir "save"}

proc save_checkpoint { checkpoint_name args } {
    global save_dir time step_by_step_debug
    set lvs [expr {[lsearch -exact $args "-lvs"] != -1}]

    utl::report "Saving checkpoint $checkpoint_name"

    set checkpoint_dir ${save_dir}/${checkpoint_name}
    set checkpoint ${save_dir}/${checkpoint_name}/${checkpoint_name}

    exec mkdir -p $checkpoint_dir

    write_def ${checkpoint}.def
    write_verilog ${checkpoint}.v
    write_db ${checkpoint}.odb
    write_sdc ${checkpoint}.sdc

    if { $lvs } {
        write_verilog -include_pwr_gnd ${checkpoint}_lvs.v
    }

    if { $step_by_step_debug } {
        utl::report "Pause at checkpoint: $checkpoint_name"
        gui::pause
    }
}

proc load_checkpoint { checkpoint_name args } {
    global save_dir
    set lvs [expr {[lsearch -exact $args "-lvs"] != -1}]
    utl::report "Loading checkpoint $checkpoint_name"
    
    set checkpoint_dir ${save_dir}/${checkpoint_name}
    set checkpoint ${save_dir}/${checkpoint_name}/${checkpoint_name}

    read_verilog ${checkpoint}.v
    read_db ${checkpoint}.odb
    if { [file exists ${checkpoint}.sdc] } {
        read_sdc ${checkpoint}.sdc
    }
}

proc load_checkpoint_def { checkpoint_name } {
    global save_dir
    utl::report "Loading checkpoint $checkpoint_name"
    set checkpoint ${save_dir}/${checkpoint_name}
    
    exec unzip ${checkpoint}.zip -d ${save_dir}
    read_verilog ${checkpoint}/$checkpoint_name.v
    read_def ${checkpoint}/$checkpoint_name.def
}