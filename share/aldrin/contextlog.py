# encoding: latin-1

"""
contextlog adds a hook to stdout which enhances each line with a source
path and a line number so the source of the log output can be easily found
later and the output can be grepped easier.

in order to use contextlog, just import it and call L{init}.
"""

import traceback
import os
from path import path

ESCAPE_BEGIN = "\033[0;34m"
ESCAPE_END = "\033[0;0m"

ERROR_ESCAPE_BEGIN = "\033[0;31m"

class StdErrAnnotator:
	def __init__(self):
		import sys
		self.stderr = sys.stderr
		sys.stderr = self
		self.annotate_next = True
		print >> sys.stderr, "annotating stderr"
		
	def write(self, text):
		error = False
		if text.strip().startswith('File '):
			error = True
		elif text.strip().startswith('Traceback'):
			error = True
		elif 'Error' in text:
			error = True
		for c in text:
			if self.annotate_next:
				self.annotate_next = False
				if error:
					self.stderr.write(ERROR_ESCAPE_BEGIN)
				#self.annotate(traceback.extract_stack())
			if c == '\n':
				if not self.annotate_next:
					self.annotate_next = True
					self.stderr.write(ESCAPE_END)
			self.stderr.write(c)
		
	def flush(self):
		self.stderr.flush()
		
	def close(self):
		self.stderr.close()

class StdOutAnnotator:
	def __init__(self):
		import sys
		self.stdout = sys.stdout
		sys.stdout = self
		self.annotate_next = True
		print "annotating stdout"
		
	def annotate(self, stack):
		filename = '?'
		line = 0
		if len(stack) >= 2:
			entry = stack[-2]
			if len(entry) >= 2:
				filename = path(entry[0])
				line = entry[1]
		if not str(filename.relpath()).startswith('..'):
			filename = filename.relpath()
		self.stdout.write("%s%s:%s:%s" % (ESCAPE_BEGIN,filename,line,ESCAPE_END))
	
	def write(self, text):
		for c in text:
			if self.annotate_next:
				self.annotate_next = False
				self.annotate(traceback.extract_stack())
			if c == '\n':
				self.annotate_next = True
			self.stdout.write(c)
		
	def flush(self):
		self.stdout.flush()
		
	def close(self):
		self.stdout.close()

_stderr_annotator = None
_stdout_annotator = None

def init():
	"""
	enables log annotation.
	"""
	global _stderr_annotator
	global _stdout_annotator
	
	if not _stderr_annotator:
		_stderr_annotator = StdErrAnnotator()	
	if not _stdout_annotator:
		_stdout_annotator = StdOutAnnotator()	

def clean_next_line():
	if not _stdout_annotator:
		return
	_stdout_annotator.annotate_next = False

__all__ = [
	'init',
	'print_clean',
]

if __name__ == '__main__':
	init()
	print "hello."
	clean_next_line()
	print "hello again."
	print "and hello once more."
	raise Exception
