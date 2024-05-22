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

class Port:
	def __init__(self, Name, Interface, Type):
		self.Name = Name
		self.Interface_ = Interface # future: reference actual interface object
		self.Type = Type

	def Print(self):
		print("\tPORT %s<%s> %s" % (self.Interface_, self.Type, self.Name))

class ChannelInstance:
	def __init__(self, Name, Channel, Type):
		self.Name = Name
		self.Channel_ = Channel     # future: reference actual interface object
		self.Type = Type
		self.Parameters = None

	def Print(self):
		if self.Parameters:
			print("\tCHANNEL_INSTANCE %s<%s> %s(%s)"
				% (self.Channel_, self.Type, self.Name, ','.join(t for t in self.Parameters)))
		else:
			print("\tCHANNEL_INSTANCE %s<%s> %s" % (self.Channel_, self.Type, self.Name))

class ModuleInstance:
	def __init__(self, Name, Module_):
		self.Name = Name
		self.Module_ = Module_      # reference to actual module object
		self.PortMap = []           # list of references to actual targets
									# (targets are either ports or channel instances)

	def Print(self):
		print("\tMODULE_INSTANCE %s %s(%s)"
				% (self.Module_.Name, self.Name, ','.join(t.Name for t in self.PortMap)))

class Thread:
	def __init__(self, Name, Detached=False):
		self.Name = Name
		self.Detached = Detached

	def Print(self):
		print("\tTHREAD %s (%s)" % (self.Name, ("detached" if self.Detached else "joinable")))

class Module:
	class PushPop():
		def __init__(self, _type = None, channel = None, data = None):
			self.TypeInfo = _type
			self.ChannelInstance = channel
			self.DataInstance = data
		
		def AddPushPopParams(self, _type, channel, data):
			self.TypeInfo = _type
			self.ChannelInstance = channel
			self.DataInstance = data

	class Constructor():
		def __init__(self, code = None):
			if code is None:
				self.Code: list[str] = []
			else:
				self.Code: list[str] = code
	
	def __init__(self, Name: str):
		self.Name = Name
		self.Ports: list[Port] = []
		self.ChannelInstances: list[ChannelInstance]= []
		self.ModuleInstances: list[ModuleInstance] = []
		self.Threads: list[Thread] = []
		self.Code: list[str] = []
		self.PUSHnPOP: list[Module.PushPop] = []
		self.ConstructorInstance: Module.Constructor = Module.Constructor()  
	
	def Print(self):
		print("    MODULE", self.Name)
		for p in self.Ports:
			p.Print()
		for i in self.ChannelInstances:
			i.Print()
		for i in self.ModuleInstances:
			i.Print()
		for t in self.Threads:
			t.Print()
		print("\tCode: %d lines" % (len(self.Code)))

	def AddChannelParams(self, ChannelInstanceName, Parameters):
		i = next((i for i in self.ChannelInstances if i.Name==ChannelInstanceName), None)
		if not i:
			exit("ERROR: Module::AddChannelParams: channel instance \"%s\" not found" % ChannelInstanceName)
		i.Parameters = Parameters

	def AddPortMap(self, ModuleInstanceName, TargetNames):
		i = next((i for i in self.ModuleInstances if i.Name==ModuleInstanceName), None)
		if not i:
			exit("ERROR: Module::AddPortMap: module instance \"%s\" not found" % ModuleInstanceName)
		ports = i.Module_.Ports
		if len(TargetNames)!=len(ports):
			exit("ERROR: Module::AddPortMap: for module instance \"%s\", %d mappings given for %d ports"
					% (ModuleInstanceName, len(TargetNames), len(ports)))
		for p, n in zip(ports, TargetNames):
			target = next((i for i in self.ChannelInstances+self.Ports if i.Name==n), None)
			if not target:
				exit("ERROR: Module::AddPortMap: for module instance \"%s\", no such port or channel instance \"%s\""
					% (ModuleInstanceName, n))
			# future: type checking: ensure that 'target' actually implements interface of port 'p'
			i.PortMap.append(target)

class AppTaskGraph:
	def __init__(self, SourceFile=None):
		self.SourceFile = SourceFile
		self.Header = []
		self.GPCCincluded = False
		self.Modules: list[Module] = []
		self.InstanceTree = None
		self.Footer = []

	def Print(self):
		print("SOURCE: %s" % self.SourceFile)
		print("HEADER: %d lines" % (len(self.Header)))
		print("GPCC:  ", self.GPCCincluded)
		print("MODULES: %d" % (len(self.Modules)))
		for m in self.Modules:
			m.Print()
		if self.InstanceTree:
			print("INSTANCE TREE:")
			self.InstanceTree.Print()
		print("FOOTER: %d lines" % (len(self.Footer)))

	def AddModuleInstance(self, Parent, Name, ModuleName):
		m = next((m for m in self.Modules if m.Name==ModuleName), None)
		if not m:
			exit("ERROR: AppTaskGraph::AddModuleInstance: module \"%s\" not found" % ModuleName)
		Parent.ModuleInstances.append(ModuleInstance(Name, m))

	def SetInstanceTree(self, Name, ModuleName):
		m = next((m for m in self.Modules if m.Name==ModuleName), None)
		if not m:
			exit("ERROR: AppTaskGraph::SetInstanceTree: module \"%s\" not found" % ModuleName)
		self.InstanceTree = ModuleInstance(Name, m)

def replace_newlines_in_string(match):
	return match.group(0).replace(r'\n',r'\\n')