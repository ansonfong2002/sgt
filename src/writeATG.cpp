#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace std;

int parseNode(ifstream &input, ofstream &output, int mode) {
    string line, container, queue, name;
    int parseIndex, parseBreak;

    // Parsing Monitor (mode = 1) or Node (mode = 0)
    char cond = mode ? 'M' : 'V';
    
    // get module name
    getline(input, line);
    parseBreak = line.find_first_of(":");
    name = line.substr(6, parseBreak - 6);

    // Mode 0: exit if non general node (DUT, Platform, etc)
    if (!mode && name[0] != 'V') {return 1;}

    output << "m = Module(\"" << name << "\")\n";
    getline(input, line);

    // input/output ports
    parseIndex = line.find_first_of("Q");
    while (parseIndex != (int)line.npos) {
        // queue type
        parseBreak = line.find_first_of("<");
        queue = line.substr(parseIndex, parseBreak - parseIndex);
        
        // queue name
        parseIndex = line.find_first_of("&") + 1;
        parseBreak = line.find_first_of(";");
        name = line.substr(parseIndex, parseBreak - parseIndex);

        output << "p = Port(\"" << name << "\", \"" << queue << "\", \"int\")\n";
        output << "m.Ports.append(p)\n";
        getline(input, line);
        parseIndex = line.find_first_of("Q");
    }
    
    // public
    while (line[1] != cond) {
        container += line + "\n";
        getline(input, line);
    }
    output << "m.Code.extend(\nr\"\"\"\n" << container << "\"\"\".splitlines())\n";

    // constructor
    container = "";
    while (line[0] != '}') {
        // constructor raw code
        container += line + "\n";
        getline(input, line);
    }
    output << "m.ConstructorInstance.Code.extend(\nr\"\"\"\n" << container << "\"\"\".splitlines())\n";
    output << "t = Thread(\"main\", True);\nm.Threads.append(t)\nApp.Modules.append(m)\n\n";
    getline(input, line);
    return 0;
}

void parseDUT(ifstream &input, ofstream &output) {
    string line, container, queue, name;
    int parseIndex, parseBreak;
    
    // module name
    getline(input, line);
    output << "m = Module(\"DUT\")\n";

    // input/output ports
    parseIndex = line.find_first_of("Q");
    while (parseIndex != (int)line.npos) {
        // queue type
        parseBreak = line.find_first_of("<");
        queue = line.substr(parseIndex, parseBreak - parseIndex);

        // queue name
        parseIndex = line.find_first_of("&") + 1;
        parseBreak = line.find_first_of(";");
        name = line.substr(parseIndex, parseBreak - parseIndex);

        output << "p = Port(\"" << name << "\", \"" << queue << "\", \"int\")\n";
        output << "m.Ports.append(p)\n";
        getline(input, line);
        parseIndex = line.find_first_of("Q");
    }

    // channels
    getline(input, line);
    parseIndex = line.find_first_of("V");
    parseBreak = line.find_first_of(",");

    while (parseBreak != (int)line.npos) {
        name = line.substr(parseIndex, parseBreak - parseIndex);
        output << "i = ChannelInstance(\"" << name << "\", \"QUEUE\", \"int\")\nm.ChannelInstances.append(i)\n";
        parseIndex = parseBreak + 2;
        parseBreak = line.find_first_of(",", parseIndex);
    }
    parseBreak = line.find_first_of(";");
    name = line.substr(parseIndex, parseBreak - parseIndex);
    output << "i = ChannelInstance(\"" << name << "\", \"QUEUE\", \"int\")\nm.ChannelInstances.append(i)\n";

    // module instances
    getline(input, line);
    parseIndex = line.find_first_of("V");
    parseBreak = 0;
    while (parseIndex != (int)line.npos) {
        output << "App.AddModuleInstance(m, \"v" << parseBreak << "\", \"V" << parseBreak << "\")\n";
        getline(input, line);
        parseIndex = line.find_first_of("V");
        parseBreak++;
    }

    // public
    output << "m.Code.extend(\nr\"\"\"\n\tpublic:\n\"\"\".splitlines())\n";
    getline(input, line);
    getline(input, line);
    
    // constructor
    queue = "";
    while (line[0] != '}') {
        // constructor raw code
        container += line + "\n";

        // update mapping
        parseIndex = line.find_first_of(",");
        parseBreak = line.find_first_of("v");
        if (parseIndex == parseBreak - 1) {
            parseIndex = parseBreak;
            parseBreak = line.find_first_of("(");
            name = line.substr(parseIndex, parseBreak - parseIndex);
            queue += "m.AddPortMap(\"" + name + "\", [";

            parseIndex = parseBreak + 1;
            parseBreak = line.find_first_of(",", parseIndex);

            while (parseBreak != (int)line.npos) {
                name = line.substr(parseIndex, parseBreak - parseIndex);
                queue += "\""  + name + "\", ";
                parseIndex = parseBreak + 2;
                parseBreak = line.find_first_of(",", parseIndex);
            }
            parseBreak = line.find_first_of(")");
            name = line.substr(parseIndex, parseBreak - parseIndex);
            queue += "\"" + name + "\"])\n";
        }
        getline(input, line);
    }
    output << "m.ConstructorInstance.Code.extend(\nr\"\"\"\n" << container << "\"\"\".splitlines())\n";
    output << queue << "App.Modules.append(m)\n";
    getline(input, line);
}

