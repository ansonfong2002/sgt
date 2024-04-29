import json
import networkx as nx
import matplotlib.pyplot as plt

import matplotlib
import argparse
import os.path
import copy
import logging

matplotlib.use('TkAgg')

def saveGraphToFile(g: nx.classes.graph.Graph, fname: str):
	with open(fname, 'w') as f:
		json.dump(nx.node_link_data(g), f)


def readGraphFromFile(fname: str):
	with open(fname, 'r') as f:
		g_dict = json.load(f)

	G = nx.node_link_graph(g_dict)

	return G

class mappedGPCDrawer:
	# YW 24 Combined both draw function using optional kwds TODO
	# def drawMappedGPC(self, gpc: nx.classes.graph.Graph, **kwds):
	# 	plt.gca().invert_yaxis()
	# 	labels = nx.get_node_attributes(gpc, 'map')


	def drawMappedGPC(self, gpc: nx.classes.graph.Graph, fname: str, saveImage: bool):
		# plt.gca().cla() #use this to clear only the subfigure
		plt.gca().invert_yaxis()
		labels = nx.get_node_attributes(gpc, 'map')

		def makeGPCLabel(n):
			if labels[n] is not None:
				return labels[n]
			else:
				return ""

		gpcPos = nx.get_node_attributes(gpc, 'pos')
		nx.draw(gpc, gpcPos, labels={n: makeGPCLabel(n) for n in gpc.nodes()})
		if saveImage:
			plt.savefig(fname)
			print("Image is saved as ", fname)
		plt.show()

	

	def drawMappedGPC(self, gpc: nx.classes.graph.Graph):
		# plt.gca().cla() #use this to clear only the subfigure
		plt.gca().invert_yaxis()
		try:
			labels = nx.get_node_attributes(gpc, 'map')
		except KeyError:  #happens in L2 mem
			labels = ""

		print("labels: ", labels)

		def format_tuple(tuples_list):
			return '\n'.join([f"{tup[1]}" for tup in tuples_list])

		# make better labels... (got rid of data type)
		for key,value in labels.items():
			print("Value: ",value)
			if (key.startswith('M') or key.startswith('Off')) and value is not None:  #temporary, should probably check Mem=True instead of 'M' or 'Off'
				for tup in value:
					labels[key] = format_tuple(value)
			else:
				labels[key] = value

		def makeGPCLabel(n):
			if labels[n] is not None:
				return labels[n]
			else:
				return ""

		#recolor mems
		colorMap = []
		for node in gpc.nodes(data=True):
			if node[1]['Mem']:
				colorMap.append('green')
			else:
				colorMap.append('lightblue')


		gpcPos = nx.get_node_attributes(gpc, 'pos')
		nx.draw(gpc, gpcPos, labels={n: makeGPCLabel(n) for n in gpc.nodes()}, node_color=colorMap)
		#nx.draw(gpc, gpcPos, labels=labels, node_color=colorMap)
		plt.show()