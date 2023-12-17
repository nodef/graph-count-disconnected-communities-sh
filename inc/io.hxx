#pragma once
#include <cstdint>
#include <utility>
#include <string>
#include <istream>
#include <sstream>
#include <algorithm>
#include "_main.hxx"
#include "Graph.hxx"
#include "update.hxx"
#ifdef OPENMP
#include <omp.h>
#endif

using std::tuple;
using std::string;
using std::istream;
using std::ostream;
using std::istringstream;
using std::move;
using std::min;
using std::max;
using std::replace;
using std::getline;
using std::to_string;




#pragma region METHODS
#pragma region READ COO FORMAT HEADER
/**
 * Read header of COO format file.
 * @param rows number of rows (output)
 * @param cols number of columns (output)
 * @param size number of lines/edges (output)
 * @param stream input stream
 */
inline void readCooFormatHeaderW(size_t& rows, size_t& cols, size_t &size, istream& stream) {
  string line;
  // Skip past empty lines and comments.
  while (getline(stream, line)) {
    auto fu = [](char c) { return false; };
    auto it = findNextNonBlank(line.begin(), line.end(), fu);
    if ( it==line.end() || *it=='\n') continue;
    if (*it!='%' || *it!='#') break;
  }
  // Read rows, cols, size.
  istringstream lstream(line);
  lstream >> rows >> cols >> size;
}
#pragma endregion




#pragma region READ MTX FORMAT HEADER
/**
 * Read header of MTX format file.
 * @param symmetric is graph symmetric (output)
 * @param rows number of rows (output)
 * @param cols number of columns (output)
 * @param size number of lines/edges (output)
 * @param stream input stream
 */
inline void readMtxFormatHeader(bool& symmetric, size_t& rows, size_t& cols, size_t& size, istream& stream) {
  string line, h0, h1, h2, h3, h4;
  // Skip past empty lines and comments, and read the graph type.
  while (getline(stream, line)) {
    if (line.find('%')!=0) break;
    if (line.find("%%")!=0) continue;
    istringstream lstream(line);
    lstream >> h0 >> h1 >> h2 >> h3 >> h4;
  }
  if (h1!="matrix" || h2!="coordinate") throw FormatError("Invalid MTX header (unknown format)");
  symmetric = h4=="symmetric" || h4=="skew-symmetric";
  // Read rows, cols, size.
  istringstream lstream(line);
  lstream >> rows >> cols >> size;
}
#pragma endregion




#pragma region READ EDGELIST FORMAT
/**
 * Read a file in Edgelist format (also supports CSV and TSV).
 * @tparam WEIGHTED is graph weighted?
 * @param stream input stream
 * @param symmetric is graph symmetric?
 * @param fb on body line (u, v, w)
 */
template <bool WEIGHTED=false, class FB>
inline void readEdgelistFormatDo(istream& stream, bool symmetric, FB fb) {
  string line;
  while (getline(stream, line)) {
    size_t u, v; double w = 1;
    replace(line.begin(), line.end(), ',', ' ');
    istringstream lstream(line);
    if (!(lstream >> u >> v)) break;
    if (WEIGHTED) lstream >> w;
    fb(u, v, w);
    if (symmetric) fb(v, u, w);
  }
}


#ifdef OPENMP
/**
 * Read a file in Edgelist format (also supports CSV and TSV).
 * @tparam WEIGHTED is graph weighted?
 * @param stream input stream
 * @param symmetric is graph symmetric?
 * @param fb on body line (u, v, w)
 */
