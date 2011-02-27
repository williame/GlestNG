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
        self.triangles = numpy.array((((-1,-1,-1),(1,-1,-1),(1,-1,1)),
            ((-1,-1,-1),(1,-1,1),(-1,-1,1))),dtype=numpy.float32)
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

class RoadMaker(zpr.GLZPR):
    def __init__(self):
        zpr.GLZPR.__init__(self)
        self.ground = Ground()
        self.path = []
        self.end_point = None
    def draw(self,event):
        glClearColor(1,1,1,0)
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT)
        glScale(.8,.8,.8)        
        glDisable(GL_DEPTH_TEST)
        self.ground.draw()
        # draw path
        glColor(1,1,1,1)
        glBegin(GL_QUADS)
        for i in xrange(1,len(self.path)):
            for vertex in quad_line(self.path[i-1],self.path[i]):
                glVertex(*vertex)
        glEnd()
        if self.end_point is not None:
            # draw a cross
            glColor(1,0,0,1)
            glPushMatrix()
            glTranslate(*self.end_point)
            glRotate(45,0,1,0)
            glScale(.1,.1,.1)
            glBegin(GL_QUADS)
            glVertex(-.2,0,-1)
            glVertex(-.2,0,1)
            glVertex(.2,0,1)
            glVertex(.2,0,-1)
            glVertex(-1,0,.2)
            glVertex(-1,0,-.2)
            glVertex(1,0,-.2)
            glVertex(1,0,.2)
            glEnd()
            glPopMatrix()
        glEnable(GL_DEPTH_TEST)
            
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
        if point is not None:
            self.end_point = point
            self.path.append(self.end_point)
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
