#!/bin/bash
# This script runs inside Docker

set -e -x


# Cleanup
cd /pyuv
rm -rf wheeltmp

# Build
for PYBIN in /opt/python/*/bin; do
    "$PYBIN/python" setup.py build_ext
    "$PYBIN/pip" wheel . -w wheeltmp
done
for whl in wheeltmp/*.whl; do
    auditwheel repair $whl -w wheelhouse
done

# Cleanup
rm -rf wheeltmp
cd ..

# Test (ignore if some fail, we just want to know that the module works)
for PYBIN in /opt/python/*/bin; do
    "$PYBIN/python" --version
    "$PYBIN/pip" install pyuv --no-index -f /pyuv/wheelhouse
    "$PYBIN/python" -c "import pyuv; print('%s - %s' % (pyuv.__version__, pyuv.LIBUV_VERSION));"
done

exit 0
