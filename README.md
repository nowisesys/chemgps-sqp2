# Multi Variant Analysis using SIMCA-QP

The chemgps-sqp2 package contains a daemon, client and standalone program
for making predictions using Umetrics SIMAP-QP. The programs is based on
the libchemgps library (should be installed before trying to build this
package).

The cgpsclt/cgpsd works together and provides a client/server solution.
Communication is done using local (UNIX socket) and networks (TCP socket)
connections. 

The cgpsstd (standalone) binary is considered to be a test tool. Use the
client/server solution for production systems.

### BUILD:

Use CC to link against 32-bit library on 64-bit Linux:

```bash
CC="gcc32" CFLAGS="-Wall -O2 -m32" ./configure
make
make install
```

### ABOUT:

Used in backend code by https://chemgps.bmc.uu.se for PCA. You need to
obtain a server side license for SIMCA-QP to use these binaries.
