#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace std;

typedef struct edge {
    int s, d;   // source, destination
    int ds, dd; // source depth, destination depth
} edge;

typedef struct node {
    int ID, depth;
    vector<int> stageConnections;   // stage connections at each node
    vector<int> nodeConnections;    // node connections at each node
    int host, terminal;         // boolean ints for SM host, terminal node
} node;

typedef struct stage {
    vector<int> readers;        // readers at each memory stage
    vector<int> writers;        // writers at each memory stage
    int accesses;               // total accesses (readers + writers).size()
} stage;

// simple binary search
int binarySearch(vector<int> array, int target, int start, int end) {
    int middle = (start + end) / 2;
    if (start < end) {
        if (array[middle] == target) {return middle;}
        if (array[middle] < target) {return binarySearch(array, target, middle + 1, end);}
        else {return binarySearch(array, target, start, middle);}
    } else {return -1;}
}

void createConfig() {
    ofstream config("./config/TLM.config");
    config << "// CONFIGURATION FILE (for TLM models)\n[TLM1]\n-- Delay Factor: 1\n\n[TLM2]\n-- Off-Chip Memory Size: 8*1024*1024\n-- Off-Chip Read Delay: 100\n-- Off-Chip Write Delay: 110\n-- On-Chip Memory Size: 4*1024*1024\n-- On-Chip Read Delay: 5\n-- On-Chip Write Delay: 6";
    config.close();
}

