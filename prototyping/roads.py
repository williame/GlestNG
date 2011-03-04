#!/usr/bin/env python

import pygtk; pygtk.require('2.0')
import gtk, gtk.gdk as gdk, gtk.gtkgl as gtkgl, gtk.gdkgl as gdkgl, gobject
from OpenGL.GL import *
from OpenGL.GLU import *
from OpenGL.GLUT import *
from random import random
import numpy, math

import zpr

class Ground:
    def __init__(self):
        self.triangles = numpy.array((((-1,-1,0),(-1,1,0),(1,1,0)),
            ((1,1,0),(1,-1,0),(-1,-1,0))),dtype=numpy.float32)
    def draw(self):
        glColor(0,0.6,0,1) # dark green
        glBegin(GL_TRIANGLES)
        for triangle in self.triangles:
            for vertex in triangle:
                glVertex(*vertex)
        glEnd()
    def pick(self,ray):
        for triangle in self.triangles:
            I = zpr.ray_triangle(ray[0],ray[1],triangle)
            if I is not None:
                return I

def quad_line(p1,p2,width=0.05):
    x1, y1, z1 = p1
    x2, y2, z2 = p2
    angle = math.atan2(z2 - z1,x2 - x1)
    sin = width / 2. * math.sin(angle)
    cos = width / 2. * math.cos(angle)
    return ( \
        (x1 + sin, y1, z1 - cos),
        (x2 + sin, y2, z2 - cos),
        (x2 - sin, y2, z2 + cos),
        (x1 - sin, y1, z1 + cos)
        )

class Point:
    def __init__(self,x,y):
        self.x = float(x)
        self.y = float(y)
    def __div__(self,n):
        self.x /= n
        self.y /= n
    def distance(self,other):
        return math.sqrt(self.distance_sqrd(other))
    def distance_sqrd(self,other):
        return (self.x-other.x)**2 + (self.y-other.y)**2
    def circle(self,radius):
        return Circle(self,radius)
    def line(self,to):
        return Line(self,to)
    def to_3d(self):
        return (self.x,self.y,0)
        
class Line:
    def __init__(self,a,b):
        self.a, self.b = a, b
        self.dx, self.dy = (self.b.x - self.a.x), (self.b.y - self.a.y)
    def closest(self,pt):
        U = (pt.x-self.a.x)*self.dx + (pt.y-self.a.y)*self.dy
        U /= self.a.distance_sqrd(self.b)
        if U >= 1.: return self.b
        if U <= 0.: return self.a
        return self.interpolate(U)
    def distance(self,pt):
        return abs((pt.x-self.a.x)*self.dy - self.dx*(pt.y-self.a.y)) / \
            math.sqrt(self.dx**2 + self.dy**2)
    def length(self):
        return self.a.distance(self.b)
    def interpolate(self,U):
        return Point(self.a.x + U * self.dx,self.a.y + U * self.dy)
    def normal(self):
        U = self.length()
        x = (self.b.x-self.a.x) / U
        y = (self.b.y-self.a.y) / U
        return Line(self.a,Point(self.a.x+x,self.a.y+y))
    def side(self,pt):
        """0 = on, <0 = right, >0 = left"""
        return (pt.y-self.a.y)*self.dx-(pt.x-self.a.x)*self.dy
    def draw(self,*rgb):
        glColor(*rgb)
        glBegin(GL_LINES)
        glVertex(*self.a.to_3d())
        glVertex(*self.b.to_3d())
        glEnd()
    
class Circle:
    def __init__(self,pt,radius):
        self.pt = pt
        self.radius = radius
    def draw(self,*rgb):
        glColor(*rgb)
        num_segments = 4*4
        theta = 2.0 * 3.1415926 / float(num_segments)
        c = math.cos(theta)
        s = math.sin(theta)
        x, y = self.radius, 0
        glBegin(GL_LINE_LOOP)
        for i in xrange(num_segments):
            glVertex(x + self.pt.x, y + self.pt.y) 
            t = x
            x = c * x - s * y
            y = s * t + c * y
        glEnd()
    def outer_tangents(self,other):
        return self._tangents(other,+1)
    def inner_tangents(self,other):
        return self._tangents(other,-1)
    def _tangents(self,other,sign1):
        d_sq = self.pt.distance_sqrd(other.pt)
        if (d_sq <= (self.radius-other.radius)**2): raise Exception("containment")
        d = math.sqrt(d_sq)
        vx = (other.pt.x - self.pt.x) / d
        vy = (other.pt.y - self.pt.y) / d 
        res = []
        c = (self.radius - sign1 * other.radius) / d
        if (c*c > 1.0): raise Exception("overlap")
        h = math.sqrt(max(0.0, 1.0 - c**2))
        for sign2 in (+1,-1):
            nx = vx * c - sign2 * h * vy
            ny = vy * c + sign2 * h * vx
            res.append(Line( \
                Point(self.pt.x + self.radius * nx,self.pt.y + self.radius * ny),
                Point(other.pt.x + sign1 * other.radius * nx,other.pt.y + sign1 * other.radius * ny)))
        return tuple(res)

