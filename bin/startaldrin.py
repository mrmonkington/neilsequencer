# this is a startup file for python on win32

import os, sys
os.environ['PATH'] = r'PATH=C:\aldrin\lib;' + os.environ['PATH']
os.environ['ALDRIN_PLUGIN_PATH'] = r'C:\aldrin\lib\zzub'

if '--debug' in sys.argv:
	sys.argv.remove('--debug')
	os.system('gdb --eval-command=run --args python "%s"$*' % sys.argv[0])
	raise SystemExit, 0
parent_path = os.path.abspath(os.path.join(os.path.dirname(__file__),'..','share'))
if not parent_path in sys.path:
	sys.path += [parent_path]
import aldrin
aldrin.run(sys.argv)
