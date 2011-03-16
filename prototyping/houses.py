#!/usr/bin/env python

import pygtk; pygtk.require('2.0')
import gtk, gtk.gdk as gdk, gtk.gtkgl as gtkgl, gtk.gdkgl as gdkgl, gobject
from OpenGL.GL import *
from OpenGL.GLU import *
from OpenGL.GLUT import *
from random import random
import numpy, math

import zpr

def is_vec(a):
    return isinstance(a,tuple) and (len(a)==3)
def vec_cross(a,b):
    assert is_vec(a) and is_vec(b)
    return ( \
        (a[1]*b[2]-a[2]*b[1]),
        (a[2]*b[0]-a[0]*b[2]),
        (a[0]*b[1]-a[1]*b[0]))
def vec_dot(a,b):
    assert is_vec(a) and is_vec(b)
    return sum(u*v for u,v in zip(a,b))
    
class Point:
    def __init__(self,*xyz):
        if len(xyz)==0:
            self.x = self.y = self.z = None
        else:
            self.x, self.y, self.z = xyz
    def reset(self,x,y,z):
        self.x, self.y, self.z = x,y,z
    def cross(self,p):
        return Point((self.y*p.z-self.z*p.y),
            (self.z*p.x-self.x*p.z),
            (self.x*p.y-self.y*p.x))
    def dot(self,p):
        return self.x*p.x + self.y*p.y + self.z*p.z
    def magnitude(self):
        d = self.dot(self)
        if d > 0: d = math.sqrt(d)
        return d
    def distance_sqrd(self,p):
        return sum((a-b)**2 for a,b in zip(self,p))
    def distance(self,p):
        return math.sqrt(self.distance_sqrd(p))
    def normal(self):
        d = self.magnitude()
        if d: return self / d
        return self
    def __sub__(self,p):
        if isinstance(p,Point):
            return Point(self.x-p.x,self.y-p.y,self.z-p.z)
        return Point(self.x-p,self.y-p,self.z-p)
    def __mul__(self,p):
        if isinstance(p,Point):
            return Point(self.x*p.x,self.y*p.y,self.z*p.z)
        return Point(self.x*p,self.y*p,self.z*p)
    def __add__(self,p):
        if isinstance(p,Point):
            return Point(self.x+p.x,self.y+p.y,self.z+p.z)
        return Point(self.x+p,self.y+p,self.z+p)
    def __div__(self,p):
        if isinstance(p,Point):
            return Point(self.x/p.x,self.y/p.y,self.z/p.z)
        return Point(self.x/p,self.y/p,self.z/p)
    def __neg__(self):
        return Point(-self.x,-self.y,-self.z)
    def __call__(self):
        return self
    def numpy(self):
        return numpy.array((self.x,self.y,self.z),dtype=numpy.float32)
    def xyz(self):
        return (self.x,self.y,self.z)
    def __getitem__(self,i):
        if i == 0: return self.x
        if i == 1: return self.y
        if i == 2: return self.z
        raise IndexError()
    def __repr__(self):
        return "P(%s,%s,%s)"%(self.x,self.y,self.z)
    
class Mix2:
    def __init__(self,p1,weight,p2):
        assert isinstance(p1,Point)
        assert isinstance(p2,Point)
        self.o = p1
        self.d = p2-p1
        self.w = weight
    def __call__(self):
        return self.o + self.d * self.w
        
class Quad:
    def __init__(self,*mix):
        assert len(mix) in (4,5,6)
        self.m = mix[:4]
        if len(mix) > 4:
            self.n = mix[4]
            if self.n is not None and not isinstance(self.n,Point):
                self.n = Point(*self.n)
        else:
            self.n = None
        if len(mix) > 5:
            self.c = mix[5]
        else:
            self.c = None
    def normal(self):
        if self.n is not None: return self.n
        m0 = self.m[0]()
        u = self.m[1]()-m0
        v = self.m[3]()-m0
        return u.cross(v).normal()
    def vertices(self):
        return (self.m[0](),self.m[1](),self.m[2](),self.m[3]())
    def intersection(self,ray_origin,ray_dir):
        v = [v.numpy() for v in self.vertices()]
        I = zpr.ray_triangle(ray_origin,ray_dir,(v[0],v[1],v[2]))
        if I is None:
            I = zpr.ray_triangle(ray_origin,ray_dir,(v[2],v[3],v[0]))
        if I is not None:
            return Point(*I)
    def draw(self,selected,guide,outline):
        if guide: colour = (.4,.4,1.,.2) if outline else (.8,.8,1.,.2)
        else: colour = (1,0,0) if selected else (0,1,0)
        glColor(*colour)
        glNormal(*self.normal())
        glBegin(GL_LINE_LOOP if outline else GL_QUADS)
        for pt in self.vertices():
            glVertex(*pt.xyz())
        glEnd()
    
