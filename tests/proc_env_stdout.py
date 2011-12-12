#!/usr/bin/env python

import os
import sys

print os.environ.get("TEST", "FAIL")
sys.stdout.flush()

