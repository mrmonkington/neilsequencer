#!/usr/bin/env python
# encoding: latin-1
# zzub IDL parser test

import os

idl_data = """
namespace zzub:

	class plugin:
		def get_name(): (str name)
		def set_name(str name)

	class player:
		def get_name(): (str name)
		def set_name(str name)
"""

idl_file = "test.zidl"

file(idl_file,"w").write(idl_data)

ret = os.system('./zidl --c-header test_zidl.h ' + idl_file)
print ret

data = file("test_zidl.h","r").read()
print data

