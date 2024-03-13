# Overview: SYNTHETIC GRAPH TOOLBOX (SGT)
- Author: Anson Fong
- Edited: March 12, 2024
- Contains:
    - CONFIG folder
        - Configuration file for gg.cpp [gg.config]
        - Configuration file for TLM1 and TLM2 models [TLM.config]
    - LIB folder
        - Python dependencies
        - TLM2 dependencies
    - SRC folder
        - Synthetic Task Graph Generator [gg.cpp]
        - SystemC-TLM1 translator [TLM1_SC_writer.cpp]
        - SystemC-TLM2 translator [TLM2_SC_writer.cpp]
        - Python script generator for GPC [py_writer.cpp]
    - Makefile
    - Setup script [setup.csh]

# Synthetic Task Graph Generator [gg.cpp]
- Use "make gg" to generate a graph
    - Generates directed acyclic graph using [gg.config].
    - Creates output file [graph.txt].
        - Lists all nodes and their children.
    - [gg.cpp] will crash if configuration file contains bad input.
        - Singular output = 0 --> task graph terminates at single node.
        - Singular output = 1 --> task graph may have multiple terminal nodes.
        - "Maximum edges" limits the amount of children a node can have.
    - Default: uses random() at a fixed seed of 0.
    - Default: uses threshold value of 0.3 for appending edges.

# SystemC (TLM1 and TLM2) Translators [TLM1_SC_writer.cpp] [TLM2_SC_writer.cpp]
- Use "make writeTLM1" or "make writeTLM2" to generate graph and SystemC code.
    - Configure using [TLM.config]. Both models may not work if [TLM.config] contains bad input.
    - Translates [graph.txt] into SystemC (TLM1 or TLM2).
    - Creates output file [graph_TLM1.cpp] [graph_TLM2.cpp].
        - handles singular and non-singular output task graphs.
    - Each node compares inputs with the highest input received, increments 1, passes it along the graph.
    - Output at Monitor will reveal longest paths to terminal node(s).

# Simulating SystemC Models [graph_TLM1.cpp] [graph_TLM2.cpp]
- Requires setup: run "source setup.csh"
- Use "make testTLM1" or "make testTLM2" to run SystemC simulation.
    - Uses output from [gg.cpp] and [TLM1_SC_writer] or [TLM2_SC_writer] to build SystemC model.
    - Sends one stimulus signal "0" through the DUT, Monitor reveals longest path(s) to terminal node(s).

# Python Script Generation [py_writer.cpp]
- Requires setup: run "source setup.csh"
- Use "make mapper" to generate and run mapperInput.py file.
    - Parses graph output from [gg.cpp] to write a python script.
    - Although python script handles multiple terminal nodes, GPC compiler may not.
    - Running mapperInput.py using will create:
        - ./input/[canny.json] containing graph data for GPC compiler
        - Visualization of graph nodes/edges 
        - Requires X11 remote graphics and Python 3+

# MakeFile Commands
- "gg"
    - builds and runs [gg.cpp].
    - creates output file [graph.txt].
- "writeTLM1"
    - runs "gg" and [TLM1_SC_writer.cpp].
    - creates output file [graph_TLM1.cpp].
- "writeTLM2"
    - runs "gg" and [TLM2_SC_writer.cpp].
    - creates output file [graph_TLM2.cpp].
- "testTLM1"
    - runs "writeTLM1".
    - builds and runs [graph_TLM1.cpp].
- "testTLM2"
    - runs "writeTLM2".
    - builds and runs [graph_TLM2.cpp].
- "mapper"
    - runs "gg" and [py_writer.cpp].
    - creates output file [mapperInput.py].
    - runs [mapperInput.py] using python3 interpreter.
- "make clean"
    - removes all generated files.
