// SPDX-License-Identifier: MIT

#include "binaryArr.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <iterator>

int main(int argc, char **argv) {
  if (argc < 4) {
    std::cout << "usage: binaryArr <VertexArray> <EdgeArray> <BinaryArray Prefix>" << std::endl;
    return -1;
  }

  std::ifstream varrin(argv[1]);
  std::ifstream earrin(argv[2]);
  std::vector<Vertex> varr;
  std::vector<Edge> earr;
  std::ofstream varrout(std::string(argv[3]) + "_V.bin", std::ios::binary);
  std::ofstream earrout(std::string(argv[3]) + "_E.bin", std::ios::binary);

  {
    std::string buf;
    while(std::getline(varrin, buf) && !buf.empty()) {
      Vertex v;
      std::stringstream ss(buf);
      ss >> v.id >> v.edges >> v.start >> v.type;
      varr.push_back(v);
    }

    while(std::getline(earrin, buf) && !buf.empty()) {
      Edge e;
      std::stringstream ss(buf);
      ss >> e.src >> e.dst >> e.type >> e.src_type >> e.dst_type >> e.src_glbid >> e.dst_glbid;
      earr.push_back(e);
    }
  }

  size_t numVertices = varr.size(), numEdges = earr.size();
  std::cout << "number of vertices: " << numVertices << std::endl;
  std::cout << "number of edges: " << numEdges << std::endl;


  varrout.write(reinterpret_cast<char *>(&numVertices), sizeof(numVertices));
  varrout.write(reinterpret_cast<char *>(varr.data()), numVertices * sizeof(Vertex));
  size_t varrBytes = sizeof(numVertices) + numVertices * sizeof(Vertex);

  earrout.write(reinterpret_cast<char *>(&numEdges), sizeof(numEdges));
  earrout.write(reinterpret_cast<char *>(earr.data()), numEdges * sizeof(Edge));
  size_t earrBytes = sizeof(numEdges) + numEdges * sizeof(Edge);

  std::cout << "vertex binary array size: " << varrBytes << std::endl;
  std::cout << "edge binary array size: " << earrBytes << std::endl;

  {
    std::ifstream earrin(std::string(argv[3])+"_E.bin", std::ios::binary);
    size_t size;
    earrin.read(reinterpret_cast<char *>(&size), sizeof(size));
    std::cout << size << std::endl;
    std::vector<unsigned char> buffer(size * sizeof(Edge));
    earrin.read(reinterpret_cast<char *>(buffer.data()), sizeof(Edge) * size);
    for (int i = 0; i < 10; i++) {
      std::cout << (int) buffer[i] << " ";
    }

  }

}