void parseData(ofstream &output) {
    output << "\nm = Module(\"DataIn\")\np = Port(\"input\", \"QUEUE_IN\", \"int\")\nm.Ports.append(p)\np = Port(\"output\", \"QUEUE_OUT\", \"int\")\nm.Ports.append(p)\nm.Code.extend(\nr\"\"\"";
    output << "\n\tint data;\n\tpublic:\n\tvoid main() {\n\t\twhile (1) {\n\t\t\tinput.POP(data);\n\t\t\toutput.PUSH(data);\n\t\t}\n\t}\n\"\"\".splitlines())\nm.ConstructorInstance.Code.extend(\nr\"\"\"";
    output << "\n\tDataIn(QUEUE_IN<int> &input, QUEUE_OUT<int> &output)\n\t\t: input(input), output(output)\n\t{\n\t\tTHREAD::DETACH(this);\n\t}\n\"\"\".splitlines())\nt = Thread(\"main\", True)\nm.Threads.append(t)\nApp.Modules.append(m)\n";
    output << "\nm = Module(\"DataOut\")\np = Port(\"input\", \"QUEUE_IN\", \"int\")\nm.Ports.append(p)\np = Port(\"output\", \"QUEUE_OUT\", \"int\")\nm.Ports.append(p)\nm.Code.extend(\nr\"\"\"";
    output << "\n\tint data;\n\tpublic:\n\tvoid main() {\n\t\twhile (1) {\n\t\t\tinput.POP(data);\n\t\t\toutput.PUSH(data);\n\t\t}\n\t}\n\"\"\".splitlines())\nm.ConstructorInstance.Code.extend(\nr\"\"\"";
    output << "\n\tDataOut(QUEUE_IN<int> &input, QUEUE_OUT<int> &output)\n\t\t: input(input), output(output)\n\t{\n\t\tTHREAD::DETACH(this);\n\t}\n\"\"\".splitlines())\nt = Thread(\"main\", True)\nm.Threads.append(t)\nApp.Modules.append(m)\n\n";
}

