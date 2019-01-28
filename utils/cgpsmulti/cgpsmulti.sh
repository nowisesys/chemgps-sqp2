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
verbose="0"
logdir=""
outfmt="out.%.07d"

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
  echo "  -o,--logdir=path:   Save per process log (i.e. path/`printf $outfmt 465`)"
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
    -o|--logdir)
      shift
      logdir="$1"
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    -v|--verbose)
      verbose="1"
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

if ! [ -z "$logdir" ]; then
  if ! [ -d "$logdir" ]; then
    echo "$prog: log directory $logdir don't exist"
    exit 1
  fi
fi

if [ "$verbose" == "1" ]; then
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
  echo "  logdir = $logdir"
  echo
  echo "Result:"
  echo "----------------------"
fi

function run_command()
{
  local backgr="$1"
  local logdir="$2"
  local args="$3"
  local file="`printf $outfmt $4`"
  
  if [ "$backgr" == 1 ]; then
    if [ -z "$logdir" ]; then
      $cgpscltdir/cgpsclt $args &
    else
      $cgpscltdir/cgpsclt $args >& $logdir/$file &
    fi
  else
    if [ -z "$logdir" ]; then
      $cgpscltdir/cgpsclt $args
    else
      $cgpscltdir/cgpsclt $args >& $logdir/$file
    fi
  fi    
}

num=0
while [ "$num" -lt "$max" ]; do
  if [ "$unix" == "1" ]; then
    run_command "$background" "$logdir" "$args -s $sock -i $input -r $result" "$num"
  else
    run_command "$background" "$logdir" "$args -H $host -i $input -r $result" "$num"
  fi
  let num=$num+1
done
