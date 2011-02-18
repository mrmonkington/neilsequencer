#!/usr/bin/env python

import os, sys, imp

if sys.platform == 'win32':
	os.environ['PATH'] = r'PATH=C:\aldrin\lib;' + os.environ['PATH']
	os.environ['ALDRIN_PLUGIN_PATH'] = r'C:\aldrin\lib\zzub'

if '--debug' in sys.argv:
	sys.argv.remove('--debug')
	os.system('gdb --eval-command=run -q --args python "%s"$*' % sys.argv[0])
	raise SystemExit, 0
if '--pydebug' in sys.argv:
	sys.argv.remove('--pydebug')
	os.system('winpdb "%s"$*' % sys.argv[0])
	raise SystemExit, 0

CWD = os.path.abspath(os.path.join(os.path.dirname(__file__)))
if os.path.isfile(os.path.join(CWD, 'this_is_a_repository')):
	module_path = os.path.normpath(os.path.join(CWD, '../src'))
	print "adding " + module_path + " to sys.path"
	sys.path = [module_path] + sys.path

import aldrin.main
aldrin.main.run(sys.argv)
