#!/usr/bin/env python
# encoding: latin-1
# zzub IDL parser test

import os

ret = os.system('./zidl --c-header test_zidl.h libzzub.zidl')
assert ret == 0

data = file("test_zidl.h","r").read()
print data

