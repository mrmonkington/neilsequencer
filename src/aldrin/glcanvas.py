
from wximport import wx
import wx.glcanvas

from OpenGL.GL import 	glColor3f, \
						glClearColor, \
						glClear, \
						GL_COLOR_BUFFER_BIT, \
						glOrtho, \
						glBegin, \
						glEnd, \
						glVertex2f, \
						GL_QUADS, \
						glMatrixMode, \
						GL_PROJECTION, \
						glLoadIdentity, \
						GL_LINES, \
						GL_POLYGON, \
						GL_LINE_LOOP

def make_glcolor(colour):
	return colour.Red()/255.0, colour.Green()/255.0, colour.Blue()/255.0

class GLClientDC:
	def __init__(self, glcanvas):
		self.canvas = glcanvas		
		
	def BeginDrawing(self):
		self.canvas.SetCurrent()
		w,h = self.canvas.GetClientSize()
		glMatrixMode(GL_PROJECTION)
		glLoadIdentity()
		glOrtho(0,w,h,0,-1,1)
		
	def EndDrawing(self):
		self.canvas.SwapBuffers()
		
	def SetFont(self, font):
		pass
		
	def SetBackground(self, brush):
		self.bgbrushcolor = make_glcolor(brush.GetColour())
		
	def SetBackgroundMode(self, mode):
		pass
		
	def SetBrush(self, brush):
		self.brushcolor = make_glcolor(brush.GetColour())
		
	def SetPen(self, pen):
		if pen == wx.TRANSPARENT_PEN:
			self.pencolor = None
		else:
			self.pencolor = make_glcolor(pen.GetColour())
		
	def SetTextForeground(self, color):
		pass
		
	def SetTextBackground(self, color):
		pass
		
	def Clear(self):
		glClearColor(*(self.bgbrushcolor + (0,)))
		glClear(GL_COLOR_BUFFER_BIT)
		
	def DrawCircle(self, x, y, radius):
		pass
		
	def DrawRectangle(self, x, y, w, h):
		w -= 1
		h -= 1
		if self.brushcolor:
			glColor3f(*self.brushcolor)
			glBegin(GL_QUADS)
			glVertex2f(x,y)
			glVertex2f(x,y+h)
			glVertex2f(x+w,y+h)
			glVertex2f(x+w,y)
			glEnd()
		if self.pencolor:
			glColor3f(*self.pencolor)
			#~ x += 1
			#~ y += 1
			#~ w -= 2
			#~ h -= 2
			glBegin(GL_LINE_LOOP)
			glVertex2f(x,y)
			glVertex2f(x,y+h)
			glVertex2f(x+w,y+h)
			glVertex2f(x+w,y)
			glEnd()
		
	def DrawPolygon(self, points):
		if self.brushcolor:
			glColor3f(*self.brushcolor)
			glBegin(GL_POLYGON)
			for pt in points:
				glVertex2f(pt.x,pt.y)
			glEnd()
		if self.pencolor:
			glColor3f(*self.pencolor)
			glBegin(GL_LINE_LOOP)
			for pt in points:
				glVertex2f(pt.x,pt.y)
			glEnd()
		
	def SetLogicalFunction(self, func):
		pass
		
	def DrawLine(self, x1, y1, x2, y2):
		if self.pencolor:
			glColor3f(*self.pencolor)
			glBegin(GL_LINES)
			glVertex2f(x1,y1)
			glVertex2f(x2,y2)
			glEnd()
		
	def DrawLabel(self, text, rect, flags):
		pass
		
	def DestroyClippingRegion(self):
		pass
		
	def SetClippingRegion(self, region):
		pass
		
	def SetClippingRegionAsRegion(self, region):
		pass

class GLCanvas(wx.glcanvas.GLCanvas):
	def ReDraw(self,*args,**kargs):
		pass

	def __init__(self, *args, **kargs):
		self.buffer = None
		self.redraw_args = None
		wx.glcanvas.GLCanvas.__init__(self, *args, **kargs)

		self.ReDraw()

		self.Bind(wx.EVT_SIZE, self.onSize)
		self.Bind(wx.EVT_PAINT, self.onPaint)
		
	def onSize(self, event):
		self.ReDraw()

	def ReDraw(self,*args,**kargs):
		self.Refresh()
		
	def Refresh(self):
		dc = GLClientDC(self)
		dc.BeginDrawing()
		self.buffer = dc
		self.DrawBuffer()
		self.onPostPaint(dc)
		dc.EndDrawing()
		
	def onPostPaint(self, dc):
		pass

	def onPaint(self, event):
		dc = wx.PaintDC(self)
		dc.BeginDrawing()
		gldc = GLClientDC(self)
		gldc.BeginDrawing()
		self.buffer = gldc
		self.DrawBuffer()
		self.onPostPaint(gldc)
		gldc.EndDrawing()
		dc.EndDrawing()

	def Blit(self, dc):
		pass

	def GetBoundingRect(self):
		x, y = self.GetPosition()
		w, h = self.GetSize()
		return(x, y + h, x + w, y)

	def DrawBuffer(self,*args,**kargs):
		pass