class House:
    FACE = "face"
    GUIDE = "guide"
    def __init__(self,mgr):
        self.mgr = mgr
        self.pt = pt = (0.,0.,0.)
        self.w, self.h, self.d = w,h,d = 1.,1.,1.
        self.tlf = tlf = Point()
        self.tlb = tlb = Point()
        self.blf = blf = Point()
        self.blb = blb = Point()
        self.trf = trf = Point()
        self.trb = trb = Point()
        self.brf = brf = Point()
        self.brb = brb = Point()
        self.compute_corners()
        self.guides = ( \
            Quad(tlf,trf,brf,blf), #,(0,0,-1)), # front
            Quad(trf,trb,brb,brf), #,(-1,0,0)), # right
            Quad(tlb,tlf,blf,blb), #,(1,0,0)), # left
            Quad(trb,trf,tlf,tlb), #,(0,-1,0)), # top
            Quad(blb,blf,brf,brb), #,(0,1,0)), # bottom
            Quad(trb,tlb,blb,brb)) #,(0,0,1))) # back
        self.selected = Quad(Mix2(tlf,.2,blb),Mix2(blf,.2,tlb),
            Mix2(brf,.2,trb),Mix2(trf,.2,brb),None,(1.,.4,.4))
        self.faces = [self.selected]
    def compute_corners(self):
        pt = self.pt
        w, h, d = self.w,self.h,self.d
        self.tlf.reset(-w/2.,h/2.,d/2.)
        self.tlb.reset(-w/2.,h/2.,-d/2.)
        self.blf.reset(-w/2.,-h/2.,d/2.)
        self.blb.reset(-w/2.,-h/2.,-d/2.)
        self.trf.reset(w/2.,h/2.,d/2.)
        self.trb.reset(w/2.,h/2.,-d/2.)
        self.brf.reset(w/2.,-h/2.,d/2.)
        self.brb.reset(w/2.,-h/2.,-d/2.)
    def pick(self,ray_origin,ray_dir):
        hits = []
        for typ,faces in ((self.FACE,self.faces),(self.GUIDE,self.guides)):
            for face in faces:
                I = face.intersection(ray_origin,ray_dir)
                if I is not None:
                    hits.append((I.distance(ray_origin),typ,face,I))
        return hits
    def draw(self,event):
        glEnable(GL_CULL_FACE)
        # draw guides
        glLineWidth(1)
        for face in self.guides:
            face.draw(False,True,True)
            face.draw(False,True,False)
        # draw model
        for face in self.faces:
            face.draw(face == self.selected,False,False)
        # draw guide outlines again
        glLineWidth(2)
        for face in self.guides:
            face.draw(False,True,True)

class Editor(zpr.GLZPR):
    def __init__(self):
        zpr.GLZPR.__init__(self)
        self.model = House(self)
    def init(self):
        zpr.GLZPR.init(self)
        glEnable(GL_BLEND)
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA)
    def draw(self,event):
        glClearColor(.8,.8,.7,0)
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT)
        self.model.draw(event)
    def keyPress(self,event):
        if event.keyval == gtk.keysyms.Escape:
            gtk.main_quit()
            return
        if event.keyval > 255:
            print "cannot handle key",event.keyval,"- type H for help"
            return
        key = chr(event.keyval)
        if key in "eE":
            self.screenshot("houses.png")
        else:
            print "cannot handle key",key,"(%d)"%ord(key)
           
    def _pick(self,x,y,dx,dy,event):
        modelview = numpy.matrix(glGetDoublev(GL_MODELVIEW_MATRIX))
        projection = numpy.matrix(glGetDoublev(GL_PROJECTION_MATRIX))
        viewport = glGetIntegerv(GL_VIEWPORT)
        R = numpy.array([gluUnProject(x,y,10,modelview,projection,viewport),
            gluUnProject(x,y,-10,modelview,projection,viewport)],
            dtype=numpy.float32)
        ray_origin, ray_dir = R[0], R[1]-R[0]
        hits = self.model.pick(ray_origin,ray_dir)
        if len(hits) == 0: return ((),())
        hits.sort()
        print x,y,"\n\t","\n\t".join(str(h) for h in hits)
        return ((),())
    def pick(self,*args):
        pass

glutInit(())
gtk.gdk.threads_init()
window = gtk.Window(gtk.WINDOW_TOPLEVEL)
window.set_title("GlestNG Houses Prototyping")
window.set_size_request(1024,768)
window.connect("destroy",lambda event: gtk.main_quit())
vbox = gtk.VBox(False, 0)
window.add(vbox)
vbox.pack_start(Editor(),True,True)
window.show_all()
gtk.main()
