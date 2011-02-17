#pragma once

using namespace boost;

namespace zzub {

struct connection;

// plugin vertex properties
struct vertex_props {
	int id;
};

// connection properties
struct edge_props {
	connection* conn;
};

// the graph type
typedef adjacency_list<vecS, vecS, bidirectionalS, vertex_props, edge_props > plugin_map;

// various graph helper types
typedef graph_traits<plugin_map>::vertex_descriptor plugin_descriptor;
typedef graph_traits<plugin_map>::vertex_iterator plugin_iterator;
typedef graph_traits<plugin_map>::edge_descriptor connection_descriptor;
typedef graph_traits<plugin_map>::vertices_size_type size_type;
typedef graph_traits<plugin_map>::out_edge_iterator out_edge_iterator;
typedef graph_traits<plugin_map>::in_edge_iterator in_edge_iterator;

};
