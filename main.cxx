#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <istream>
#include <ostream>
#include <fstream>
#include <omp.h>
#include "inc/main.hxx"

using namespace std;




#pragma region CONFIGURATION
#ifndef KEY_TYPE
/** Type of vertex ids. */
#define KEY_TYPE uint32_t
#endif
#ifndef EDGE_VALUE_TYPE
/** Type of edge weights. */
#define EDGE_VALUE_TYPE float
#endif
#ifndef MAX_THREADS
/** Maximum number of threads to use. */
#define MAX_THREADS 1
#endif
#pragma endregion




#pragma region MAIN
struct Options {
  /** Input file name. */
  string inputFile = "";
  /** Output file name. */
  string outputFile = "";
  /** Input file format ("mtx", "coo", "edgelist", "csv", "tsv"). */
  string inputFormat = "mtx";
  /** Output file format ("mtx", "coo", "edgelist", "csv", "tsv"). */
  string outputFormat = "mtx";
  /** Whether the input graph is weighted. */
  bool weighted = false;
  /** Whether the input graph is symmetric. */
  bool symmetric = false;
  /** Whether to print help. */
  bool help = false;
};


/**
 * Parse command line arguments.
 * @param argc argument count
 * @param argv argument values
 * @returns options
 */
inline Options parseOptions(int argc, char **argv) {
  Options o;
  o.inputFormat  = "mtx";
  o.outputFormat = "mtx";
  for (int i=1; i<argc; ++i) {
    string k = argv[i];
    if (k=="") continue;
    else if (k=="-h" || k=="--help") o.help = true;
    else if (k=="-i" || k=="--input")  o.inputFile  = argv[++i];
    else if (k=="-o" || k=="--output") o.outputFile = argv[++i];
    else if (k=="-f" || k=="--input-format")  o.inputFormat  = argv[++i];
    else if (k=="-g" || k=="--output-format") o.outputFormat = argv[++i];
    else if (k=="-w" || k=="--weighted")  o.weighted  = true;
    else if (k=="-s" || k=="--symmetric") o.symmetric = true;
    else {
      fprintf(stderr, "Unknown option: %s\n", k.c_str());
      o.help = true;
    }
  }
  return o;
}


/**
 * Show help message.
 * @param name program name
 */
inline void showHelp(const char *name) {
  fprintf(stderr, "graph-make-undirected:\n");
  fprintf(stderr, "Convert a directed graph to an undirected graph.\n\n");
  fprintf(stderr, "Usage: %s [options]\n", name);
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  -h, --help                    Show this help message.\n");
  fprintf(stderr, "  -i, --input <file>            Input file name.\n");
  fprintf(stderr, "  -o, --output <file>           Output file name.\n");
  fprintf(stderr, "  -f, --input-format <format>   Input file format.\n");
  fprintf(stderr, "  -g, --output-format <format>  Output file format.\n");
  fprintf(stderr, "  -w, --weighted                Input graph is weighted.\n");
  fprintf(stderr, "  -s, --symmetric               Input graph is symmetric.\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Supported formats:\n");
  fprintf(stderr, "  mtx       Matrix Market format (default).\n");
  fprintf(stderr, "  coo       Coordinate format.\n");
  fprintf(stderr, "  edgelist  Edgelist format.\n");
  fprintf(stderr, "  csv       Comma-separated values format.\n");
  fprintf(stderr, "  tsv       Tab-separated values format.\n");
  fprintf(stderr, "\n");
}


/**
 * Validate input/output file format.
 * @param format format
 * @returns true if valid
 */
inline bool validateFormat(const string &format) {
  return format=="mtx" || format=="coo" || format=="csv" || format=="tsv";
}


/**
 * Validate options.
 * @param o options
 * @returns true if valid
 */
inline bool validateOptions(const Options &o) {
  if (o.inputFile.empty())  { fprintf(stderr, "Input file is not specified.\n");  return false; }
  if (o.outputFile.empty()) { fprintf(stderr, "Output file is not specified.\n"); return false; }
  if (!validateFormat(o.inputFormat))  { fprintf(stderr, "Unknown input format: %s\n",  o.inputFormat.c_str());  return false; }
  if (!validateFormat(o.outputFormat)) { fprintf(stderr, "Unknown output format: %s\n", o.outputFormat.c_str()); return false; }
  return true;
}


/**
 * Read the specified input graph.
 * @param a read graph (output)
 * @param file input file name
 * @param format input file format
 * @param symmetric is graph symmetric?
 */
template <bool WEIGHTED=false, class G>
inline void readGraphW(G& a, const string& file, const string& format, bool symmetric=false) {
  ifstream stream(file.c_str());
  if (format=="mtx") readGraphMtxFormatOmpW<WEIGHTED>(a, stream);
  else if (format=="coo") readGraphCooFormatOmpW<WEIGHTED>(a, stream, symmetric);
  else if (format=="edgelist" || format=="csv" || format=="tsv") readGraphEdgelistFormatOmpW<WEIGHTED>(a, stream, symmetric);
  else throw std::runtime_error("Unknown input format: " + format);
}


/**
 * Write the specified output graph.
 * @param x graph to write (input)
 * @param file output file name
 * @param format output file format
 * @param symmetric is graph symmetric?
 */
template <bool WEIGHTED=false, class G>
inline void writeGraph(const G& x, const string& file, const string& format, bool symmetric=false) {
  ofstream stream(file.c_str());
  if (format=="mtx") writeGraphMtxFormatOmp<WEIGHTED>(stream, x, symmetric);
  else if (format=="coo") writeGraphCooFormatOmp<WEIGHTED>(stream, x, symmetric);
  else if (format=="edgelist") writeGraphEdgelistFormatOmp<WEIGHTED>(stream, x, symmetric);
  else if (format=="csv") writeGraphEdgelistFormatOmp<WEIGHTED>(stream, x, symmetric, ',');
  else if (format=="tsv") writeGraphEdgelistFormatOmp<WEIGHTED>(stream, x, symmetric, '\t');
  else throw std::runtime_error("Unknown output format: " + format);
}


/**
 * Main function.
 * @param argc argument count
 * @param argv argument values
 * @returns zero on success, non-zero on failure
 */
int main(int argc, char **argv) {
  using K = KEY_TYPE;
  using E = EDGE_VALUE_TYPE;
  Options o = parseOptions(argc, argv);
  if (o.help) { showHelp(argv[0]); return 1; }
  if (!validateOptions(o)) return 1;
  if (MAX_THREADS) omp_set_num_threads(MAX_THREADS);
  // Read graph.
  DiGraph<K, None, E> x;
  printf("Reading graph %s ...\n", o.inputFile.c_str());
  if (o.weighted) readGraphW<true> (x, o.inputFile, o.inputFormat, o.symmetric);
  else            readGraphW<false>(x, o.inputFile, o.inputFormat, o.symmetric);
  println(x);
  // Symmetrize graph.
  if (!o.symmetric) {
    x = symmetrizeOmp(x);
    print(x); printf(" (symmetrize)\n");
  }
  // Write undirected graph.
  printf("Writing undirected graph %s ...\n", o.outputFile.c_str());
  if (o.weighted) writeGraph<true> (x, o.outputFile, o.outputFormat, true);
  else            writeGraph<false>(x, o.outputFile, o.outputFormat, true);
  printf("Done.\n\n");
  return 0;
}
#pragma endregion
