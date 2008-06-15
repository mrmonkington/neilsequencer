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
		self.stdout.write("%s:%s:" % (filename,line))
	
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

_annotator = None

def init():
	"""
	enables log annotation.
	"""
	global _annotator
	if not _annotator:
		_annotator = StdOutAnnotator()	
	return _annotator

def clean_next_line():
	if not _annotator:
		return
	_annotator.annotate_next = False

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
