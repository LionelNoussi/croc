# IHP13 IC Design Flow

This document outlines the complete flow for IC design using the IHP13 PDK and OCEDA toolchain.

## 0. Environment Setup

icdesign ihp13 -update all -nogui

## 1. Development

1. Write RTL, testbench, and software.
2. Add all source files to Bender (Bender.yml) under appropriate sections.

## 2. Software Build & Simulation

oceda make -B sw  
oceda make verilator

✅ If simulation works, proceed to synthesis.

## 3. Synthesis (Yosys)

oceda make yosys-flist  
oceda make yosys

ℹ️ To preserve hierarchy for specific modules, add them to yosys/scripts/synthesis.tcl.

## 4. Physical Design (OpenROAD)

cd openroad  
oseda -2025.07 openroad scripts/S1_*  
oseda -2025.07 openroad scripts/S2_*  
oseda -2025.07 openroad scripts/S3_*  
oseda -2025.07 openroad scripts/S4_*

Execute scripts in order and review reports after each step!

✅ Script S5 finishes the flow. S6 is for analysis only.

## 5. GDS Generation

./final2gds.sh

This prepares the design for DRC & LVS checks.

## 6. Verification & Signoff

- Open GDS in Calibre
- Fix all DRC and LVS violations
- Save final outputs:
  - GDS
  - DEF
  - SPICE

Place these into openroad/out to complete the design handoff.
