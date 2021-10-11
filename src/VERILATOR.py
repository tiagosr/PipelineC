import sys
import os
import shutil

import SIM
import SYN
import C_TO_LOGIC
import OPEN_TOOLS

VERILATOR_EXE="verilator"
VERILATOR_BIN_PATH = OPEN_TOOLS.OSS_CAD_SUITE_PATH + "/bin"

if not os.path.exists(VERILATOR_BIN_PATH):
  VERILATOR_EXE_PATH = C_TO_LOGIC.GET_TOOL_PATH(VERILATOR_EXE)
  if VERILATOR_EXE_PATH is not None:
    VERILATOR_BIN_PATH = os.path.abspath(os.path.dirname(VERILATOR_EXE_PATH))

def DO_SIM(latency, parser_state, args):
  print("================== Doing Verilator Simulation ================================", flush=True)
  VERILATOR_OUT_DIR = SYN.SYN_OUTPUT_DIRECTORY + "/verilator"
  if not os.path.exists(VERILATOR_OUT_DIR):
    os.makedirs(VERILATOR_OUT_DIR)
  # Generate helpful include of verilator names
  names_text = ""
  # Clocks
  clock_name_to_mhz,out_filepath = SYN.GET_CLK_TO_MHZ_AND_CONSTRAINTS_PATH(parser_state, None, True)
  if len(clock_name_to_mhz) > 1:
    for clock_name,mhz in clock_name_to_mhz.items():
      if mhz:
        names_text += f'#define {clock_name} {clock_name.replace("__","_")}\n'
      else:
        names_text += f'#define clk {clock_name.replace("__","_")}\n'
  else:
    clock_name,mhz = list(clock_name_to_mhz.items())[0]
    names_text += f'#define clk {clock_name.replace("__","_")}\n'
    
  # Debug ports
  debug_names = []
  for func in parser_state.main_mhz:
    debug_name = func.split("_DEBUG")[0]
    if func.endswith("_DEBUG_OUTPUT_MAIN"):
      debug_names.append(debug_name)
      debug_verilator_name = func.replace("__","_") + "_return_output"
      names_text += f'#define {debug_name} {debug_verilator_name}\n'
    if func.endswith("_DEBUG_INPUT_MAIN"):
      debug_names.append(debug_name)
      debug_verilator_name = func.replace("__","_") + "_val"
      names_text += f'#define {debug_name} {debug_verilator_name}\n'
      
  names_text += '''#define DUMP_PIPELINEC_DEBUG(top) \
cout <<'''
  for debug_name in debug_names:
    names_text += '"' + debug_name + ': " << to_string(' +"top->" + debug_name + ") << " + '" " << '
  names_text += "endl;\n"
      
  # Write names files
  names_h_path = VERILATOR_OUT_DIR + "/pipelinec_verilator.h"
  f=open(names_h_path,"w")
  f.write(names_text)
  f.close()  
  
  # Generate main.cpp
  main_cpp_text = '''
// Default main.cpp template

// Names and helper macro generated by PipelineC tool
#include "pipelinec_verilator.h"

// Generated by Verilator
#include "verilated.h"
#include "Vtop.h" 

#include <iostream>
using namespace std;

int main(int argc, char *argv[]) {
    Vtop* g_top = new Vtop;
    
    // Run the simulation for 10 cycles
    uint64_t cycle = 0;
    while (cycle < 10)
    {
        // Print the PipelineC debug vars
        cout << "cycle " << cycle << ": ";
        DUMP_PIPELINEC_DEBUG(g_top)

        g_top->clk = 0;
        g_top->eval();

        g_top->clk = 1;
        g_top->eval();
        ++cycle;
    }

    return 0;
}
'''
  main_cpp_path = VERILATOR_OUT_DIR + "/" + "main.cpp"
  f=open(main_cpp_path,"w")
  f.write(main_cpp_text)
  f.close()
  
  # Use main cpp template or not?
  if args.main_cpp is not None:
    main_cpp_path = os.path.abspath(args.main_cpp)
  
  # Generate+compile sim .cpp from output VHDL
  # Get all vhd files in syn output
  vhd_files = SIM.GET_SIM_FILES(latency=0)
 
  # Identify tool versions
  import OPEN_TOOLS
  print(C_TO_LOGIC.GET_SHELL_CMD_OUTPUT(f"{OPEN_TOOLS.GHDL_BIN_PATH}/ghdl --version"), flush=True)
  print(C_TO_LOGIC.GET_SHELL_CMD_OUTPUT(f"{OPEN_TOOLS.YOSYS_BIN_PATH}/yosys --version"), flush=True)
  print(C_TO_LOGIC.GET_SHELL_CMD_OUTPUT(f"{VERILATOR_BIN_PATH}/verilator --version"), flush=True)  
  
  # Write a shell script to execute
  m_ghdl = ""
  if not OPEN_TOOLS.GHDL_PLUGIN_BUILT_IN:
    m_ghdl = "-m ghdl "
  sh_text = f'''
{OPEN_TOOLS.GHDL_BIN_PATH}/ghdl -i --std=08 `cat ../vhdl_files.txt` && \
{OPEN_TOOLS.GHDL_BIN_PATH}/ghdl -m --std=08 top && \
{OPEN_TOOLS.YOSYS_BIN_PATH}/yosys -g {m_ghdl}-p "ghdl --std=08 top; proc; opt; fsm; opt; memory; opt; write_verilog ../top/top.v" && \
{VERILATOR_BIN_PATH}/verilator -Wno-UNOPTFLAT --top-module top -cc ../top/top.v -O3 --exe {main_cpp_path} -I{VERILATOR_OUT_DIR} -I{C_TO_LOGIC.REPO_ABS_DIR()} && \
make CXXFLAGS="-I{VERILATOR_OUT_DIR} -I{C_TO_LOGIC.REPO_ABS_DIR()}" -j4 -C obj_dir -f Vtop.mk
'''
  # --report-unoptflat
  sh_path = VERILATOR_OUT_DIR + "/" + "verilator.sh"
  f=open(sh_path,"w")
  f.write(sh_text)
  f.close()
  
  # Run compile
  print(f"Compiling {main_cpp_path}...", flush=True)
  bash_cmd = f"bash {sh_path}"
  #print(bash_cmd, flush=True)  
  log_text = C_TO_LOGIC.GET_SHELL_CMD_OUTPUT(bash_cmd, cwd=VERILATOR_OUT_DIR)
  #print(log_text, flush=True)

  # Run the simulation
  print("Starting simulation...", flush=True)
  bash_cmd = "./obj_dir/Vtop"
  #print(bash_cmd, flush=True)
  log_text = C_TO_LOGIC.GET_SHELL_CMD_OUTPUT(bash_cmd, cwd=VERILATOR_OUT_DIR)
  print(log_text, flush=True)
  sys.exit(0)
