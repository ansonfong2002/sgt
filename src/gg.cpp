#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <set>

using namespace std;

const double THRESHOLD = 0.3;   // probability of making new edge
const int SEED = 0;             // seed for random

class vertex {
    int id, outEdges, depth;
    public:
    vertex(int i, int d) : id(i), outEdges(0), depth(d) {
        cout << " > Generated V" << id << " [d = " << depth << "]\n";
    }
    int getID() const {return id;}
    int capacity(int limit) const {return limit - outEdges;}
    int getDepth() const {return depth;}
    void setDepth(int d) {depth = d;}
    void addEdge() {outEdges++;}
};

class edge {
    vertex in;
    vertex out;
    public:
    edge(vertex &v_in, vertex &v_out) : in(v_in), out(v_out) {
        cout << " > Connected: V" << v_in.getID() << " [d = " << in.getDepth() << "] -> V" << v_out.getID() << " [d = " << out.getDepth() <<"]\n";
    }
    int getV_in_ID() const {return in.getID();}
    int getV_out_ID() const {return out.getID();}
    int getV_out_d() const {return out.getDepth();}
    friend bool operator< (const edge &e1, const edge &e2);
};

bool operator< (const edge &e1, const edge &e2) {    // overloaded comparison to utilize sets
    if (e1.getV_in_ID() < e2.getV_in_ID()) {return true;}
    else if (e1.getV_in_ID() == e2.getV_in_ID() && e1.getV_out_ID() < e2.getV_out_ID()) {return true;}
    return false;
}

void makePath(vector<vertex> &vertices, set<edge> &edges, int height) {
    for (int i = 0; i < height - 1; i++) {
        edge newEdge(vertices[i], vertices[i + 1]);
        vertices[i].addEdge();
        edges.insert(newEdge);
    }
}

void attachExtras(vector<vertex> &vertices, vector<vertex> &extras, set<edge> &edges, int height, int max_num_out, int singular) {
    for (int i = 0; i < (int)extras.size(); i++) {
        bool connected = false;
        // attach extras to central vertices (EXTRA becomes depth [x + 1] VERTEX when connected to depth [x] VERTEX)
        int j = rand() % (height - 2); // random index, omit terminal node and preceding node
        while (!connected) {
            if (vertices[j].getDepth() < extras[i].getDepth() && vertices[j].capacity(max_num_out)) {    // checks [x] depth VERTEX for capacity
                extras[i].setDepth(vertices[j].getDepth() + 1);
                edge newEdge(vertices[j], extras[i]);
                edges.insert(newEdge);
                vertices[j].addEdge();
                vertices.push_back(extras[i]);
                if (singular) {
                    // all extra edges connect with terminal node
                    edge termEdge(vertices.back(), vertices[height - 1]);
                    edges.insert(termEdge);
                    vertices.back().addEdge();
                }
                connected = true;
            }
            j = (j == (int)vertices.size() - 1) ? 0 : j + 1;
        }
    }
}

void populate(vector<vertex> &vertices, set<edge> &edges, int max_num_out) {
    for (int i = 0; i < (int)vertices.size(); i++) {
        if ((rand() % 100) / 100.0 < THRESHOLD) {
            int index_1, index_2;
            do {
                index_1 = rand() % vertices.size();
                index_2 = rand() % vertices.size();
            } while (index_1 == index_2);
            if (vertices[index_1].getDepth() > vertices[index_2].getDepth() && vertices[index_2].capacity(max_num_out)) {
                edge newEdge(vertices[index_2], vertices[index_1]);
                edges.insert(newEdge);
                vertices[index_2].addEdge();
            } else if (vertices[index_2].getDepth() > vertices[index_1].getDepth() && vertices[index_1].capacity(max_num_out)){
                edge newEdge(vertices[index_1], vertices[index_2]);
                edges.insert(newEdge);
                vertices[index_1].addEdge();
            }
        }
    }
}

void output(vector<vertex> &vertices, set<edge> edges, int nodes, int max_num_out) {
    cout << ">> Producing output file: \"graph.txt\"...\n";
    ofstream outputFile("graph.txt");
    outputFile << "Nodes: " << nodes << "\n";
    vector<int> fileptrs(nodes);
    for (int i = 0; i < nodes; i++) {
        //outputFile << "V" << i << ": ";            // create line for each vertex
        outputFile << "V" << i << "[" << vertices[i].getDepth() << "]" << ": ";            // create line for each vertex
        fileptrs[i] = outputFile.tellp();     // log the write index for each vertex
        for (int k = 0; k < max_num_out; k++) {outputFile << "       ";}    // make editable spaces
        outputFile << "\n";
    }
    for (set<edge>::iterator j = edges.begin(); j != edges.end(); j++) {
        int source = j->getV_in_ID();
        int dest = j->getV_out_ID();
        outputFile.seekp(fileptrs[source]);
        outputFile << "V" << dest << "[" << j->getV_out_d() << "]" << ",";
        fileptrs[source] = outputFile.tellp();
    }
    outputFile.close();
}

int generate(int nodes, int height, int max_num_out, int singular) {
    vector<vertex> vertices;
    vector<vertex> extras;
    for (int i = 0; i < height; i++) {
        vertex newVertex(i, i);
        vertices.push_back(newVertex);
    }
    for (int j = height; j < nodes; j++) {
        vertex newVertex(j, height - 1);
        extras.push_back(newVertex);
    }
    set<edge> edges;
    cout << "<> Building root pathway...\n";
    makePath(vertices, edges, height);
    cout << "<> Randomly appending extras...\n";
    attachExtras(vertices, extras, edges, height, max_num_out, singular);
    cout << "<> Randomly populating edges...\n";
    populate(vertices, edges, max_num_out);
    output(vertices, edges, nodes, max_num_out);
    return 0;
}

void createConfig() {
    ofstream config("./config/gg.config");
    config << "// CONFIGURATION FILE (for gg.cpp)\nNodes (min 2): 5\nHeight: 3\nMaximum edges: 3\nSingular output [0/1]: 0";
    config.close();
}

int main() {
    srand(SEED);
    int nodes, height, max_num_out, singular;
    string line;
    cout << "<> Starting Graph Generator\n";
    ifstream userConfig("./config/gg.config");
    if (!userConfig.is_open()) {
        cout << "<> Error: missing gg.config\n>> Creating gg.config\n>> Try again?\n";
        createConfig();
        return 0;
    }
    cout << "<> Reading config/gg.config...\n";
    getline(userConfig, line);
    getline(userConfig, line);
    nodes = stoi(line.substr(15));
    getline(userConfig, line);
    height = stoi(line.substr(8));
    getline(userConfig, line);
    max_num_out = stoi(line.substr(15));
    getline(userConfig, line);
    singular = stoi(line.substr(23));
    userConfig.close();
    cout << ">> Nodes: " << nodes << ", Height: " << height << ", Maximum edges: " << max_num_out << ", Singular: " << singular << "\n";
    generate(nodes, height, max_num_out, singular);
    cout << "<> Exiting...\n";
    return 0;
}