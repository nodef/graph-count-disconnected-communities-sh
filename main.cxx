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
  /** Input file format ("mtx", "coo", "edgelist", "csv", "tsv"). */
  string inputFormat = "mtx";
  /** Community membership file name. */
  string membershipFile = "";
  /** Whether the community membership file is keyed. */
  bool membershipKeyed = false;
  /** Community membership start index. */
  int membershipStart = 0;
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
  for (int i=1; i<argc; ++i) {
    string k = argv[i];
    if (k=="") continue;
    else if (k=="-h" || k=="--help") o.help = true;
    else if (k=="-i" || k=="--input")  o.inputFile  = argv[++i];
    else if (k=="-m" || k=="--membership") o.membershipFile  = argv[++i];
    else if (k=="-f" || k=="--input-format")  o.inputFormat  = argv[++i];
    else if (k=="-k" || k=="--membership-keyed") o.membershipKeyed = true;
    else if (k=="-r" || k=="--membership-start") o.membershipStart = atoi(argv[++i]);
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
  fprintf(stderr, "  -h, --help                   Show this help message.\n");
  fprintf(stderr, "  -i, --input <file>           Input file name.\n");
  fprintf(stderr, "  -f, --input-format <format>  Input file format.\n");
  fprintf(stderr, "  -m, --membership <file>      Community membership file name.\n");
  fprintf(stderr, "  -k, --membership-keyed       Community membership file is keyed.\n");
  fprintf(stderr, "  -r, --membership-start       Community membership start index.\n");
  fprintf(stderr, "  -w, --weighted               Input graph is weighted.\n");
  fprintf(stderr, "  -s, --symmetric              Input graph is symmetric.\n");
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
  if (o.inputFile.empty())      { fprintf(stderr, "Input file is not specified.\n");  return false; }
  if (o.membershipFile.empty()) { fprintf(stderr, "Community membership file is not specified.\n"); return false; }
  if (!validateFormat(o.inputFormat))  { fprintf(stderr, "Unknown input format: %s\n",  o.inputFormat.c_str());  return false; }
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
  // Read community membership.
  vector<K> membership(x.span());
  ifstream membershipStream(o.membershipFile.c_str());
  printf("Reading community membership %s ...\n", o.membershipFile.c_str());
  if (o.membershipKeyed) readVectorW<true> (membership, membershipStream, o.membershipStart);
  else                   readVectorW<false>(membership, membershipStream, o.membershipStart);
  // Count the number of disconnected communities.
  auto fc = [&](auto u) { return membership[u]; };
  size_t ncom = communities(x, membership).size();
  size_t ndis = countValue(communitiesDisconnectedOmp(x, membership), char(1));
  printf("Number of communities: %zu\n", ncom);
  printf("Number of disconnected communities: %zu\n", ndis);
  printf("\n");
  return 0;
}
#pragma endregion
