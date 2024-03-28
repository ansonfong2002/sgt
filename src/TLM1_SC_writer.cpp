#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace std;

typedef struct edge {
    int s, d;
} edge;

void createConfig() {
    ofstream config("./config/TLM.config");
    config << "// CONFIGURATION FILE (for TLM models)\n[TLM1]\n-- Delay Factor: 1\n\n[TLM2]\n-- Off-Chip Memory Size: 8*1024*1024\n-- Off-Chip Read Delay: 100\n-- Off-Chip Write Delay: 110\n-- On-Chip Memory Size: 4*1024*1024\n-- On-Chip Read Delay: 5\n-- On-Chip Write Delay: 6\n-- Delay Factor: 1";
    config.close();
}

int main() {
    cout << "<> Starting SystemC (TLM1) Writer\n";
    string line;

    // READ CONFIG
    ifstream config("./config/TLM.config");
    if (!config.is_open()) {
        cout << "<> Error: missing TLM.config\n>> Creating TLM.config\n>> Try again?\n";
        createConfig();
        return 0;
    }
    cout << ">> Reading config/TLM.config...\n";
    for (int a = 0; a < 3; a++) {getline(config, line);}
    string delay = line.substr(17);
    config.close();

    // READ INPUT
    ifstream inputData("graph.txt");
    if (!inputData.is_open()) {
        cout << "<> Error opening file\n";
        return 0;
    }
    cout << ">> Reading graph.txt...\n";
    getline(inputData, line);
    int nodes = stoi(line.substr(7));

    vector<string> inputText(nodes);            // per-module input port text
    inputText[0] = "\tsc_fifo_in<int> in;\n";
    vector<string> outputText(nodes);           // per-module output port text
    vector<string> functionInput(nodes);        // per-module input variable
    vector<string> functionText(nodes);         // per-module function text
    functionText[0] = "\t\t\tin.read(max);\n";
    vector<string> functionOutput(nodes);       // per-module function output text

    vector<edge> edges;                 // edge data storage
    vector<int> terminals;              // terminal node ID storage

    // fill text bodies for primitive SC_MODULE instances
    for (int i = 0; i < nodes; i++) {
        getline(inputData, line);
        int parseIndex = 1;                             // index currently evaluating
        int parseBreak = line.find_first_of("[");       // index of next break
        
        // grab source
        int source = stoi(line.substr(parseIndex, parseBreak - parseIndex));    // source node
        
        // set next evaluation point, look for bracket
        parseIndex = parseBreak + 6; 
        parseBreak = line.find_first_of("[", parseIndex);
        
        // grab edges, if they exist
        while (parseBreak != (int)line.npos) {                   // while information still remains: node connects to others
            // grab dest
            int dest = stoi(line.substr(parseIndex, parseBreak - parseIndex));  // dest node
            
            // create edge
            edge newEdge;
            newEdge.s = source;
            newEdge.d = dest;
            edges.push_back(newEdge);
            
            // update node text
            outputText[i] += "\tsc_fifo_out<int> V" + to_string(dest) + ";\n";
            inputText[dest] += "\tsc_fifo_in<int> V" + to_string(source) + ";\n";
            functionInput[dest] += "\tint v" + to_string(source) + ";\n";
            functionText[dest] += "\t\t\tV" + to_string(source) + ".read(v" + to_string(i) + ");\n\t\t\tif (v" + to_string(i) + " > max) {max = v" + to_string(i) + ";}\n";
            functionOutput[i] += "\t\t\tV" + to_string(dest) + ".write(max + 1);\n";
            
            // set next evaluation point, look for bracket
            parseIndex = parseBreak + 5;
            parseBreak = line.find_first_of("[", parseIndex);
        }
    }
    inputData.close();

    // PRODUCE OUTPUT
    cout << ">> Writing graph_TLM1.cpp\n";
    ofstream sc_file("graph_TLM1.cpp");
    
    // text bodies for the DUT Constructor:
    string edgeDeclaration;
    string nodeDeclaration;
    string DUT_constructor = "\tSC_CTOR(DUT)\n\t:v0(\"v0\")\n";
    string nodeBinding = "\t\tv0.in.bind(input);\n";
    string outputBinding;

    // title + header
    sc_file << "// SystemC representation of generated task graph: graph_TLM1.cpp\n\n#include <stdio.h>\n#include <stdlib.h>\n#include <string.h>\n#include \"systemc.h\"\n\n#define DELAY_FACTOR " << delay << "\n\n";

    // write SC_MODULE text for nodes, while filling DUT Constructor node declarations
    for (int j = 0; j < nodes; j++) {
        nodeDeclaration += "\tV" + to_string(j) + " v" + to_string(j) + ";\n";
        if (j != 0) {DUT_constructor += "\t,v" + to_string(j) + "(\"v" + to_string(j) + "\")\n";}
        sc_file << "SC_MODULE(V" << j << ") {\n";
        if (outputText[j] == "") {
            // terminal node
            sc_file << inputText[j] << "\tsc_fifo_out<int> output;\n";
            terminals.push_back(j);
            functionOutput[j] = "\t\t\toutput.write(max + 1);\n";
        } else {
            // non-terminal node
            sc_file << inputText[j] << outputText[j];
        }
        sc_file << functionInput[j] << "\tvoid main(void) {\n\t\tint max = -1;\n\t\twhile (1) {\n" << functionText[j] << "\t\t\twait(DELAY_FACTOR, SC_MS);\n" << functionOutput[j] << "\t\t}\n\t}\n";
        sc_file << "\tSC_CTOR(V" << j << ") {\n";
        sc_file << "\t\tSC_THREAD(main);\n";
        sc_file << "\t}\n};\n";
    }

    // fill remaining text bodies for DUT Constructor
    for (int m = 0; m < (int)edges.size(); m++) {
        edgeDeclaration += "\tsc_fifo<int> E" + to_string(m) + ";\n";
        DUT_constructor += "\t,E" + to_string(m) + "(\"E" + to_string(m) + "\", 1)\n";
        nodeBinding += "\t\tv" + to_string(edges[m].s) + ".V" + to_string(edges[m].d) + ".bind(" + "E" + to_string(m) + ");\n";
        nodeBinding += "\t\tv" + to_string(edges[m].d) + ".V" + to_string(edges[m].s) + ".bind(" + "E" + to_string(m) + ");\n";
    }

    // write DUT constructor header
    sc_file << "SC_MODULE(DUT) {\n\tsc_fifo_in<int> input;\n";

    // terminal channel text
    string t_inputs;
    string t_outputs;
    string t_data;
    string t_monitor;
    string t_channels;
    string t_top_instance;
    string t_top_binding;
    string t_plat;
    string t_plat_instance1;
    string t_plat_instance2;
    string t_plat_binding;

    // ITERATE: terminals
    for (int k = 0; k < (int)terminals.size(); k++) {
        string tk_string = to_string(terminals[k]);
        string t_string = to_string(k);
        t_inputs += "\tsc_fifo_in<int> input_V" + tk_string + ";\n";
        t_outputs += "\tsc_fifo_out<int> output_V" + tk_string + ";\n";
        t_data += "\tint data_V" + tk_string + ";\n";
        t_monitor += "\n\t\tinput_V" + tk_string + ".read(data_V" + tk_string + ");\n\t\tt2 = sc_time_stamp();\n\t\tt = t2 - t1;\n\t\tprintf(\">> %9s: Monitor received output [%d] from V" + tk_string + " after %9s delay.\\n\", t2.to_string().c_str(), data_V" + tk_string + ", t.to_string().c_str());";
        t_channels += "\tsc_fifo<int> t" + t_string + ";\n";
        t_top_instance += "\t,t" + t_string + "(\"t" + t_string + "\", 1)\n";
        t_top_binding += "\t\tplatform.output_V" + tk_string + ".bind(t" + t_string + ");\n\t\tmonitor.input_V" + tk_string + ".bind(t" + t_string + ");\n";
        t_plat += "\tDataOut V" + tk_string + ";\n";
        t_plat_instance1 += "\t,t" + t_string + "(\"t" + t_string + "\", 1)\n";
        t_plat_instance2 += "\t,V" + tk_string + "(\"V" + tk_string + "\")\n";
        t_plat_binding += "\t\tdut.output_V" + tk_string + ".bind(t" + t_string + ");\n\t\tV" + tk_string + ".input.bind(t" + t_string + ");\n\t\tV" + tk_string + ".output.bind(output_V" + tk_string + ");\n";
        outputBinding += "\t\tv" + tk_string + ".output.bind(output_V" + tk_string + ");\n";
    }
    sc_file << t_outputs;

    // build + write DUT constructor body
    sc_file << nodeDeclaration << edgeDeclaration << DUT_constructor << "\t{\n" << nodeBinding << outputBinding << "\t}\n};\n";
    
    // build DataOut, DataIn, Platform, Stimulus, Monitor, Top, sc_main
    sc_file << "SC_MODULE(DataIn) {\n\tsc_fifo_in<int> input;\n\tsc_fifo_out<int> output;\n\tint data;\n\tvoid main() {\n\t\twhile (1) {\n\t\t\tinput.read(data);\n\t\t\toutput.write(data);\n\t\t}\n\t}\n\tSC_CTOR(DataIn) {\n\t\tSC_THREAD(main);\n\t}\n};\n";
    sc_file << "SC_MODULE(DataOut) {\n\tsc_fifo_in<int> input;\n\tsc_fifo_out<int> output;\n\tint data;\n\tvoid main() {\n\t\twhile (1) {\n\t\t\tinput.read(data);\n\t\t\toutput.write(data);\n\t\t}\n\t}\n\tSC_CTOR(DataOut) {\n\t\tSC_THREAD(main);\n\t}\n};\n";
    sc_file << "SC_MODULE(Platform) {\n\tsc_fifo_in<int> input;\n" << t_outputs << "\tsc_fifo<int> q0;\n" << t_channels << "\tDataIn din;\n\tDUT dut;\n" << t_plat;
    sc_file << "\tSC_CTOR(Platform)\n\t:q0(\"q0\", 1)\n"<< t_plat_instance1 << "\t,din(\"din\")\n\t,dut(\"dut\")\n" << t_plat_instance2 << "\t{\n\t\tdin.input.bind(input);\n\t\tdin.output.bind(q0);\n\t\tdut.input.bind(q0);\n" << t_plat_binding << "\t}\n};\n";
    sc_file << "SC_MODULE(Stimulus) {\n\tsc_fifo_out<int> output;\n\tsc_fifo_out<sc_time> StartTime;\n\tint data = 0;\n\tvoid main(void) {\n\t\tsc_time t;\n\t\toutput.write(data);\n\t\tt = sc_time_stamp();\n\t\tprintf(\"<> %9s: Stimulus sent.\\n\", t.to_string().c_str());\n\t\tStartTime.write(t);\n\t}\n\tSC_CTOR(Stimulus) {\n\t\tSC_THREAD(main);\n\t}\n};\n";
    sc_file << "SC_MODULE(Monitor) {\n" << t_inputs << "\tsc_fifo_in<sc_time> StartTime;\n" << t_data << "\tvoid main() {\n\t\tsc_time t, t1, t2;" << t_monitor << "\n\t}\n\tSC_CTOR(Monitor) {\n\t\tSC_THREAD(main);\n\t}\n};\n";
    sc_file << "SC_MODULE(Top) {\n\tsc_fifo<int> q0;\n" << t_channels << "\tsc_fifo<sc_time> qt;\n\tStimulus stimulus;\n\tPlatform platform;\n\tMonitor monitor;\n\tSC_CTOR(Top)\n\t:q0(\"q0\", 1)\n" << t_top_instance <<"\t,qt(\"qt\", 1)\n\t,stimulus(\"stimulus\")\n\t,platform(\"platform\")\n\t,monitor(\"monitor\")\n";
    sc_file << "\t{\n\t\tstimulus.output.bind(q0);\n\t\tstimulus.StartTime.bind(qt);\n\t\tplatform.input.bind(q0);\n" << t_top_binding << "\t\tmonitor.StartTime.bind(qt);\n\t}\n};\n";
    sc_file << "Top top(\"top\");\nint sc_main(int argc, char* argv[]) {\n\tsc_start();\n\treturn 0;\n}";

    cout << "<> Exiting...\n";
    sc_file.close();
    return 0;
}