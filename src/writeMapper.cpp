#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace std;

typedef struct edge {
    int s, d;
} edge;

typedef struct node {
    int ID;
} node;

int main() {
    cout << "<> Starting Mapper Input Writer\n";
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

    node nodes[numNodes];
    vector<edge> edges;
    vector<int> terminals;
    string nodeText, edgeText;

    for (int i = 0; i < numNodes; i++) {
        // grab new line
        getline(inputData, line);
        int parseIndex = 1;
        int parseBreak = line.find_first_of("[");   // index of next break

        // grab source
        int source = stoi(line.substr(parseIndex, parseBreak - parseIndex));    // source node
        string sourceStr = to_string(source);

        // create node
        node newNode;
        newNode.ID = source;
        nodes[i] = newNode;

        parseIndex = parseBreak + 6;
        parseBreak = line.find_first_of("[", parseIndex);

        // determine terminals
        if (parseBreak == (int)line.npos) {terminals.push_back(source);}

        // grab edges, if they exist
        while (parseBreak != (int)line.npos) {
            // grab dest
            int dest = stoi(line.substr(parseIndex, parseBreak - parseIndex));  // dest node
            
            // create edge
            edge newEdge;
            newEdge.s = source;
            newEdge.d = dest;
            edges.push_back(newEdge);

            // update text
            nodeText += "\tG.add_node('V" + sourceStr + "', name='V" + sourceStr + "', targets={})\n";

            // find next breakpoint
            parseIndex = parseBreak + 5;
            parseBreak = line.find_first_of("[", parseIndex);
        }
    }
    inputData.close();

    // PRODUCE OUTPUT
    cout << ">> Writing mapper.py\n";
    ofstream py_file("mapper.py");

    // adding nodes
    py_file << "from mapperLib import *\n\npath = \"./input\"\n";
    py_file << "def main():\n\tprint(\">> This program only needs to run once to generate all necessary input\")\n";
    py_file << "\ttry:\n\t\tos.mkdir(path)\n\texcept OSError:\n\t\tprint(\">> Folder already exists, continuing...\")\n";
    py_file << "\tinputGraph()\n\tinputJPEG()\n\tinputInitialAPNG()\n\ndef inputGraph() -> (nx.classes.digraph.DiGraph, list):\n";
    py_file << "\tG = nx.DiGraph()\n\n\tG.add_node('Din', name='Din', offChip=True)\n\tG.add_node('Dout', name='Dout', offChip=True)\n\n" << nodeText;

    // adding edges
    py_file << "\n\tG.add_edge('Din', 'V0', type=['int'], name=['Din_V0'])\n\n";
    for (int j = 0; j < (int)edges.size(); j++) {
        string s_str = to_string(edges[j].s);
        string d_str = to_string(edges[j].d);
        py_file << "\tG.add_edge('V" + s_str + "', 'V" + d_str + "', type=['int'], name=['V" + s_str + "_V" + d_str + "'])\n";
        edgeText += "\t\t\t\t\t ('V" + s_str + "', 'V" + d_str + "'): 'V" + s_str + "_V" + d_str + "',\n";
    }
    py_file << "\n";
    for (int k = 0; k < (int)terminals.size(); k++) {
        string t_str = to_string(terminals[k]);
        py_file << "\tG.add_edge('V" + t_str + "', 'Dout', type=['int'], name=['Dout_V" + t_str + "'])\n";
        if (k < (int)terminals.size() - 1) {edgeText += "\t\t\t\t\t ('V" + t_str + "', 'Dout'): 'Dout_V" + t_str + "',\n";}
        else {edgeText += "\t\t\t\t\t ('V" + t_str + "', 'Dout'): 'Dout_V" + t_str + "'\n";}
    }

    // body
    py_file << "\n\tfile = os.path.join(path, \"graph.json\")\n\tsaveGraphToFile(G, file)\n\n\tplt.clf()\n";
    py_file << "\tfixed_pos = {'Din': (0.5,1),'Dout': (0.5, 0)}\n\tspring_pos = nx.spring_layout(G, pos=fixed_pos, fixed = ['Din', 'Dout'])\n\n";
    py_file << "\tfor node, pos in spring_pos.items():\n\t\tif node not in fixed_pos:\n\t\t\tspring_pos[node] = (pos[0],0.1+pos[1]%0.8)\n";
    py_file << "\tfixed_pos.update(spring_pos)\n\n\tcolorMap = []\n\tfor node in G.nodes:\n\t\tif node == 'Din' or node == 'Dout':\n\t\t\tcolorMap.append('yellow')\n\t\telse:\n\t\t\tcolorMap.append('lightblue')\n";
    py_file << "\tnx.draw(\n\t\tG, pos=fixed_pos, edge_color='black', width=1, linewidths=1,\n\t\tnode_size=500, node_color=colorMap, alpha=0.9,\n\t\tlabels={node: node for node in G.nodes()}\n\t)\n";
    py_file << "\tnx.draw_networkx_edge_labels(\n\t\tG, pos = fixed_pos,\n\t\tedge_labels={ ('Din', 'V0'): 'Din_V0',\n" << edgeText << "\t\t\t\t\t },\n\t\tfont_color='red'\n\t)\n";
    py_file << "\tfile = os.path.join(path, \"graph.json\")\n\tplt.show()\n\tplt.savefig(file)\n\treturn G\n";
    py_file << "\ndef inputInitialAPNG() -> nx.classes.digraph.DiGraph:  # tuple(nx.classes.digraph.DiGraph, list)\n\tG = nx.DiGraph()\n\n\tG.add_node('CS', name='CS', targets={}, startNode=True)\n\tG.add_node('SF', name='SF', targets={})\n";
    py_file << "\tG.add_node('AF', name='AF', targets={})\n\tG.add_node('UF', name='UF', targets={})\n\tG.add_node('PF', name='PF', targets={})\n\tG.add_node('Comparator', name='Comparator', targets={})\n\tG.add_node('Compressor', name='Compressor', targets={})\n";
    py_file << "\tG.add_node('Encoder', name='Encoder', targets={}, endNode=True)\n\n\tG.add_edges_from([('CS', 'SF'), ('CS', 'AF'), ('SF', 'UF'), ('UF', 'PF'), ('AF', 'Comparator'),\n\t\t\t\t\t  ('PF', 'Comparator'), ('Comparator', 'Compressor'), ('Compressor', 'Encoder')])";
    py_file << "\n\n\tfile = os.path.join(path, \"APNG.json\")\n\tsaveGraphToFile(G, file)\n\n\tplt.clf()\n\tpos = nx.spring_layout(G)\n\tnx.draw(\n\t\tG, pos, edge_color='black', width=1, linewidths=1,\n\t\tnode_size=500, node_color='pink', alpha=0.9,\n\t\tlabels={node: node for node in G.nodes()}";
    py_file << "\n\t)\n\tfile = os.path.join(path, \"APNG.png\")\n\tplt.savefig(file)\n\treturn G\n\ndef inputJPEG() -> nx.classes.digraph.DiGraph:  # tuple(nx.classes.digraph.DiGraph, list)\n\tG = nx.DiGraph()\n\n\tG.add_node('RGB2YC', name='RGB2YC', targets={}, startNode=True)";
    py_file << "\n\tG.add_node('DCTr', name='DCTr', targets={})\n\tG.add_node('DCTg', name='DCTg', targets={})\n\tG.add_node('DCTb', name='DCTb', targets={})\n\tG.add_node('Quantr', name='Quantr', targets={})\n\tG.add_node('Quantg', name='Quantg', targets={})\n\tG.add_node('Quantb', name='Quantb', targets={})";
    py_file << "\n\tG.add_node('Zigzagr', name='Zigzagr', targets={})\n\tG.add_node('Zigzagg', name='Zigzagg', targets={})\n\tG.add_node('Zigzagb', name='Zigzagb', targets={})\n\tG.add_node('Huffman', name='Huffman', targets={}, endNode=True)\n\n\tG.add_edges_from([('RGB2YC', 'DCTr'), ('RGB2YC', 'DCTg'), ('RGB2YC', 'DCTb'),";
    py_file << "\n\t\t\t\t\t  ('DCTr', 'Quantr'), ('DCTg', 'Quantg'), ('DCTb', 'Quantb'),\n\t\t\t\t\t  ('Quantr', 'Zigzagr'), ('Quantg', 'Zigzagg'), ('Quantb', 'Zigzagb'),\n\t\t\t\t\t  ('Zigzagr', 'Huffman'), ('Zigzagg', 'Huffman'), ('Zigzagb', 'Huffman'),\n\t\t\t\t\t  ])\n\t# G.add_edge('RGB2YC', 'DCTr', type='')";
    py_file << "\n\n\tfile = os.path.join(path, \"JPEG.json\")\n\tsaveGraphToFile(G, file)\n\n\tplt.clf()\n\tpos = nx.spring_layout(G)\n\tnx.draw(\n\t\tG, pos, edge_color='black', width=1, linewidths=1,\n\t\tnode_size=500, node_color='pink', alpha=0.9,\n\t\tlabels={node: node for node in G.nodes()}\n\t)";
    py_file << "\n\tfile = os.path.join(path, \"JPEG.png\")\n\tplt.savefig(file)\n\treturn G\n\nif __name__ == \"__main__\":\n\tmain()";
}