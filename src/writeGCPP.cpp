#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace std;

typedef struct edge {
    int s, d;
} edge;

typedef struct node {
    vector<edge> inEdges;
    vector<edge> outEdges;
    int ID, terminal;
} node;

void writeNodes(ofstream &out_file, node nodes[], int numNodes) {
    // node strings: initialized to V0 values
    string inputQueues = "\tQUEUE_IN<int> &input;\n";
    string functionText = "\t\t\tinput.POP(max);\n";
    string constr1 = "\tV0(QUEUE_IN<int> &input, ";
    string constr2 = "input(input), ";
    string outputQueues, inputText, outputText;

    // LOOP: NODES
    for (int i = 0; i < numNodes; i++) {
        string str = to_string(i);

        out_file << "class V" << str << ": public MODULE {\n";
        for (int j = 0; j < (int)nodes[i].inEdges.size(); j++) {
            // Node inputs
            string sourceStr = to_string(nodes[i].inEdges[j].s);
            inputQueues += "\tQUEUE_IN<int> &V" + sourceStr + ";\n";
            inputText += "\tint v" + sourceStr + ";\n";
            functionText += "\t\t\tV" + sourceStr + ".POP(v" + sourceStr + ");\n\t\t\tif (v" + sourceStr + " > max) {max = v" + sourceStr + ";}\n";
            if (j > 0) {constr1 += "QUEUE_IN<int> &V" + sourceStr + ", ";}
            else {constr1 += "\tV" + to_string(i) + "(QUEUE_IN<int> &V" + sourceStr + ", ";}
            constr2 += "V" + sourceStr + "(V" + sourceStr + "), ";
        }
        for (int k = 0; k < (int)nodes[i].outEdges.size(); k++) {
            // Node outputs
            string destStr = to_string(nodes[i].outEdges[k].d);
            outputQueues += "\tQUEUE_OUT<int> &V" + destStr + ";\n";
            functionText += "\t\t\tV" + destStr + ".PUSH(max + 1);\n";
            if (k > 0) {
                constr1 += ", QUEUE_OUT<int> &V" + destStr;
                constr2 += ", V" + destStr + "(V" + destStr + ")";
            } else {
                constr1 += "QUEUE_OUT<int> &V" + destStr;
                constr2 += "V" + destStr + "(V" + destStr + ")";
            }
        }
        
        // terminal output text
        if (nodes[i].terminal) {
            // Node Terminals
            outputQueues += "\tQUEUE_OUT<int> &output_V" + str + ";\n";
            outputText += "\t\t\toutput_V" + str + ".PUSH(max + 1);\n";
            constr1 += "QUEUE_OUT<int> &output_V" + str;
            constr2 += "output_V" + str + "(output_V" + str + ")";
        }
        
        // output to file
        constr1 += ")\n\t\t: ";
        out_file << inputQueues << outputQueues << inputText << "\tpublic:\n\tvoid main(void) {\n\t\tint max = -1;\n\t\twhile (1) {\n";
        out_file << functionText << outputText << "\t\t}\n\t}\n" << constr1 << constr2 << "\n\t{\n\t\tTHREAD::DETACH(this);\n\t}\n};\n\n";
        
        // reset buffers
        inputQueues = "";
        outputQueues = "";
        inputText = "";
        functionText = "";
        outputText = "";
        constr1 = "";
        constr2 = "";
    }
}

void writeDUT(ofstream &out_file, node nodes[], int numNodes, vector<edge> edges) {
    // DUT strings
    string DUT_queues, DUT_nodes, DUT_constr1, DUT_constr2, DUT_constr3;
    string DUT_edges = "\tQUEUE<int>"; 
    
    // LOOP: EDGES
    for (int i = 0; i < (int)edges.size(); i++) {DUT_edges += " V" + to_string(edges[i].s) + "_V" + to_string(edges[i].d) + ",";}
    DUT_edges[DUT_edges.size() - 1] = ';';
    
    // LOOP: NODES
    for (int i = 0; i < numNodes; i++) {
        string str = to_string(i);
        if (i > 0) {DUT_constr3 += "\t,v" + str + "(";}
        else {DUT_constr3 += "\t,v0(DataIn, ";}
        DUT_nodes += "\tV" + str + " v" + str + ";\n";

        // Nodes inputs
        for (int j = 0; j < (int)nodes[i].inEdges.size(); j++) {
            DUT_constr3 += "V" + to_string(nodes[i].inEdges[j].s) + "_V" + to_string(nodes[i].inEdges[j].d) + ", ";
        }

        // Node outputs
        for (int k = 0; k < (int)nodes[i].outEdges.size(); k++) {
            if (k > 0) {DUT_constr3 += ", V" + str + "_V" + to_string(nodes[i].outEdges[k].d);} 
            else {DUT_constr3 += "V" + str + "_V" + to_string(nodes[i].outEdges[k].d);}
        }
        if (nodes[i].terminal) {
            DUT_queues += "\tQUEUE_OUT<int> &V" + str + "_out;\n";
            DUT_constr1 += ", QUEUE_OUT<int> &V" + str + "_out_";
            DUT_constr2 += ", V" + str + "_out(V" + str + "_out_)";
            DUT_constr3 += "V" + str + "_out";
        }
        DUT_constr3 += ")\n";
    }

    // output to file
    DUT_constr1 += ")\n\t:DataIn(DataIn_)";
    out_file << "class DUT: public MODULE {\n\tQUEUE_IN<int> &DataIn;\n" << DUT_queues << "\n" << DUT_edges << "\n" << DUT_nodes << "\n\tpublic:\n";
    out_file << "\tDUT(QUEUE_IN<int> &DataIn_" << DUT_constr1 << DUT_constr2 << "\n" << DUT_constr3 << "\t{}\n};\n\n";
}

