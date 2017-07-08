#!/bin/bash
# This script runs inside Docker

set -e -x


# Cleanup
cd /pyuv
rm -rf wheeltmp

VERSIONS="cp27-cp27mu cp33-cp33m cp34-cp34m cp35-cp35m cp36-cp36m"

# Build
for version in $VERSIONS; do
    /opt/python/${version}/bin/python setup.py build_ext
    /opt/python/${version}/bin/pip wheel . -w wheeltmp
done
for whl in wheeltmp/*.whl; do
    auditwheel repair $whl -w wheelhouse
done

# Cleanup
rm -rf wheeltmp
cd ..

# Test (ignore if some fail, we just want to know that the module works)
for version in $VERSIONS; do
    /opt/python/${version}/bin/python --version
    /opt/python/${version}/bin/pip install pyuv --no-index -f /pyuv/wheelhouse
    /opt/python/${version}/bin/python -c "import pyuv; print('%s - %s' % (pyuv.__version__, pyuv.LIBUV_VERSION));"
done

exit 0