int main() {
    cout << "<> Starting SystemC (TLM2) Writer\n";
    string line;

    // READ CONFIG
    ifstream config("./config/TLM.config");
    if (!config.is_open()) {
        cout << "<> Error: missing TLM.config\n>> Creating TLM.config\n>> Try again?\n";
        createConfig();
        return 0;
    }
    cout << ">> Reading config/TLM.config...\n";
    for (int a = 0; a < 6; a++) {getline(config, line);}
    string configs = "\n#define OFF_CHIP_MEMORY_SIZE " + line.substr(25) + "\n";
    getline(config, line);
    configs += "#define OFF_CHIP_MEM_READ_ACCESS_DELAY sc_time(" + line.substr(24) + ", SC_NS)\n";
    getline(config, line);
    configs += "#define OFF_CHIP_MEM_WRITE_ACCESS_DELAY sc_time(" + line.substr(25) + ", SC_NS)\n";
    getline(config, line);
    configs += "#define ON_CHIP_MEMORY_SIZE " + line.substr(24) + "\n";
    getline(config, line);
    configs += "#define ON_CHIP_MEM_READ_ACCESS_DELAY sc_time(" + line.substr(23) + ", SC_NS)\n";
    getline(config, line);
    configs += "#define ON_CHIP_MEM_WRITE_ACCESS_DELAY sc_time(" + line.substr(24) + ", SC_NS)\n";

    // READ INPUT
    ifstream inputData("graph.txt");
    if (!inputData.is_open()) {
        cout << "<> Error opening file\n";
        return 0;
    }
    cout << ">> Reading graph.txt...\n";
    getline(inputData, line);
    int numNodes = stoi(line.substr(7));

    node nodes[numNodes];           // node data storage
    vector<edge> edges;             // edge data storage
    vector<stage> stages;           // memory stage storage
    vector<int> terminals;          // terminal node ID storage
    int terminal_depth = 0;

    // fill nodes, edges, stages
    for (int i = 0; i < numNodes; i++) {
        // grab new line
        getline(inputData, line);
        int parseIndex = 1;                             // index currently evaluating
        int parseBreak = line.find_first_of("[");       // index of next break
        
        // grab source and depth
        int source = stoi(line.substr(parseIndex, parseBreak - parseIndex));
        parseIndex = parseBreak + 1;                    // index of depth val
        parseBreak = line.find_first_of("]");
        int SRC_depth = stoi(line.substr(parseIndex, parseBreak - parseIndex)); 
        
        // log the new node
        nodes[i].ID = source;
        nodes[i].depth = SRC_depth;
        nodes[i].terminal = 0;

        parseIndex = parseBreak + 4;                    // set new evaluation point, after '[d]: V'
        parseBreak = line.find_first_of("[", parseIndex);   // set new break point, next '['. If no bracket exists, parseBreak = npos
        
        // determine: terminal || host || remote
        if (parseBreak == (int)line.npos) {
            nodes[i].terminal = 1;
            nodes[i].host = 0;
            terminals.push_back(source);
            if (!terminal_depth) {terminal_depth = nodes[i].depth;}
        } else if ((SRC_depth + 1) > (int)stages.size()) {
            // Host: create stage
            stage newStage;
            newStage.accesses = 1;
            newStage.writers.push_back(source);
            stages.push_back(newStage);
            nodes[i].host = 1;
        } else {
            // Non-terminal remote: add writer
            stages[SRC_depth].writers.push_back(source);
            stages[SRC_depth].accesses++;
            nodes[i].host = 0;
        }

        // grab edges, if they exist
        while (parseBreak != (int)line.npos) {
            // grab dest and depth
            int dest = stoi(line.substr(parseIndex, parseBreak - parseIndex));  // dest node
            parseIndex = parseBreak + 1;
            parseBreak = line.find_first_of("]", parseIndex);
            int DEST_depth = stoi(line.substr(parseIndex, parseBreak - parseIndex));    // dest depth
            parseIndex = parseBreak + 3;                        // set new evaluation point, after '[d],V'
            
            // create edge
            edge newEdge;
            newEdge.s = source;
            newEdge.d = dest;
            newEdge.ds = SRC_depth;
            newEdge.dd = DEST_depth;
            edges.push_back(newEdge);
            nodes[dest].nodeConnections.push_back(source);

            // update stage info: add new reader, add DEPTH to source stage connections if source stage empty or not redundant
            if (binarySearch(stages[SRC_depth].readers, dest, 0, stages[SRC_depth].readers.size()) == -1) {
                stages[SRC_depth].accesses++;    // new reader for established depth
                stages[SRC_depth].readers.push_back(dest);
                nodes[dest].stageConnections.push_back(SRC_depth);
            }

            // set new break point, next '['. If no bracket exists, parseBreak = npos
            parseBreak = line.find_first_of("[", parseIndex);
        }
    }
    inputData.close();

    // PRODUCE OUTPUT
    cout << ">> Writing graph_TLM2.cpp\n";
    ofstream sc_file("graph_TLM2.cpp");
    
    // text body storage
    vector<string> SM_text(numNodes);
    vector<int> muxCounts;
    string DUT_memD = "\tMemory";
    string DUT_sigD = "\tsc_event";
    string DUT_muxD, DUT_nodeD;
    string DUT_memI, DUT_muxI, DUT_nodeI;
    string DUT_muxB, DUT_nodeB;
    string DUT_outputs;
    string Dout_output, Dout_SM, Dout_funct;
    string plat_dut, plat_dout;
    string t_inputs, t_channels, t_data, t_monitor, t_channelB, t_channelI, t_top;

    // title + header
    sc_file << "// SystemC representation of generated task graph: graph_TLM2_cpp\n\n#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#include \"systemc.h\"\n\n";
    sc_file << "#include \"tlm.h\"\n#include \"tlm_utils/simple_initiator_socket.h\"\n#include \"tlm_utils/simple_target_socket.h\"\n#include \"lib/TLM2/Mux_N_FCFS.cpp\"\n#include \"lib/TLM2/Memory.cpp\"\n#include \"lib/TLM2/MemQ.cpp\"\n";
    sc_file << configs << "#define MUX_DELAY sc_time(1, SC_NS)\n#define SET_STACK_SIZE set_stack_size(128*1024*1024);\n\n";

    // struct M0_Channels
    sc_file << "struct M0_Channels {\n\tQueue<int, 1> input_data;\n};\n";
    
    /*  ITERATE: STAGES
        - starts input and DUT channel structs
    */
    for (int j = 0; j < (int)stages.size(); j++) {
        string stageStr = to_string(j);
        string a_str = to_string(stages[j].accesses);
        sc_file << "struct DUT_M" << stageStr << "_Channels {\n";
        for (int k = 0; k < (int)stages[j].writers.size(); k++) {
            sc_file << "\tQueue<int, 1> V" << to_string(stages[j].writers[k]) << "_data;\n";
        }
        sc_file << "};\n";
        
        // DUT text
        muxCounts.push_back(0);
        DUT_memD += " m" + stageStr + ",";
        DUT_memI += "\t,m" + stageStr + "(\"m" + stageStr + "\", ON_CHIP_MEMORY_SIZE, ON_CHIP_MEM_READ_ACCESS_DELAY, ON_CHIP_MEM_WRITE_ACCESS_DELAY)\n";
        DUT_muxD += "\tMux_N<" + a_str + "> mux" + stageStr + ";\n";
        DUT_muxI += "\t,mux" + stageStr + "(\"mux" + stageStr + "\", MUX_DELAY)\n";
        DUT_muxB += "\t\tmux" + stageStr + ".out.bind(m" + stageStr + ".s0);\n";
        DUT_sigD += " sig" + stageStr + ",";
    }
    DUT_memD[DUT_memD.size() - 1] = ';';
    DUT_sigD[DUT_sigD.size() - 1] = ';';

    /*  ITERATE: TERMINALS
        - completes terminal channel struct
    */
    sc_file << "struct M" + to_string(terminals[0]) + "_Channels{\n";
    int muxCount = 0;
    for (int m = 0; m < (int)terminals.size(); m++) {
        string t_str = to_string(terminals[m]);
        string m_str = to_string(m);
        sc_file << "\tQueue<int, 1> V" + t_str + "_data;\n";

        // DUT text
        DUT_outputs += "\ttlm::tlm_initiator_socket<> outputV" + t_str + ";\n";
    
        // DataOut Text
        Dout_output += "\tsc_fifo_out<int> outputV" + t_str + ";\n";
        Dout_SM += "\t\tMemQ<int, 1> dout_V" + t_str + "(SM2, offsetof(M" + to_string(terminals[0]) + "_Channels, V" + t_str + "_data));\n";
        Dout_funct += "\t\t\tdout_V" + t_str + ".Pop(data);\n\t\t\toutputV" + t_str + ".write(data);\n";
    
        // Platform text
        plat_dout += "\t\tdout.outputV" + t_str + ".bind(outputV" + t_str + ");\n";
        plat_dut += "\t\tdut.outputV" + t_str + ".bind(mux2.s[" + to_string(muxCount) + "]);\n";
        muxCount++;

        // Monitor/Top text
        t_inputs += "\tsc_fifo_in<int> inputV" + t_str + ";\n";
        t_channels += "\tsc_fifo_in<int> t" + m_str + ";\n";
        t_top +=  "\tsc_fifo<int> t" + m_str + ";\n";
        t_data += "\tint data_V" + t_str + ";\n";
        t_monitor += "\n\t\tinputV" + t_str + ".read(data_V" + t_str + ");\n\t\tt2 = sc_time_stamp();\n\t\tt = t2 - t1;\n\t\tprintf(\">> %9s: Monitor received output [%d] from V" + t_str + " after %9s delay.\\n\", t2.to_string().c_str(), data_V" + t_str + ", t.to_string().c_str());";
        t_channelI += "\t,t" + m_str + "(\"t" + m_str + "\", 1)\n";
        t_channelB += "\t\tplatform.outputV" + t_str + ".bind(t" + m_str + ");\n\t\tmonitor.inputV" + t_str + ".bind(t" + m_str + ");\n";
    }
    sc_file << "};\n\n";

    // SC_MODULE(V0)
    sc_file << "SC_MODULE(V0) {\n\ttlm_utils::simple_initiator_socket<V0> input;\n\tsc_event &sig_in;\n\ttlm_utils::simple_initiator_socket<V0> output;\n\tsc_event &sig_out;\n\tvoid main(void)";
    sc_file << "{\n\t\tHostedSM SM0(output, sig_out, 0x00, 0x1000, ON_CHIP_MEMORY_SIZE);\n\t\tMemQ<int, 1> v0_out(SM0, offsetof(DUT_M0_Channels, V0_data));\n\t\twait(MEMQ_INITIALIZATION_DELAY);\n";
    sc_file << "\t\tRemoteSM M0(input, sig_in, 0x00, OFF_CHIP_MEMORY_SIZE);\n\t\tMemQ<int, 1> v0_in(M0, offsetof(M0_Channels, input_data));\n\n\t\tint max = -1;\n\t\twhile (1) {\n\t\t\tv0_in.Pop(max);\n\t\t\tmax++;\n\t\t\tv0_out.Push(max);\n\t\t}\n\t}";
    sc_file << "\n\n\tSC_HAS_PROCESS(V0);\n\n\tV0(sc_module_name n, sc_event &sig_in, sc_event &sig_out)\n\t:sc_module(n)\n\t,sig_in(sig_in)\n\t,sig_out(sig_out)\n\t{\n\t\tSC_THREAD(main);\n\t\tSET_STACK_SIZE\n\t}\n};\n";
    
    /*  ITERATE: EDGES
        - writes text bodies: SM_text
    */
    for (int r = 0; r < (int)edges.size(); r++) {
        string s_str = to_string(edges[r].s);
        string d_str = to_string(edges[r].d);
        string ds_str = to_string(edges[r].ds);
        SM_text[edges[r].d] += "\t\tMemQ<int, 1> v" + d_str + "_from_v" + s_str + "(SM" + ds_str + ", offsetof(DUT_M" + ds_str + "_Channels, V" + s_str + "_data));\n";
    }
    
    // DUT: v0 binding
    DUT_nodeB = "\t\tv0.input.bind(input);\n\t\tv0.output.bind(mux0.s[0]);\n";
    muxCounts[0]++;

    /*  ITERATE: NODES
        - writes SC_MODULEs for all nodes
    */
    for (int n = 1; n < numNodes; n++) {
        string nodeStr = to_string(nodes[n].ID);
        string depthStr = to_string(nodes[n].depth);
        string SM_in, functionText, sigH, sigB, DUT_sigs, DUT_out;
        sc_file << "SC_MODULE(V" << nodeStr << ") {\n";
        
        // ITERATE: stageConnections
        for (int p = 0; p < (int)nodes[n].stageConnections.size(); p++) {
            string stageStr = to_string(nodes[n].stageConnections[p]);
            sc_file << "\ttlm_utils::simple_initiator_socket<V" << nodeStr << "> input_D" << stageStr << ";\n\tsc_event &sigD" + stageStr + ";\n";
            SM_in += "\t\tRemoteSM SM" + stageStr + "(input_D" + stageStr + ", sigD" + stageStr + ", 0x00, ON_CHIP_MEMORY_SIZE);\n";
            sigH += ", sc_event &sigD" + stageStr;
            sigB += "\t,sigD" + stageStr + "(sigD" + stageStr + ")\n";
            
            // DUT text
            DUT_sigs += ", sig" + stageStr;
            DUT_nodeB += "\t\tv" + nodeStr + ".input_D" + stageStr + ".bind(mux" + stageStr + ".s[" + to_string(muxCounts[nodes[n].stageConnections[p]]) + "]);\n";
            muxCounts[nodes[n].stageConnections[p]]++;
        }

        // output socket
        sc_file << "\ttlm_utils::simple_initiator_socket<V" << nodeStr << "> output;\n\tsc_event &sig_out;\n";
        
        // ITERATE: nodeConnections
        for (int q = 0; q < (int)nodes[n].nodeConnections.size(); q++) { 
            string readStr = to_string(nodes[n].nodeConnections[q]);
            sc_file << "\tint v" + readStr + ";\n";
            functionText += "\t\t\tv" + nodeStr + "_from_v" + readStr + ".Pop(v" + readStr + ");\n\t\t\tif (v" + readStr + " > max) {max = v" + readStr + " + 1;}\n";
        }
        sc_file << "\tvoid main(void) {\n";
        if (nodes[n].host) {
            sc_file << "\t\tHostedSM SM" << nodeStr << "(output, sig_out, 0x00, 0x1000, ON_CHIP_MEMORY_SIZE);\n\t\tMemQ<int, 1> v" << nodeStr << "_out(SM" + nodeStr << ", offsetof(DUT_M" << nodeStr << "_Channels, V" << nodeStr << "_data));\n\t\twait(MEMQ_INITIALIZATION_DELAY);\n";
            
            // DUT text
            DUT_out = ", sig" + depthStr;
            DUT_nodeB += "\t\tv" + nodeStr + ".output.bind(mux" + depthStr + ".s[" + to_string(muxCounts[nodes[n].depth]) + "]);\n";
            muxCounts[nodes[n].depth]++;
        } else {
            if (nodes[n].terminal) {
                sc_file << "\t\twait(MEMQ_INITIALIZATION_DELAY);\n\t\tRemoteSM M" << to_string(terminal_depth) << "(output, sig_out, 0x00, OFF_CHIP_MEMORY_SIZE);\n\t\tMemQ<int, 1> v" << nodeStr << "_out(M" << to_string(terminal_depth) << ", offsetof(M" << to_string(terminal_depth) << "_Channels, V" << nodeStr << "_data));\n";
                
                // DUT text
                DUT_out = ", sig_out";
                DUT_nodeB += "\t\tv" + nodeStr + ".output.bind(outputV" + nodeStr + ");\n";
            } else {
                sc_file << "\t\tRemoteSM SM" << depthStr << "(output, sig_out, 0x00, ON_CHIP_MEMORY_SIZE);\n\t\tMemQ<int, 1> v" << nodeStr << "_out(SM" << depthStr << ", offsetof(DUT_M" << depthStr << "_Channels, V" << nodeStr << "_data));\n";
                
                // DUT text
                DUT_out = ", sig" + depthStr;
                DUT_nodeB += "\t\tv" + nodeStr + ".output.bind(mux" + depthStr + ".s[" + to_string(muxCounts[nodes[n].depth]) + "]);\n";
                muxCounts[nodes[n].depth]++;
            }
        }
        sc_file << SM_in << SM_text[n] << "\n\t\tint max = -1;\n\t\twhile (1) {\n" << functionText << "\t\t\tv" + nodeStr + "_out.Push(max);\n\t\t}\n\t};\n";
        sc_file << "\tSC_HAS_PROCESS(V" + nodeStr + ");\n\n\tV" << to_string(nodes[n].ID) << "(sc_module_name n" << sigH << ", sc_event &sig_out)\n\t:sc_module(n)\n" << sigB << "\t,sig_out(sig_out)\n\t{\n\t\tSC_THREAD(main);\n\t\tSET_STACK_SIZE\n\t}\n};\n";

        // DUT text
        DUT_nodeD += "\tV" + nodeStr + " v" + nodeStr + ";\n";
        DUT_nodeI += "\t,v" + nodeStr + "(\"v" + nodeStr + "\"" + DUT_sigs + DUT_out + ")\n";
    }

    // SC_MODULE(DUT)
    sc_file << "SC_MODULE(DUT) {\n\ttlm::tlm_initiator_socket<> input;\n\tsc_event &sig_in;\n" << DUT_outputs << "\tsc_event &sig_out;\n\n";
    sc_file << DUT_memD << "\n" << DUT_muxD << DUT_sigD << "\n\n\tV0 v0;\n" << DUT_nodeD << "\n\tDUT(sc_module_name n, sc_event &sig_in, sc_event &sig_out)\n\t:sc_module(n)\n";
    sc_file << "\t,sig_in(sig_in)\n\t,sig_out(sig_out)\n" << DUT_memI << DUT_muxI << "\t,v0(\"v0\", sig_in, sig0)\n" << DUT_nodeI << "\t{\n" << DUT_nodeB << DUT_muxB << "\t}\n";
    sc_file << "};\n";

    // SC_MODULE(DataIn)
    sc_file << "SC_MODULE(DataIn) {\n\tsc_fifo_in<int> input;\n\ttlm_utils::simple_initiator_socket<DataIn> output;\n\tsc_event &sig_out;\n\tint data;\n\tvoid main() {\n\t\tHostedSM SM1(output, sig_out, 0x00, 0x1000, OFF_CHIP_MEMORY_SIZE);\n\t\tMemQ<int, 1> din_out(SM1, offsetof(M0_Channels, input_data));\n";
    sc_file << "\t\twait(MEMQ_INITIALIZATION_DELAY);\n\t\twhile (1) {\n\t\t\tinput.read(data);\n\t\t\tdin_out.Push(data);\n\t\t}\n\t}\n\tSC_HAS_PROCESS(DataIn);\n\tDataIn(sc_module_name n, sc_event &sig_out)\n\t:sc_module(n)\n\t,sig_out(sig_out)\n\t{\n\t\tSC_THREAD(main);\n\t\tSET_STACK_SIZE\n\t}\n};\n";
    
    // SC_MODULE(DataOut)
    sc_file << "SC_MODULE(DataOut) {\n\ttlm_utils::simple_initiator_socket<DataOut> input;\n\tsc_event &sig_in;\n" << Dout_output << "\tint data;\n\tvoid main() {\n\t\tHostedSM SM2(input, sig_in, 0x00, 0x1000, OFF_CHIP_MEMORY_SIZE);\n" << Dout_SM << "\t\twait(MEMQ_INITIALIZATION_DELAY);\n";
    sc_file << "\t\twhile (1) {\n" << Dout_funct << "\t\t}\n\t}\n\tSC_HAS_PROCESS(DataOut);\n\tDataOut(sc_module_name n, sc_event &sig_in)\n\t:sc_module(n)\n\t,sig_in(sig_in)\n\t{\n\t\tSC_THREAD(main)\n\t\tSET_STACK_SIZE\n\t}\n};\n";
    
    // SC_MODULE(Platform)
    sc_file << "SC_MODULE(Platform) {\n\tsc_fifo_in<int> input;\n" << Dout_output << "\n\tMemory mem1, mem2;\n\tMux_N<2> mux1;\n\tMux_N<" << to_string(terminals.size() + 1) << "> mux2;\n\tsc_event sig1, sig2;\n\n\tDataIn din;\n\tDUT dut;\n\tDataOut dout;\n\n";
    sc_file << "\tSC_CTOR(Platform)\n\t:mem1(\"mem1\", OFF_CHIP_MEMORY_SIZE, OFF_CHIP_MEM_READ_ACCESS_DELAY, OFF_CHIP_MEM_WRITE_ACCESS_DELAY)\n\t,mem2(\"mem2\", OFF_CHIP_MEMORY_SIZE, OFF_CHIP_MEM_READ_ACCESS_DELAY, OFF_CHIP_MEM_WRITE_ACCESS_DELAY)\n";
    sc_file << "\t,mux1(\"mux1\", MUX_DELAY)\n\t,mux2(\"mux2\", MUX_DELAY)\n\t,din(\"din\", sig1)\n\t,dut(\"dut\", sig1, sig2)\n\t,dout(\"dout\", sig2)\n\t{\n\t\tdin.input.bind(input);\n\t\tdin.output.bind(mux1.s[0]);\n\t\tdut.input.bind(mux1.s[1]);\n";
    sc_file << plat_dut << "\t\tdout.input.bind(mux2.s[" << to_string(muxCount) << "]);\n" << plat_dout << "\t\tmux1.out.bind(mem1.s0);\n\t\tmux2.out.bind(mem2.s0);\n\t}\n};\n";
    
    // SC_MODULE(Stimulus)
    sc_file << "SC_MODULE(Stimulus) {\n\tsc_fifo_out<int> output;\n\tsc_fifo_out<sc_time> StartTime;\n\tint data = 0;\n\tvoid main(void) {\n\t\tsc_time t;\n\t\toutput.write(data);\n\t\tt = sc_time_stamp();\n\t\tprintf(\"<> %9s: Stimulus sent.\\n\", t.to_string().c_str());\n\t\tStartTime.write(t);\n\t}\n\tSC_CTOR(Stimulus) {\n\t\tSC_THREAD(main);\n\t}\n};\n";

    // SC_MODULE(Monitor)
    sc_file << "SC_MODULE(Monitor) {\n" << t_inputs << "\tsc_fifo_in<sc_time> StartTime;\n" << t_data << "\tvoid main() {\n\t\tsc_time t, t1, t2;" << t_monitor << "\n\t}\n\tSC_CTOR(Monitor) {\n\t\tSC_THREAD(main);\n\t}\n};\n";

    // SC_MODULE(Top)
    sc_file << "SC_MODULE(Top) {\n\tsc_fifo<int> q0;\n" << t_top << "\tsc_fifo<sc_time> qt;\n\tStimulus stimulus;\n\tPlatform platform;\n\tMonitor monitor;\n\tSC_CTOR(Top)\n\t:q0(\"q0\", 1)\n" << t_channelI <<"\t,qt(\"qt\", 1)\n\t,stimulus(\"stimulus\")\n\t,platform(\"platform\")\n\t,monitor(\"monitor\")\n";
    sc_file << "\t{\n\t\tstimulus.output.bind(q0);\n\t\tstimulus.StartTime.bind(qt);\n\t\tplatform.input.bind(q0);\n" << t_channelB << "\t\tmonitor.StartTime.bind(qt);\n\t}\n};\n";
    sc_file << "Top top(\"top\");\nint sc_main(int argc, char* argv[]) {\n\tsc_start();\n\treturn 0;\n}";
}