template <bool WEIGHTED=false, class FB>
inline void readEdgelistFormatDoOmp(istream& stream, bool symmetric, FB fb) {
  const int THREADS = omp_get_max_threads();
  const int LINES   = 131072;
  vector<string> lines(LINES);
  vector<tuple<size_t, size_t, double>> edges(LINES);
  while (true) {
    // Read several lines from the stream.
    int READ = 0;
    for (int i=0; i<LINES; ++i, ++READ)
      if (!getline(stream, lines[i])) break;
    if (READ==0) break;
    // Parse lines using multiple threads.
    #pragma omp parallel for schedule(dynamic, 1024)
    for (int i=0; i<READ; ++i) {
      replace(lines[i].begin(), lines[i].end(), ',', ' ');
      char *line = (char*) lines[i].c_str();
      size_t u = strtoull(line, &line, 10);
      size_t v = strtoull(line, &line, 10);
      double w = WEIGHTED? strtod(line, &line) : 0;
      edges[i] = {u, v, WEIGHTED? w : 1};
    }
    // Notify parsed lines.
    #pragma omp parallel
    {
      for (int i=0; i<READ; ++i) {
        const auto& [u, v, w] = edges[i];
        fb(u, v, w);
        if (symmetric) fb(v, u, w);
      }
    }
  }
}
#endif
#pragma endregion




#pragma region READ GRAPH (EDGELIST, COO, MTX FORMATS)
/**
 * Read a file in Edgelist format as a graph.
 * @tparam WEIGHTED is graph weighted?
 * @param a output graph (output)
 * @param stream input stream
 * @param symmetric is graph symmetric?
 * @param span maximum number of vertices (0 for no limit)
 */
template <bool WEIGHTED=false, class G>
inline void readGraphEdgelistFormatW(G& a, istream& stream, bool symmetric, size_t span=0) {
  using K = typename G::key_type;
  using E = typename G::edge_value_type;
  auto fb = [&](auto u, auto v, auto w) { a.addEdge(K(u), K(v), E(w)); };
  a.clear();
  if (span) a.respan(span);
  readEdgelistFormatDo<WEIGHTED>(stream, symmetric, fb);
  a.update();
}


#ifdef OPENMP
/**
 * Read a file in Edgelist format as a graph.
 * @tparam WEIGHTED is graph weighted?
 * @param a output graph (output)
 * @param stream input stream
 * @param symmetric is graph symmetric?
 * @param span maximum number of vertices (0 for no limit)
 */
template <bool WEIGHTED=false, class G>
inline void readGraphEdgelistFormatOmpW(G& a, istream& stream, bool symmetric=false, size_t span=0) {
  using K = typename G::key_type;
  using E = typename G::edge_value_type;
  auto fb = [&](auto u, auto v, auto w) { addEdgeOmpU(a, K(u), K(v), E(w)); };
  a.clear();
  if (span) a.respan(span);
  readEdgelistFormatDoOmp<WEIGHTED>(stream, symmetric, fb);
  updateOmpU(a);
}
#endif


/**
 * Read a file in COO format as a graph.
 * @tparam WEIGHTED is graph weighted?
 * @param a output graph (output)
 * @param stream input stream
 * @param symmetric is graph symmetric?
 */
template <bool WEIGHTED=false, class G>
inline void readGraphCooFormatW(G& a, istream& stream, bool symmetric=false) {
  using K = typename G::key_type;
  using E = typename G::edge_value_type;
  auto fb = [&](auto u, auto v, auto w) { a.addEdge(K(u), K(v), E(w)); };
  size_t rows = 0, cols = 0, size = 0;
  a.clear();
  readCooFormatHeaderW(rows, cols, size, stream);
  addVerticesU(a, K(1), K(max(rows, cols) + 1));
  readEdgelistFormatDo<WEIGHTED>(stream, symmetric, fb);
  a.update();
}


#ifdef OPENMP
/**
 * Read a file in COO format as a graph.
 * @tparam WEIGHTED is graph weighted?
 * @param a output graph (output)
 * @param stream input stream
 * @param symmetric is graph symmetric?
 */
template <bool WEIGHTED=false, class G>
inline void readGraphCooFormatOmpW(G& a, istream& stream, bool symmetric=false) {
  using K = typename G::key_type;
  using E = typename G::edge_value_type;
  auto fb = [&](auto u, auto v, auto w) { addEdgeOmpU(a, K(u), K(v), E(w)); };
  size_t rows = 0, cols = 0, size = 0;
  a.clear();
  readCooFormatHeaderW(rows, cols, size, stream);
  addVerticesU(a, K(1), K(max(rows, cols) + 1));
  readEdgelistFormatDoOmp<WEIGHTED>(stream, symmetric, fb);
  updateOmpU(a);
}
#endif


