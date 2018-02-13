#!/usr/bin/env python3

import sys
import re

with open(sys.argv[1]) as f:
    print("\"%s\"" % re.sub('[\s\t\n]+', ' ', str(f.read())).strip())
