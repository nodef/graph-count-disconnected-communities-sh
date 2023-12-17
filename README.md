Convert a directed graph to an undirected graph.

```bash
# Convert the web-Stanford directed graph to an undirected graph.
$ graph-make-undirected -i web-Stanford.mtx -o web-Stanford-undirected.mtx

# Convert the web-Stanford directed graph to an undirected graph in CSV format.
$ graph-make-undirected -i web-Stanford.mtx -o web-Stanford-undirected.tsv -f mtx -g csv

# Convert the weighted vt2010 directed graph to an undirected graph.
$ graph-make-undirected -i vt2010.mtx -o vt2010-undirected.mtx -w
```

<br>


### Usage

```bash
$ graph-make-undirected [options]

Options:
  -h, --help                    Show this help message.
  -i, --input <file>            Input file name.
  -o, --output <file>           Output file name.
  -f, --input-format <format>   Input file format.
  -g, --output-format <format>  Output file format.
  -w, --weighted                Input graph is weighted.
  -s, --symmetric               Input graph is symmetric.

Supported formats:
  mtx       Matrix Market format (default).
  coo       Coordinate format.
  edgelist  Edgelist format.
  csv       Comma-separated values format.
  tsv       Tab-separated values format.
```

<br>


### Installation

```bash
$ git clone https://github.com/ionicf/graph-make-undirected
$ cd graph-make-undirected
$ ./install.sh --prefix /usr/local
```

<br>
<br>


[![](https://img.youtube.com/vi/yqO7wVBTuLw/maxresdefault.jpg)](https://www.youtube.com/watch?v=yqO7wVBTuLw)<br>
[![ORG](https://img.shields.io/badge/org-puzzlef-green?logo=Org)](https://puzzlef.github.io)