/**
 * Read a file in MTX format as a graph.
 * @tparam WEIGHTED is graph weighted?
 * @param a output graph (output)
 * @param stream input stream
 */
template <bool WEIGHTED=false, class G>
inline void readGraphMtxFormatW(G& a, istream& stream) {
  using K = typename G::key_type;
  using E = typename G::edge_value_type;
  auto fb = [&](auto u, auto v, auto w) { a.addEdge(K(u), K(v), E(w)); };
  bool symmetric = false;
  size_t rows = 0, cols = 0, size = 0;
  a.clear();
  readMtxFormatHeader(symmetric, rows, cols, size, stream);
  addVerticesU(a, K(1), K(max(rows, cols) + 1));
  readEdgelistFormatDo<WEIGHTED>(stream, symmetric, fb);
  a.update();
}


#ifdef OPENMP
/**
 * Read a file in MTX format as a graph.
 * @tparam WEIGHTED is graph weighted?
 * @param a output graph (output)
 * @param stream input stream
 */
template <bool WEIGHTED=false, class G>
inline void readGraphMtxFormatOmpW(G& a, istream& stream) {
  using K = typename G::key_type;
  using E = typename G::edge_value_type;
  auto fb = [&](auto u, auto v, auto w) { addEdgeOmpU(a, K(u), K(v), E(w)); };
  bool symmetric = false;
  size_t rows = 0, cols = 0, size = 0;
  a.clear();
  readMtxFormatHeader(symmetric, rows, cols, size, stream);
  addVerticesU(a, K(1), K(max(rows, cols) + 1));
  readEdgelistFormatDoOmp<WEIGHTED>(stream, symmetric, fb);
  updateOmpU(a);
}
#endif
#pragma endregion




#pragma region WRITE GRAPH (EDGELIST, COO, MTX FORMATS)
/**
 * Write a graph in Edgelist format.
 * @tparam WEIGHTED is graph weighted?
 * @param stream output stream
 * @param x input graph
 * @param symmetric is graph symmetric?
 * @param sep separator
 */
template <bool WEIGHTED=false, class G>
inline void writeGraphEdgelistFormat(ostream& stream, const G& x, bool symmetric=false, char sep=' ') {
  x.forEachVertexKey([&](auto u) {
    x.forEachEdge(u, [&](auto v, auto w) {
      if (symmetric && u>v) return;
      stream << u << sep << v;
      if (WEIGHTED) stream << sep << w;
      stream << '\n';
    });
  });
}


#ifdef OPENMP
/**
 * Write a graph in Edgelist format.
 * @tparam WEIGHTED is graph weighted?
 * @param stream output stream
 * @param x input graph
 * @param symmetric is graph symmetric?
 * @param sep separator
 */
template <bool WEIGHTED=false, class G>
inline void writeGraphEdgelistFormatOmp(ostream& stream, const G& x, bool symmetric=false, char sep=' ') {
  const size_t CHUNK = 131072;
  // Allocate a text-block for each thread.
  int T = omp_get_max_threads();
  vector<string*> texts(T);
  for (int t=0; t<T; ++t)
    texts[t] = new string();
  // Write edges to text-blocks using multiple threads.
  size_t S = x.span();
  size_t I = ceilDiv(S, CHUNK);
  for (size_t i=0; i<I; ++i) {
    for (int t=0; t<T; ++t)
      texts[t]->clear();
    size_t U = min((i+1)*CHUNK, S);
    #pragma omp parallel for schedule(dynamic, 1024)
    for (size_t u=i*CHUNK; u<U; ++u) {
      int t = omp_get_thread_num();
      string ustr, vstr, wstr;
      x.forEachEdge(u, [&](auto v, auto w) {
        if (symmetric && u>v) return;
        ustr = to_string(u);
        vstr = to_string(v);
        wstr = WEIGHTED? to_string(w) : "";
        texts[t]->append(ustr);
        texts[t]->push_back(sep);
        texts[t]->append(vstr);
        if (WEIGHTED) {
          texts[t]->push_back(sep);
          texts[t]->append(wstr);
        }
        texts[t]->push_back('\n');
      });
    }
    // Write text-blocks to output stream.
    for (int t=0; t<T; ++t)
      stream << (*texts[t]);
  }
  // Free text-blocks.
  for (int t=0; t<T; ++t)
    if (texts[t]) delete texts[t];
}
#endif


