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
    tension, bias = 0.01, 0.5
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
    def feq(a,b,tolerance):
        return abs(a-b) < tolerance
    tri = []
    left,right = side(pt[0],pt[0],pt[1])
    for i in xrange(len(pt)):
        i1,i2 = min(i+1,len(pt)-1),min(i+2,len(pt)-1)
        y,mu,_ = pt[i1]
        l,r = side(pt[i],pt[i1],pt[i2])
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
    
def draw_circle(a,r,*rgb):
    cx,y,cz = a
    glColor(*rgb)
    num_segments = 4*4
    theta = 2.0 * 3.1415926 / float(num_segments)
    c = math.cos(theta)
    s = math.sin(theta)
    x, z = r, 0
    glBegin(GL_LINE_LOOP)
    for i in xrange(num_segments):
        glVertex(x + cx, y, z + cz) 
        t = x
        x = c * x - s * z
        z = s * t + c * z
    glEnd()

def tangents(a,r1,b,r2):
    x1,y1,z1 = a
    x2,y2,z2 = b
    d_sq = (x1 - x2) * (x1 - x2) + (z1 - z2) * (z1 - z2)
    if (d_sq <= (r1-r2)*(r1-r2)): return ()
    d = math.sqrt(d_sq)
    vx = (x2 - x1) / d
    vz = (z2 - z1) / d 
    res = []
    c = (r1 - 1 * r2) / d
    h = math.sqrt(max(0.0, 1.0 - c*c))
    for sign2 in (+1,-1):
        nx = vx * c - sign2 * h * vz
        nz = vz * c + sign2 * h * vx
        res.append((x1 + r1 * nx,y1,z1 + r1 * nz))
        res.append((x2 + 1 * r2 * nx,y2,z2 + 1 * r2 * nz))
    return res
    
def distance_line_to_point(line,pt):
    (x1,_,z1),(x2,_,z2) = line
    x,y,z = pt
    A = x - x1
    B = z - z1
    C = x2 - x1
    D = z2 - z1
    return abs(A * D - C * B) / math.sqrt(C * C + D * D)

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
        if len(self.path) > 1:
            def draw_left(a,b):
                glColor(0,0.5,1)
                glBegin(GL_LINES)
                glVertex(*a)
                glVertex(*b)
                glEnd()
            def draw_right(a,b):
                glColor(0,1,0.5)
                glBegin(GL_LINES)
                glVertex(*a)
                glVertex(*b)
                glEnd()
            glLineWidth(2)
            path = quad_line(self.path[0],self.path[1],self.OUTER-self.INNER)
            prev = self.path[0]
            draw_circle(prev,self.MARK,1,0.6,0.4)
            l,r = path[0],path[3]
            prev_to_left = True
            for i in xrange(1,len(self.path)-1):
                pt = self.path[i]
                tan_outer = tangents(prev,self.OUTER,pt,self.OUTER)
                draw_circle(pt,self.MARK,1,0.6,0.4)
                draw_circle(pt,self.INNER,0,0.6,0.4)
                next = self.path[i+1]
                to_left = distance_line_to_point((prev,next),tan_outer[1]) > \
                    distance_line_to_point((prev,next),tan_outer[3])
                if to_left:
                    draw_circle(pt,self.OUTER,0,0.6,0.4)
                    idx = 1
                else:
                    draw_circle(pt,self.OUTER,1,0.6,0.4)
                    idx = 3
                tan_inner = tangents(prev,self.INNER,pt,self.INNER)
                left = tan_inner[idx]
                right = tan_outer[idx]
                if prev_to_left != to_left:
                    left, right = right, left
                    prev_to_left = to_left
                tan_inner = tangents(pt,self.INNER,next,self.INNER)
                tan_outer = tangents(pt,self.OUTER,next,self.OUTER)
                next_left = tan_inner[idx-1]
                next_right = tan_outer[idx-1]
                draw_left(l,left)
                draw_right(r,right)
                l, r, prev = next_left, next_right, pt
            path = quad_line([(p+q)/2. for p,q in zip(l,r)],self.path[-1],self.OUTER-self.INNER)
            draw_left(path[0],path[1])
            draw_right(path[3],path[2])
        if self.info:
            self.info = False
            print len(self.path),"points"
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
        while (point is not None):
            selected = error = None
            for i,pt in enumerate(self.path):
                if math.sqrt(sum((p-q)**2 for p,q in zip(point,pt))) <= (self.OUTER*2):
                    if selected is not None:
                        print "ARGH"
                        error = True
                        break
                    else:
                        selected = i
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