void parsePlat(ifstream &input, ofstream &output) {
    string line, container, queue, name, params;
    int parseIndex, parseBreak;
    
    // module name
    getline(input, line);
    output << "m = Module(\"Platform\")\n";
    
    // input/output ports
    getline(input, line);
    parseIndex = line.find_first_of("Q");
    while (parseIndex != (int)line.npos) {
        // queue type
        parseBreak = line.find_first_of("<");
        queue = line.substr(parseIndex, parseBreak - parseIndex);

        // queue name
        parseIndex = line.find_first_of("&") + 1;
        parseBreak = line.find_first_of(";");
        name = line.substr(parseIndex, parseBreak - parseIndex);

        output << "p = Port(\"" << name << "\", \"" << queue << "\", \"int\")\n";
        output << "m.Ports.append(p)\n";
        getline(input, line);
        parseIndex = line.find_first_of("Q");
    }

    // channels
    getline(input, line);
    output << "i = ChannelInstance(\"din_dut\", \"QUEUE\", \"int\")\nm.ChannelInstances.append(i)\n";
    getline(input, line);
    parseIndex = line.find_first_of("Q");
    while (parseIndex != (int)line.npos) {
        parseIndex = line.find_first_of("d");
        parseBreak = line.find_first_of(";");
        name = line.substr(parseIndex, parseBreak - parseIndex);
        output << "i = ChannelInstance(\"" << name << "\", \"QUEUE\", \"int\")\nm.ChannelInstances.append(i)\n";
        params += "m.AddChannelParams(\"" + name + "\", [\"1\"])\n";

        getline(input, line);
        parseIndex = line.find_first_of("Q");
    }

    // module instances
    output << "App.AddModuleInstance(m, \"din\", \"DataIn\")\nApp.AddModuleInstance(m, \"dut\", \"DUT\")\n";
    getline(input, line);
    getline(input, line);
    parseIndex = line.find_first_of("V");
    while (parseIndex != (int)line.npos) {
        parseBreak = line.find_first_of(";");
        name = line.substr(parseIndex, parseBreak - parseIndex);
        output << "App.AddModuleInstance(m, \"" << name << "\", \"DataOut\")\n";

        getline(input, line);
        parseIndex = line.find_first_of("V");
    }

    // public
    output << "m.Code.extend(\nr\"\"\"\n\tpublic:\n\"\"\".splitlines())\n";
    getline(input, line);
    getline(input, line);

    // constructor
    queue = "";
    while (line[0] != '}') {
        // constructor raw code
        container += line + "\n";

        // update mappings
        parseIndex = line.find_first_of(",");
        parseBreak = line.find_first_of("d");
        if (parseBreak > 0 && line.substr(parseBreak, parseBreak + 2) == "dut(") {
            // DUT to DataOut
            parseIndex = parseBreak;
            parseBreak = line.find_first_of("(");
            name = line.substr(parseIndex, parseBreak - parseIndex);
            queue += "m.AddPortMap(\"" + name + "\", [";

            parseIndex = parseBreak + 1;
            parseBreak = line.find_first_of(",", parseIndex);

            while (parseBreak != (int)line.npos) {
                name = line.substr(parseIndex, parseBreak - parseIndex);
                queue += "\""  + name + "\", ";
                parseIndex = parseBreak + 2;
                parseBreak = line.find_first_of(",", parseIndex);
            }
            parseBreak = line.find_first_of(")");
            name = line.substr(parseIndex, parseBreak - parseIndex);
            queue += "\"" + name + "\"])\n";
        }
        parseBreak = line.find_first_of("V");
        if (parseIndex == parseBreak - 1) {
            // DataOut to output
            parseIndex = parseBreak;
            parseBreak = line.find_first_of("(");
            name = line.substr(parseIndex, parseBreak - parseIndex);
            queue += "m.AddPortMap(\"" + name + "\", [";

            parseIndex = parseBreak + 1;
            parseBreak = line.find_first_of(",", parseIndex);

            while (parseBreak != (int)line.npos) {
                name = line.substr(parseIndex, parseBreak - parseIndex);
                queue += "\""  + name + "\", ";
                parseIndex = parseBreak + 2;
                parseBreak = line.find_first_of(",", parseIndex);
            }
            parseBreak = line.find_first_of(")");
            name = line.substr(parseIndex, parseBreak - parseIndex);
            queue += "\"" + name + "\"])\n";
        }
        getline(input, line);
    }
    output << "m.ConstructorInstance.Code.extend(\nr\"\"\"\n" << container << "\"\"\".splitlines())\n";
    output << "m.AddChannelParams(\"din_dut\", [\"1\"])\n" << params;
    output << "m.AddPortMap(\"din\", [\"input\", \"din_dut\"])\n" << queue << "App.Modules.append(m)\n\n";
    getline(input, line);
}

void parseStim(ofstream &output) {
    output << "m = Module(\"Stimulus\")\np = Port(\"output\", \"QUEUE_OUT\", \"int\")\nm.Ports.append(p)\nm.Code.extend(\nr\"\"\"\n\tint data = 0;\n\tpublic:";
    output << "\n\tvoid main(void) {\n\t\toutput.PUSH(data);\n\t\tprintf(\"<> Stimulus sent [\%d].\\n\", data);\n\t}\n\"\"\".splitlines())\nm.ConstructorInstance.Code.extend(\nr\"\"\"";
    output << "\n\tStimulus(QUEUE_OUT<int> &output)\n\t\t: output(output)\n\t{\n\t\tTHREAD::CREATE(this);\n\t}";
    output << "\n\"\"\".splitlines())\nt = Thread(\"main\")\nm.Threads.append(t)\nApp.Modules.append(m)\n\n";
}