/**
 * Write a graph in COO format.
 * @tparam WEIGHTED is graph weighted?
 * @param stream output stream
 * @param x input graph
 * @param symmetric is graph symmetric?
 * @param sep separator
 */
template <bool WEIGHTED=false, class G>
inline void writeGraphCooFormat(ostream& stream, const G& x, bool symmetric=false, char sep=' ') {
  size_t rows = x.order(), cols = x.order(), size = x.size();
  stream << rows << ' ' << cols << ' ' << size << '\n';
  writeGraphEdgelistFormat<WEIGHTED>(stream, x, symmetric, sep);
}


#ifdef OPENMP
/**
 * Write a graph in COO format.
 * @tparam WEIGHTED is graph weighted?
 * @param stream output stream
 * @param x input graph
 * @param symmetric is graph symmetric?
 * @param sep separator
 */
template <bool WEIGHTED=false, class G>
inline void writeGraphCooFormatOmp(ostream& stream, const G& x, bool symmetric=false, char sep=' ') {
  size_t rows = x.order(), cols = x.order(), size = x.size();
  stream << rows << ' ' << cols << ' ' << size << '\n';
  writeGraphEdgelistFormatOmp<WEIGHTED>(stream, x, symmetric, sep);
}
#endif


/**
 * Write a graph in MTX format.
 * @tparam WEIGHTED is graph weighted?
 * @param stream output stream
 * @param x input graph
 * @param symmetric is graph symmetric?
 * @param sep separator
 */
template <bool WEIGHTED=false, class G>
inline void writeGraphMtxFormat(ostream& stream, const G& x, bool symmetric=false, char sep=' ') {
  size_t rows = x.order(), cols = x.order(), size = x.size();
  stream << "%%MatrixMarket matrix coordinate";
  stream << (WEIGHTED?  " real"      : " pattern");
  stream << (symmetric? " symmetric" : " general") << '\n';
  stream << rows << ' ' << cols << ' ' << size << '\n';
  writeGraphEdgelistFormat<WEIGHTED>(stream, x, symmetric, sep);
}


#ifdef OPENMP
/**
 * Write a graph in MTX format.
 * @tparam WEIGHTED is graph weighted?
 * @param stream output stream
 * @param x input graph
 * @param symmetric is graph symmetric?
 * @param sep separator
 */
template <bool WEIGHTED=false, class G>
inline void writeGraphMtxFormatOmp(ostream& stream, const G& x, bool symmetric=false, char sep=' ') {
  size_t rows = x.order(), cols = x.order(), size = x.size();
  stream << "%%MatrixMarket matrix coordinate";
  stream << (WEIGHTED?  " real"      : " pattern");
  stream << (symmetric? " symmetric" : " general") << '\n';
  stream << rows << ' ' << cols << ' ' << size << '\n';
  writeGraphEdgelistFormatOmp<WEIGHTED>(stream, x, symmetric, sep);
}
#endif
#pragma endregion




#pragma region READ VECTOR
/**
 * Read a vector from a stream.
 * @param a output vector
 * @param stream input stream
 */
template <bool KEYED=false, class T>
inline void readVectorW(vector<T>& a, istream& stream) {
  string line;
  a.clear();
  while (getline(stream, line)) {
    replace(line.begin(), line.end(), ',', ' ');
    istringstream lstream(line);
    size_t i = 0; T x = T();
    if (KEYED) {
      lstream >> i >> x;
      a.resize(max(a.size(), i+1));
      a[i] = x;
    }
    else {
      lstream >> x;
      a.push_back(x);
    }
  }
}
#pragma endregion
#pragma endregion