class RoadMaker(zpr.GLZPR):
    MARK, INNER, OUTER = 0.01, 0.03, 0.09
    def __init__(self):
        zpr.GLZPR.__init__(self)
        self.ground = Ground()
        self.path = []
        self.end_point = None
        self.info = False
    def draw(self,event):
        glClearColor(1,1,1,0)
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT)
        glScale(.8,.8,.8)        
        glDisable(GL_DEPTH_TEST)
        self.ground.draw()
        glLineWidth(2)
        if self.end_point is not None:
            self.end_point.circle(0.01).draw(1,0,0)
        if len(self.path) > 1:
            pivots = []
            for pt in self.path:
                pt.circle(self.MARK).draw(0.8,0.8,1)
            # compute pivots
            pivot = self.path[0].circle((self.OUTER-self.INNER)/2.)
            LEFT, ON, RIGHT = "L", "-", "R"
            pivots.append((ON,pivot,None))
            for p in xrange(1,len(self.path)):
                from_side, from_inner, from_outer = pivots[-1]
                prev = self.path[p-1]
                pt = self.path[p]
                if p == len(self.path)-1: #last one?
                    pivot = self.path[-1].circle((self.OUTER-self.INNER)/2.)
                    pivots.append((ON,pivot,None))
                else:
                    next = self.path[p+1]
                    pivot = Line(pt,Line(pt.line(prev).normal().b,pt.line(next).normal().b).interpolate(.5))
                    pivot = pivot.interpolate(((self.INNER+self.OUTER)/2.) / pivot.length())
                    side = Line(prev,pt).side(pivot)
                    side = LEFT if side > 0. else ON if side == 0. else RIGHT
                    inner = pivot.circle(self.INNER)
                    outer = pivot.circle(self.OUTER) if side is not ON else None
                    pivots.append((side,inner,outer))
                to_side, to_inner, to_outer = pivots[-1]
                # draw it
                to_inner.draw(.7,.7,.7)
                if to_outer is not None: to_outer.draw(.7,.7,.7)
                left = right = None
                if (from_side == LEFT) and (to_side == LEFT):
                    left = from_inner.outer_tangents(to_inner)[1]
                    right = from_outer.outer_tangents(to_outer)[1]
                elif (from_side == RIGHT) and (to_side == RIGHT):
                    left = from_outer.outer_tangents(to_outer)[0]
                    right = from_inner.outer_tangents(to_inner)[0]
                elif (from_side == LEFT) and (to_side == RIGHT):
                    left = from_inner.inner_tangents(to_outer)[1]
                    right = from_outer.inner_tangents(to_inner)[1]
                elif (from_side == RIGHT) and (to_side == LEFT):
                    left = from_outer.inner_tangents(to_inner)[0]
                    right = from_inner.inner_tangents(to_outer)[0]
                elif (from_side == ON) and (to_side == LEFT):
                    left = from_inner.inner_tangents(to_inner)[0]
                    right = from_inner.outer_tangents(to_outer)[1]
                elif (from_side == ON) and (to_side == RIGHT):
                    left = from_inner.outer_tangents(to_outer)[0]
                    right = from_inner.inner_tangents(to_inner)[1]
                elif (from_side == LEFT) and (to_side == ON):
                    left = from_inner.inner_tangents(to_inner)[1]
                    right = from_outer.outer_tangents(to_inner)[1]
                elif (from_side == RIGHT) and (to_side == ON):
                    left = from_outer.outer_tangents(to_inner)[0]
                    right = from_inner.inner_tangents(to_inner)[0]
                elif (from_side == ON) and (to_side == ON):
                    left = from_inner.outer_tangents(to_inner)[0]
                    right = from_inner.outer_tangents(to_inner)[1]
                else:
                    self.path[p].line(self.path[p-1]).draw(0.8,0.8,0.8)
                if left is not None: left.draw(1,1,1)
                if right is not None: right.draw(1,.4,1)
        if self.info:
            self.info = False
            print len(self.path),"points"
        glEnable(GL_DEPTH_TEST)
        
    def keyPress(self,event):
        if event.keyval == gtk.keysyms.Escape:
            gtk.main_quit()
            return
        if event.keyval > 255:
            print "cannot handle key",event.keyval,"- type H for help"
            return
        key = chr(event.keyval)
        if key in "sS":
            self.screenshot("roads.png")
        else:
            print "cannot handle key",key,"(%d)"%ord(key)
           
    def _pick(self,x,y,dx,dy,event):
        glScale(.8,.8,.8)
        modelview = numpy.matrix(glGetDoublev(GL_MODELVIEW_MATRIX))
        projection = numpy.matrix(glGetDoublev(GL_PROJECTION_MATRIX))
        viewport = glGetIntegerv(GL_VIEWPORT)
        ray = numpy.array([gluUnProject(x,y,10,modelview,projection,viewport),
            gluUnProject(x,y,-10,modelview,projection,viewport)],
            dtype=numpy.float32)
        ray[1] -= ray[0]
        point = self.ground.pick(ray)
        while (point is not None):
            assert point[2] == 0
            point = Point(point[0],point[1])
            selected = error = None
            for i,pt in enumerate(self.path):
                d = pt.distance(point)
                if d <= self.OUTER:
                    if selected is not None:
                        print "ARGH"
                        error = True
                        break
                    else:
                        selected = i
                elif d <= (self.OUTER*2):
                    print "OGH"
                    error = True
                    break
            if error: break
            if selected:
                self.path[selected] = point
                if selected == len(self.path)-1:
                    self.end_point = point
            else:
                self.end_point = point
                self.path.append(self.end_point)
            self.info = True
            break
        return ((),())
    def pick(self,*args):
        pass

glutInit(())
gtk.gdk.threads_init()
window = gtk.Window(gtk.WINDOW_TOPLEVEL)
window.set_title("GlestNG Roads Prototyping")
window.set_size_request(1024,768)
window.connect("destroy",lambda event: gtk.main_quit())
vbox = gtk.VBox(False, 0)
window.add(vbox)
vbox.pack_start(RoadMaker(),True,True)
window.show_all()
gtk.main()