void parseTop(ifstream &input, ofstream &output) {
    string line, container, queue, name, params;
    int parseIndex, parseBreak;
    
    // module name
    getline(input, line);
    output << "m = Module(\"Top\")\n";
    
    // channels
    getline(input, line);
    output << "i = ChannelInstance(\"stim_plat\", \"QUEUE\", \"int\")\nm.ChannelInstances.append(i)\n";
    getline(input, line);
    parseIndex = line.find_first_of("Q");
    while (parseIndex != (int)line.npos) {
        parseIndex = line.find_first_of("p");
        parseBreak = line.find_first_of(";");
        name = line.substr(parseIndex, parseBreak - parseIndex);
        output << "i = ChannelInstance(\"" << name << "\", \"QUEUE\", \"int\")\nm.ChannelInstances.append(i)\n";
        params += "m.AddChannelParams(\"" + name + "\", [\"1\"])\n";

        getline(input, line);
        parseIndex = line.find_first_of("Q");
    }

    for (int i = 0; i < 3; i++) {getline(input, line);}
    output << "App.AddModuleInstance(m, \"stim\", \"Stimulus\")\nApp.AddModuleInstance(m, \"plat\", \"Platform\")\nApp.AddModuleInstance(m, \"mon\", \"Monitor\")\n";
    
    // public
    output << "m.Code.extend(\nr\"\"\"\n\tpublic:\n\"\"\".splitlines())\n";
    getline(input, line);
    getline(input, line);

    // constructor
    queue = "";
    while (line[0] != '}') {
        // constructor raw code
        container += line + "\n";

        // update mappings
        parseIndex = line.find_first_of(",");
        parseBreak = line.find_first_of("p");
        if ((parseIndex == parseBreak - 1) && line.substr(parseBreak, parseBreak + 3) == "plat(") {
            // platform to monitor
            parseIndex = parseBreak;
            parseBreak = line.find_first_of("(");
            name = line.substr(parseIndex, parseBreak - parseIndex);
            queue += "m.AddPortMap(\"" + name + "\", [";

            parseIndex = parseBreak + 1;
            parseBreak = line.find_first_of(",", parseIndex);

            while (parseBreak != (int)line.npos) {
                name = line.substr(parseIndex, parseBreak - parseIndex);
                queue += "\""  + name + "\", ";
                parseIndex = parseBreak + 2;
                parseBreak = line.find_first_of(",", parseIndex);
            }
            parseBreak = line.find_first_of(")");
            name = line.substr(parseIndex, parseBreak - parseIndex);
            queue += "\"" + name + "\"])\n";
        }
        parseBreak = line.find_first_of("m");
        if (parseIndex == parseBreak - 1) {
            // Monitor to platform
            parseIndex = parseBreak;
            parseBreak = line.find_first_of("(");
            name = line.substr(parseIndex, parseBreak - parseIndex);
            queue += "m.AddPortMap(\"" + name + "\", [";

            parseIndex = parseBreak + 1;
            parseBreak = line.find_first_of(",", parseIndex);

            while (parseBreak != (int)line.npos) {
                name = line.substr(parseIndex, parseBreak - parseIndex);
                queue += "\""  + name + "\", ";
                parseIndex = parseBreak + 2;
                parseBreak = line.find_first_of(",", parseIndex);
            }
            parseBreak = line.find_first_of(")");
            name = line.substr(parseIndex, parseBreak - parseIndex);
            queue += "\"" + name + "\"])\n";
        }
        getline(input, line);
    }
    output << "m.ConstructorInstance.Code.extend(\nr\"\"\"\n" << container << "\"\"\".splitlines())\n";
    output << "m.AddChannelParams(\"stim_plat\", [\"1\"])\n" << params;
    output << "m.AddPortMap(\"stim\", [\"stim_plat\"])\n" << queue << "App.Modules.append(m)\n\n";
    getline(input, line);
}