void writeData(ofstream &out_file) {
    out_file << "class DataIn: public MODULE {\n\tQUEUE_IN<int> &input;\n\tQUEUE_OUT<int> &output;";
    out_file << "\n\tint data;\n\tpublic:\n\tvoid main() {\n\t\twhile (1) {\n\t\t\tinput.POP(data);\n\t\t\toutput.PUSH(data);\n\t\t}\n\t}";
    out_file << "\n\tDataIn(QUEUE_IN<int> &input, QUEUE_OUT<int> &output)\n\t\t: input(input), output(output)\n\t{\n\t\tTHREAD::DETACH(this);\n\t}\n};";
    out_file << "\n\nclass DataOut: public MODULE {\n\tQUEUE_IN<int> &input;\n\tQUEUE_OUT<int> &output;";
    out_file << "\n\tint data;\n\tpublic:\n\tvoid main() {\n\t\twhile (1) {\n\t\t\tinput.POP(data);\n\t\t\toutput.PUSH(data);\n\t\t}\n\t}";
    out_file << "\n\tDataOut(QUEUE_IN<int> &input, QUEUE_OUT<int> &output)\n\t\t: input(input), output(output)\n\t{\n\t\tTHREAD::DETACH(this);\n\t}\n};\n\n";
}

void writePlatform(ofstream &out_file, vector<int> terminals) {
    // Platform strings
    string outputs, channels, douts, constr1, constr2, constr3, constr4, constr5;
    for (int i = 0; i < (int)terminals.size(); i++) {
        string str = to_string(terminals[i]);
        outputs += "\tQUEUE_OUT<int> &V" + str + "_out;\n";
        channels += "\tQUEUE<int> dut_V" + str + ";\n";
        douts += "\tDataOut V" + str + ";\n";
        constr1 += ", QUEUE_OUT<int> &V" + str + "_out_";
        constr2 += ", V" + str + "_out(V" + str + "_out_)";
        constr3 += "\t,dut_V" + str + "(1)\n";
        constr4 += ", dut_V" + str;
        constr5 += "\t,V" + str + "(dut_V" + str + ", V" + str + "_out)\n";
    }
    
    // output to file
    out_file << "class Platform: public MODULE{\n\tQUEUE_IN<int> &input;\n" << outputs << "\n\tQUEUE<int> din_dut;\n" << channels;
    out_file << "\tDataIn din;\n\tDUT dut;\n" << douts << "\n\tpublic:\n\tPlatform(QUEUE_IN<int> &input_" << constr1 << ")\n\t: input(input_)" << constr2;
    out_file << "\n\t,din_dut(1)\n" << constr3 << "\t,din(input, din_dut)\n\t,dut(din_dut" << constr4 << ")\n" << constr5 << "\t{}\n};\n\n";
}

void writeMonitor(ofstream &out_file, vector<int> terminals) {
    // Monitor strings
    string inputs, data, function, constr1, constr2;
    for (int i = 0; i < (int)terminals.size(); i++) {
        string str = to_string(terminals[i]);
        inputs += "\tQUEUE_IN<int> &input_V" + str + ";\n";
        data += "\tint data_V" + str + ";\n";
        function += "\t\tinput_V" + str + ".POP(data_V" + str + ");\n\t\tprintf(\">> Monitor received [\%d] from V" + str + ".\\n\", data_V" + str + ");\n";
        if (i > 0) {
            constr1 += ", QUEUE_IN<int> &input_V" + str;
            constr2 += ", input_V" + str + "(input_V" + str + ")";
        } 
        else {
            constr1 += "QUEUE_IN<int> &input_V" + str;
            constr2 += ": input_V" + str + "(input_V" + str + ")";
        }
    }

    // output to file
    constr1 += ")\n\t";
    out_file << "class Monitor: public MODULE {\n" << inputs << data << "\tpublic:\n\tvoid main(void) {\n";
    out_file << function << "\t\tprintf(\"<> Monitor exits simulation.\\n\");\n\t}\n\tMonitor(" << constr1 << constr2;
    out_file << "\n\t{\n\t\tTHREAD::CREATE(this);\n\t}\n};\n\n";
}

