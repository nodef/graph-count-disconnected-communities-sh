#!/usr/bin/env bash
# Default option values.
: "${PREFIX:=/usr/local}"
HELP="0"


# Parse options.
while [[ $# -gt 0 ]]; do
  case "$1" in
    -h|--help)
      HELP="1"
      shift
      ;;
    -p|--prefix)
      PREFIX="$2"
      shift 2
      ;;
    *)
      echo "Unknown option: $1"
      exit 1
      ;;
  esac
done


# Print help message.
if [[ "$HELP" == "1" ]]; then
  echo "Usage: ./install.sh [options]"
  echo "Options:"
  echo "  -h, --help      Print this message and exit."
  echo "  -p, --prefix    Install prefix (default: /usr/local)."
  exit 0
fi


# Install program.
mkdir -p "$PREFIX/bin"
echo "Compiling graph-count-disconnected-communities ..."
DOWNLOAD=0 RUN=0 ./main.sh
echo "Done."
printf "\n"
echo "Installing graph-count-disconnected-communities ..."
mv a.out "$PREFIX/bin/graph-count-disconnected-communities"
echo "Done."
printf "\n"
