#~ Canvas
#~ Copyright (C) 2002 Michael Gilfix <mgilfix@eecs.tufts.edu>
#~ Copyright (C) 2006 Leonard Ritter

#~ This file was part of PyColourChooser.

#~ This program is distributed in the hope that it will be useful,
#~ but WITHOUT ANY WARRANTY; without even the implied warranty of
#~ MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

"""
Provides a double-buffered canvas class.
"""

import  wx

class BitmapBuffer(wx.MemoryDC):
	"""A screen buffer class.

	This class implements a screen output buffer. Data is meant to
	be drawn in the buffer class and then blitted directly to the
	output device, or on-screen window.
	"""
	def __init__(self, width, height, colour = None):
		"""Initialize the empty buffer object."""
		wx.MemoryDC.__init__(self)

		self.width = width
		self.height = height
		self.colour = colour

		self.bitmap = wx.EmptyBitmap(self.width, self.height)
		self.SelectObject(self.bitmap)

		# Initialize the buffer to the background colour
		if self.colour:
			self.SetBackground(wx.Brush(self.colour, wx.SOLID))
			self.Clear()

		# Make each logical unit of the buffer equal to 1 pixel
		self.SetMapMode(wx.MM_TEXT)

	def GetBitmap(self):
		"""Returns the internal bitmap for direct drawing."""
		return self.bitmap

class CanvasBase:
	"""A canvas class for arbitrary drawing.

	The Canvas class implements a window that allows for drawing
	arbitrary graphics. It implements a double buffer scheme and
	blits the off-screen buffer to the window during paint calls
	by the windowing system for speed.

	Some other methods for determining the canvas colour and size
	are also provided.
	"""

	doublebuffered = True
	
	def __init__(self, *args, **kargs):
		"""Creates a canvas instance and initializes the off-screen
		buffer. Also sets the handler for rendering the canvas
		automatically via size and paint calls from the windowing
		system."""
		self.buffer = None
		self.redraw_args = None		
		
	def _finalize_canvas_init(self):
		# Perform an intial sizing
		self.ReDraw()

		# Register event handlers
		self.Bind(wx.EVT_SIZE, self.onSize)
		self.Bind(wx.EVT_PAINT, self.onPaint)
		wx.EVT_ERASE_BACKGROUND(self, self.onEraseBackground)
		
	def onEraseBackground(self, event):
		pass

	def MakeNewBuffer(self):
		size = self.GetSize()
		if self.buffer and (self.buffer.width == size[0]) and (self.buffer.height == size[1]):
			return
		self.buffer = BitmapBuffer(size[0], size[1],
								   self.GetBackgroundColour())

	def onSize(self, event):
		"""Perform actual redraw to off-screen buffer only when the
		size of the canvas has changed. This saves a lot of computation
		since the same image can be re-used, provided the canvas size
		hasn't changed."""
		self.ReDraw()

	def ReDraw(self,*args,**kargs):
		"""Explicitly tells the canvas to redraw it's contents."""
		if self.doublebuffered:
			self.MakeNewBuffer()
			self.DrawBuffer(*args,**kargs)
		self.Refresh()
		
	def onPostPaint(self, dc):
		pass

	def onPaint(self, event):
		"""Renders the off-screen buffer on-screen."""
		dc = wx.PaintDC(self)
		dc.BeginDrawing()
		if self.doublebuffered:
			self.Blit(dc)
		else:
			self.buffer = dc
			self.DrawBuffer()
		self.onPostPaint(dc)
		dc.EndDrawing()

	def Blit(self, dc):
		"""Performs the blit of the buffer contents on-screen."""
		width, height = self.buffer.GetSize()
		dc.Blit(0, 0, width, height, self.buffer, 0, 0)

	def GetBoundingRect(self):
		"""Returns a tuple that contains the co-ordinates of the
		top-left and bottom-right corners of the canvas."""
		x, y = self.GetPosition()
		w, h = self.GetSize()
		return(x, y + h, x + w, y)

	def DrawBuffer(self,*args,**kargs):
		"""Actual drawing function for drawing into the off-screen
		buffer. To be overrideen in the implementing class. Do nothing
		by default."""
		pass

class Canvas(wx.Window, CanvasBase):
	def __init__(self, *args, **kargs):
		CanvasBase.__init__(self, *args, **kargs)
		wx.Window.__init__(self, *args, **kargs)
		self._finalize_canvas_init()

class ScrolledCanvas(wx.ScrolledWindow, CanvasBase):
	def __init__(self, *args, **kargs):
		CanvasBase.__init__(self, *args, **kargs)
		wx.ScrolledWindow.__init__(self, *args, **kargs)
		self._finalize_canvas_init()


__all__ = [
	'BitmapBuffer',
	'Canvas',
	'ScrolledCanvas',
]