void writeTop(ofstream &out_file, vector<int> terminals) {
    // Top strings
    string channels, constr1, constr2, constr3;
    for (int i = 0; i < (int)terminals.size(); i++) {
        string str = to_string(terminals[i]);
        channels += "\tQUEUE<int> plat_mon_V" + str + ";\n";
        constr1 += "\t,plat_mon_V" + str + "(1)\n";
        constr2 += ", plat_mon_V" + str;
        if (i > 0) {constr3 += ", plat_mon_V" + str;} 
        else {constr3 += "plat_mon_V" + str;}
    }
    
    // output to file
    out_file << "class Top: public MODULE {\n\tQUEUE<int> stim_plat;\n" << channels << "\tStimulus stim;\n\tPlatform plat;\n\tMonitor mon;\n";
    out_file << "\n\tpublic:\n\tTop()\n\t:stim_plat(1)\n" << constr1 << "\t,stim(stim_plat)\n\t,plat(stim_plat" << constr2 << ")\n";
    out_file << "\t,mon(" << constr3 << ")\n\t{}\n};\n\nTop top;\nint main(int argc, char* argv[]) {\n\tTHREAD::START_ALL();\n\tTHREAD::JOIN_ALL();\n\tTHREAD::QUICK_EXIT(0);\n}";
}

int main() {
    cout << "<> Starting GCPP Writer\n";
    string line;

    // READ INPUT
    ifstream inputData("graph.txt");
    if (!inputData.is_open()) {
        cout << "<> Error opening file\n";
        return 0;
    }
    cout << ">> Reading graph.txt...\n";
    getline(inputData, line);
    int numNodes = stoi(line.substr(7));

    node nodes[numNodes];               // node data storage
    vector<edge> edges;                 // edge data storage
    vector<int> terminals;              // terminal node ID storage

    // grab nodes and edges
    for (int i = 0; i < numNodes; i++) {
        getline(inputData, line);
        int parseIndex = 1;                             // index currently evaluating
        int parseBreak = line.find_first_of("[");       // index of next break

        // grab source
        string sourceStr = line.substr(parseIndex, parseBreak - parseIndex);    // source node
        int source = stoi(sourceStr);
        
        nodes[source].ID = source;
        nodes[source].terminal = 0;

        // set next evaluation point, look for bracket
        parseIndex = line.find_first_of("V", parseBreak) + 1;
        parseBreak = line.find_first_of("[", parseIndex);
        
        // grab edges, if they exist
        while (parseBreak != (int)line.npos && parseIndex != 0) {                   // while information still remains: node connects to others
            // grab dest
            int dest = stoi(line.substr(parseIndex, parseBreak - parseIndex));  // dest node
            
            // create edge
            edge newEdge;
            newEdge.s = source;
            newEdge.d = dest;
            edges.push_back(newEdge);
            nodes[source].outEdges.push_back(newEdge);
            nodes[dest].inEdges.push_back(newEdge);

            // set next evaluation point, look for bracket
            parseIndex = line.find_first_of(",", parseBreak) + 2;
            parseBreak = line.find_first_of("[", parseIndex);
        }

        // set terminal if no outgoing edges
        if (nodes[source].outEdges.size() == 0) {
            nodes[source].terminal = 1;
            terminals.push_back(source);
        }
    }
    inputData.close();

    // PRODUCE OUTPUT
    cout << ">> Writing graph.cpp\n";
    ofstream out_file("graph.cpp");
    out_file << "// Guided C++ Representation of generated task graph: graph.txt\n#include <stdio.h>\n#include <stdlib.h>\n#include <array>\n#include \"lib/GPCC/GPCC.h\"\n";
    // Nodes
    writeNodes(out_file, nodes, numNodes);

    // DUT
    writeDUT(out_file, nodes, numNodes, edges);
    
    // DataIn and DataOut
    writeData(out_file);

    // Platform
    writePlatform(out_file, terminals);

    // Stimulus
    out_file << "class Stimulus: public MODULE {\n\tQUEUE_OUT<int> &output;\n\tint data = 0;\n\tpublic:\n";
    out_file << "\tvoid main(void) {\n\t\toutput.PUSH(data);\n\t\tprintf(\"<> Stimulus sent [\%d].\\n\", data);\n";
    out_file << "\t}\n\tStimulus(QUEUE_OUT<int> &output)\n\t\t: output(output)\n\t{\n\t\tTHREAD::CREATE(this);\n\t}\n};\n\n";
    
    // Monitor
    writeMonitor(out_file, terminals);

    // Top
    writeTop(out_file, terminals);

    cout << "<> Exiting...\n";
    out_file.close();
    return 0;
}