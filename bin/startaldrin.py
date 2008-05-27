# this is a startup file for python on win32

import os, sys, imp
if sys.platform == 'win32':
	os.environ['PATH'] = r'PATH=C:\aldrin\lib;' + os.environ['PATH']
	os.environ['ALDRIN_PLUGIN_PATH'] = r'C:\aldrin\lib\zzub'

if '--debug' in sys.argv:
	sys.argv.remove('--debug')
	os.system('gdb --eval-command=run --args python "%s"$*' % sys.argv[0])
	raise SystemExit, 0
sourcepath = os.path.abspath(os.path.join(os.path.dirname(__file__),'..','share','aldrin'))
os.chdir(sourcepath)
main = imp.load_source('aldrin', os.path.join(sourcepath, 'main.py'))
main.run(sys.argv)
