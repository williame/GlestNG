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
                
def HermiteInterpolate(y0,y1,y2,y3,mu,tension,bias):
    # from http://paulbourke.net/miscellaneous/interpolation/
    tension = (1.-tension)/2
    mu2 = mu * mu
    mu3 = mu2 * mu
    m0  = (y1-y0)*(1.+bias)*tension
    m0 += (y2-y1)*(1.-bias)*tension
    m1  = (y2-y1)*(1.+bias)*tension
    m1 += (y3-y2)*(1.-bias)*tension
    a0 =  2*mu3 - 3*mu2 + 1
    a1 =    mu3 - 2*mu2 + mu
    a2 =    mu3 -   mu2
    a3 = -2*mu3 + 3*mu2
    return(a0*y1+a1*m0+a2*m1+a3*y2)

def HermiteSpline(Y): # Y is a path of points
    if len(Y) < 2: return ()
    SEG = 10
    tension, bias = 0.1, 0.
    spline = []
    for y in xrange(len(Y)-1):
        y0 = Y[y-1] if (y > 0) else Y[0]
        y1 = Y[y]
        y2 = Y[y+1]
        y3 = Y[y+2] if (y < len(Y)-2) else Y[-1]
        for mu in xrange(SEG+1):
            mu *= 1./SEG
            spline.append((y,mu,(
                HermiteInterpolate(y0[0],y1[0],y2[0],y3[0],mu,tension,bias),
                HermiteInterpolate(y0[1],y1[1],y2[1],y3[1],mu,tension,bias),
                HermiteInterpolate(y0[2],y1[2],y2[2],y3[2],mu,tension,bias))))
    return spline
    
def triangles(pt,width):
    halfwidth = width/2 
    sqrwidth = width**2
    def side(a,b,c):
        _,_,(x1,y1,z1) = a
        _,_,(x2,y2,z2) = b
        _,_,(x3,y3,z3) = c
        theta = math.atan2(z3 - z1,x3 - x1)
        sin = halfwidth * math.sin(theta)
        cos = halfwidth * math.cos(theta)
        l = (x2+sin,y2,z2-cos)
        r = (x2-sin,y2,z2+cos)
        return (l,r)
    tri = []
    left,right = side(pt[0],pt[0],pt[1])
    for i in xrange(len(pt)):
        l,r = side(pt[i],pt[min(i+1,len(pt)-1)],pt[min(i+2,len(pt)-1)])
        if sum((p-q)**2 for p,q in zip(l,left)) > sqrwidth:
            tri.append((left,right,l))
            left = l
        if sum((p-q)**2 for p,q in zip(r,right)) > sqrwidth:
            tri.append((right,left,r))
            right = r
    glColor(1,1,1)
    glBegin(GL_TRIANGLES)
    for i,t in enumerate(tri):
        if i % 2 == 1:
            glColor(0,.5,1)
        else:
            glColor(.5,1,0)
        for v in t:
            glVertex(*v)
    glEnd()
    return tri

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
        self.info = False
    def draw(self,event):
        glClearColor(1,1,1,0)
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT)
        glScale(.8,.8,.8)        
        glDisable(GL_DEPTH_TEST)
        self.ground.draw()
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
        glLineWidth(4)
        spline = HermiteSpline(self.path)
        if len(spline) > 1:
            tri = triangles(spline,0.05)
            glColor(1,1,1)
            glBegin(GL_LINE_STRIP)
            for _,_,v in spline:
                glVertex(*v)
            glEnd()
            if self.info:
                self.info = False
                print len(self.path),"points"
                print "  ->",len(spline),"centre-line"
                print "  ->",len(tri),"triangles"
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
            self.info = True
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
