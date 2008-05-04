#!/bin/sh
#
# Issue multiple client (cgpsclt) connections against the daemon (cgpsd).
#
# Author: Anders Lövgren
# Date:   2008-04-29

#
# Defaults:
#
host="127.0.0.1"
port="9401"
max="10"
input="../../admin/data.txt"
prog="$(basename $0)"
result="tps"
sock="/var/run/cgpsd.sock"
unix="0"
background="0"
args=""

chemgpsdir="../../../libchemgps/src/.libs"
cgpscltdir="../../cgpsclt"

#
# Make sure to start in right directory:
#
cd $(dirname $0)

#
# Sanity check:
#
if ! [ -d "$chemgpsdir" ]; then
  echo "$prog: the libchemgps directory was not found (looking for $chemgpsdir)"
  exit 1
fi
if ! [ -d "$cgpscltdir" ]; then
  echo "$prog: the cgpsclt directory was not found (looking for $cgpscltdir)"
  exit 1
fi

#
# We need this:
#
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:$chemgpsdir"

function usage()
{
  echo "$prog - issue multiple client (cgpsclt) connections against the daemon (cgpsd)"
  echo 
  echo "Usage: $prog [options...]"
  echo "Options:"
  echo "  -m,--max <num>:     Number of request [$max]"
  echo "  -i,--input <file>:  Input data [$input]"
  echo "  -H,--host <host>:   Connect to host [$host]"
  echo "  -p,--port <port>:   Connect to port [$port]"
  echo "  -r,--result <list>: List of result (colon separated) [$result]"
  echo "  -s,--sock <path>:   Set UNIX socket path [$sock]"
  echo "  -l,--local:         Connect to UNIX socket [$unix]"
  echo "  -b,--background:    Run each client in background [$background]"
  echo 
  echo "All other options are passed direct to cgpsclt."
  echo 
  echo "This application is part of the ChemGPS project."
  echo "Send bug reports to anders.lovgren@bmc.uu.se"
}

if [ "$#" == "0" ]; then
  usage
  exit 1
fi

while [ "$1" != "" ]; do
  case "$1" in
    -m|--max)
      shift
      max="$1"
      ;;
    -i|--input)
      shift
      input="$1"
      ;;
    -H|--host)
      shift
      host="$1"
      ;;
    -p|--port)
      shift
      port="$1"
      ;;
    -r|--result)
      shift
      result="$1"
      ;;
    -s|--sock)
      shift
      sock="$1"
      ;;
    -l|--local)
      unix="1"
      ;;
    -b|--background)
      background="1"
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *) 
      args="$args $1"
      ;;
  esac
  shift
done

if [ "$max" -lt "1" ]; then
  echo "$prog: max is <= 0, see '--help' option"
  exit 1
fi

echo "Using options:"
echo "-----------------------"
if [ "$unix" == "1" ]; then
  echo "    sock = $sock"
else
  echo "    host = $host"
  echo "    port = $port"
fi
echo "     max = $max"
echo "   input = $input"
echo "  result = $result"
echo

echo "Result:"
echo "----------------------"
num=0
while [ "$num" -lt "$max" ]; do
  if [ "$unix" == "1" ]; then
    if [ "$background" == "1" ]; then    
      $cgpscltdir/cgpsclt $args -s $sock -i $input -r $result &
    else 
      $cgpscltdir/cgpsclt $args -s $sock -i $input -r $result
    fi 
  else
    if [ "$background" == "1" ]; then    
      $cgpscltdir/cgpsclt $args -H $host -i $input -r $result &
    else 
      $cgpscltdir/cgpsclt $args -H $host -i $input -r $result
    fi
  fi
  let num=$num+1
done
