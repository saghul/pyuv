#!/bin/bash
# This script runs inside Docker

set -e


# Dependencies
yum install -y libtool
ARCH=$(uname -m)
if [[ $ARCH == "x86_64" ]]; then
    wget ftp://ftp.pbone.net/mirror/ftp5.gwdg.de/pub/opensuse/repositories/home:/tpokorra:/openpetra/CentOS_CentOS-5/x86_64/autoconf-2.69-3.2.x86_64.rpm
    yum install -y --nogpgcheck autoconf*x86_64.rpm
else
    wget ftp://ftp.pbone.net/mirror/ftp5.gwdg.de/pub/opensuse/repositories/home:/tpokorra:/openpetra/CentOS_CentOS-5/i386/autoconf-2.69-3.2.i386.rpm
    yum install -y --nogpgcheck autoconf*i386.rpm
fi
/opt/python/cp35-cp35m/bin/pip install -U auditwheel

# Cleanup
cd /pyuv
rm -rf wheeltmp

# Build
for version in cp27-cp27m cp27-cp27mu cp33-cp33m cp34-cp34m cp35-cp35m; do
    /opt/python/${version}/bin/python setup.py build_ext --libuv-clean-compile
    /opt/python/${version}/bin/pip wheel . -w wheeltmp
done
for whl in wheeltmp/*.whl; do
    auditwheel repair $whl -w wheelhouse
done

# Cleanup
rm -rf wheeltmp
cd ..

# Test (ignore if some fail, we just want to know that the module works)
for version in cp27-cp27m cp27-cp27mu cp33-cp33m cp34-cp34m cp35-cp35m; do
    /opt/python/${version}/bin/python --version
    /opt/python/${version}/bin/pip install pyuv --no-index -f /pyuv/wheelhouse
    /opt/python/${version}/bin/python -c "import pyuv; print('%s - %s' % (pyuv.__version__, pyuv.LIBUV_VERSION));"
done

exit 0