int main() {
    cout << "<> Starting ATG Writer\n";
    string line;
    int type;
    // INPUT STREAM
    ifstream inputFile("graph.cpp");
    if (!inputFile.is_open()) {
        cout << "<> Error opening file\n";
        return 0;
    }

    // OUTPUT STREAM
    ofstream outputFile("ATG_parser.py");

    // print main function
    outputFile << "from mapperLib import *\n\ndef main():\n\tprint(App)\n\tApp.Print()\n\tunparse(App)\n\texit()\n";
    outputFile << "\nApp = AppTaskGraph(\"graph.cpp\")\n\nApp.Header.extend(\nr\"\"\"\n// GUIDED C++ Representation of generated task graph: graph.txt";
    outputFile << "\n#include <stdio.h>\n#include <stdlib.h>\n#include <array>\n\"\"\".splitlines())\n\nApp.GPCCincluded = True\n\n";

    // skip header (constant)
    cout << ">> Reading graph.cpp...\n";
    for (int i = 0; i < 5; i++) {getline(inputFile, line);}

    // parse all nodes
    do {type = parseNode(inputFile, outputFile, 0);}
    while (type == 0);

    // parse DUT
    parseDUT(inputFile, outputFile);

    // print DataIn and DataOut (constant)
    parseData(outputFile);
    for (int i = 0; i < 36; i++) {getline(inputFile, line);}

    // parse Platform
    parsePlat(inputFile, outputFile);

    // print Stimulus (constant)
    parseStim(outputFile);
    for (int i = 0; i < 15; i++) {getline(inputFile, line);}
    
    // parse Monitor
    parseNode(inputFile, outputFile, 1);

    // parse Top
    parseTop(inputFile, outputFile);

    // print remainder sections
    outputFile << "App.SetInstanceTree(\"top\", \"Top\")\nApp.Footer.extend(\nr\"\"\"\nint main(int argc, char* argv[]) {\n\tTHREAD::START_ALL();";
    outputFile << "\n\tTHREAD::JOIN_ALL();\n\tTHREAD::QUICK_EXIT(0);\n}\n\"\"\".splitlines())\n\ndef unparse(g: AppTaskGraph):";
    outputFile << "\n\t#deparse start\n\tif not isinstance(g, AppTaskGraph):\n\t\traise TypeError(\"inputgraph for unparse must be AppTaskGraph Type\")\n";
    outputFile << "\n\tfname = g.SourceFile.split(\".\")[0] + \"_unparse.cpp\"\n\tf = open(fname, \"w\")\n\n\t#lib\n\tif g.GPCCincluded:\n\t\tprint(\"#include \\\"GPCC.h\\\"\", file=f)\n\t#header";
    outputFile << "\n\tfor line in g.Header:\n\t\tprint(line, file=f)\n\t#modules\n\tfor module in g.Modules:\n\t\t#class";
    outputFile << "\n\t\tprint(f\"class {module.Name}: public MODULE\\n{{\", file=f)\n\t\t#port\n\t\tfor port in module.Ports:\n\t\t\tprint(f\"\t{port.Interface_}<{port.Type}> &{port.Name};\", file=f)";
    outputFile << "\n\t\t#channel\n\t\tfor channel in module.ChannelInstances:\n\t\t\tprint(f\"\t{channel.Channel_}<{channel.Type}> {channel.Name};\", file=f)";
    outputFile << "\n\t\t#module\n\t\tfor m in module.ModuleInstances:\n\t\t\tprint(f\"\t{m.Module_.Name} {m.Name};\",file=f)\n\t\t#code";
    outputFile << "\n\t\tfor line in module.Code:\n\t\t\tprint(line, file=f)\n\t\t#constructor\n\t\tfor line in module.ConstructorInstance.Code:\n\t\t\tprint(line, file=f)\n\t\t#class close";
    outputFile << "\n\t\tprint('};\\n', file=f)\n\t#Instance Tree\n\tprint(f\"{g.InstanceTree.Module_.Name} {g.InstanceTree.Name};\", file=f)\n\t#footer code";
    outputFile << "\n\tfor line in g.Footer:\n\t\tprint(line, file=f)\n\t#done deparse\n\tf.close()\n\nif __name__ == \"__main__\":\n\tmain()\n# EOF";
    return 0;
}