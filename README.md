Count the number of disconnected communities in a graph, for the given community membership of each vertex.

```bash
# Count disconnected communities in the web-Stanford graph, from given the membership (keyed, starting from 0).
$ graph-count-disconnected-communities -k -r 0 -i web-Stanford.mtx -m web-Stanford-membership.txt

# Count disconnected communities in the web-Stanford graph, from given the membership file (in TSV format).
$ graph-count-disconnected-communities -k -r 0 -i web-Stanford.mtx -m web-Stanford-membership.tsv -f mtx

# Count disconnected communities in the weighted vt2010 graph, from given the membership file (in CSV format).
$ graph-count-disconnected-communities -k -r 0 -i vt2010.mtx -m vt2010-membership.csv -w
```

<br>


### Usage

```bash
$ graph-count-disconnected-communities [options]

Options:
  -h, --help                    Show this help message.
  -i, --input <file>            Input file name.
  -f, --input-format <format>   Input file format.
  -m, --membership <file>       Community membership file name.
  -k, --membership-keyed        Community membership file is keyed.
  -r, --membership-start        Community membership start index.
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
$ git clone https://github.com/ionicf/graph-count-disconnected-communities
$ cd graph-count-disconnected-communities
$ ./install.sh --prefix /usr/local
```

<br>
<br>


[![](https://img.youtube.com/vi/yqO7wVBTuLw/maxresdefault.jpg)](https://www.youtube.com/watch?v=yqO7wVBTuLw)<br>
[![ORG](https://img.shields.io/badge/org-puzzlef-green?logo=Org)](https://puzzlef.github.io)
