#!/usr/bin/env python
# encoding: latin-1
# zzub IDL parser test

import os

idl_data = """

include "stdio.h"

namespace zzub:

	class Plugin:
		def get_name(): string
		def set_name(string name)
		def get_enabled(): bool
		def get_position(): float x, float y

	class Player:
		"here is a docstring for get_name(). if this string extends"
		"over multiple rows, it's going to be merged into"
		"one huge section."
		def get_name(): string
		
		"here is another one."
		def set_name(string name)
		
		def get_plugin(string name): Plugin first, Plugin second
"""

idl_file = "test.zidl"

file(idl_file,"w").write(idl_data)

ret = os.system('./zidl --c-header test_zidl.h ' + idl_file)
assert ret == 0

data = file("test_zidl.h","r").read()
print data

