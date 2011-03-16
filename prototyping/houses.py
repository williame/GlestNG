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
        
def call(c):
    if callable(c): c = c()
    return c
    
class Ofs:
    def __init__(self,x,y,z):
        self.x,self.y,self.z = x,y,z
    def __call__(self):
        return Point(call(self.x),call(self.y),call(self.z))

class Rel:
    def __init__(self,anchor,ofs):
        self.anchor, self.ofs = anchor, ofs
    def __call__(self):
        return call(self.anchor) + call(self.ofs)

class Mix2:
    def __init__(self,p1,weight,p2):
        self.p1, self.weight, self.p2 = p1,weight,p2
    def __call__(self):
        p1, p2 = call(self.p1), call(self.p2)
        return p1 + (p2-p1) * call(self.weight)
        
class Weight:
    def __init__(self,weight):
        self.weight = weight
    def __call__(self):
        return call(self.weight)
        
class Face:
    def __init__(self,name,*mix):
        self.name = name
        assert len(mix) > 2
        self.m = mix
        if len(mix) == 4:
            self.tl, self.bl, self.br, self.tr = self.m
    def normal(self):
        m0 = self.m[0]()
        u = self.m[1]()-m0
        v = self.m[2]()-m0
        return u.cross(v).normal()
    def vertices(self):
        return [call(m) for m in self.m]
    def intersection(self,ray_origin,ray_dir):
        v = [v.numpy() for v in self.vertices()]
        I = zpr.ray_triangle(ray_origin,ray_dir,(v[0],v[1],v[2]))
        if (I is None) and (len(v) > 3):
            I = zpr.ray_triangle(ray_origin,ray_dir,(v[2],v[3],v[0]))
        if I is None: return ()
        return ((Point(*I),self),)
    def draw(self,guide,outline):
        if guide: colour = (.4,.4,1.,.2) if outline else (.8,.8,1.,.2)
        else: colour = (1,0,0)
        glColor(*colour)
        glNormal(*self.normal())
        glBegin(GL_LINE_LOOP if outline else GL_POLYGON)
        for pt in self.vertices():
            glVertex(*pt.xyz())
        glEnd()
    def __repr__(self):
        return "Face<%s,%d>"%(self.name,len(self.m))
        
class Faces:
    """an arbitrary list of faces (or Faces)"""
    def __init__(self):
        self.faces = []
    def draw(self,guide,outline):        
        for face in self.faces:
            face.draw(guide,outline)
    def intersection(self,ray_origin,ray_dir):
        hits = []
        for face in self.faces:
            hits.extend(face.intersection(ray_origin,ray_dir))
        return hits
        
class Bounds(Faces):
    """defines a quad, does not fill it with sides"""
    def __init__(self,name,tlf,tlb,blf,blb,trf,trb,brf,brb):
        Faces.__init__(self)
        self.name = name
        self.tlf,self.tlb,self.blf,self.blb,self.trf,self.trb,self.brf,self.brb = \
            tlf,tlb,blf,blb,trf,trb,brf,brb

class Box(Bounds):
    """four sided box, optionally with a closed top"""
    def __init__(self,name,tlf,tlb,blf,blb,trf,trb,brf,brb,top):
        Bounds.__init__(self,name,tlf,tlb,blf,blb,trf,trb,brf,brb)
        self.front = front = Face("%s_front"%name,tlf,blf,brf,trf)
        self.back = back = Face("%s_back"%name,trb,brb,blb,tlb)
        self.left = left = Face("%s_left"%name,tlb,blb,blf,tlf)
        self.right = right = Face("%s_right"%name,trf,brf,brb,trb)
        self.faces.extend((front,back,left,right))
        if top:
            self.top = top = Face("%s_top"%name,trf,trb,tlb,tlf)
            self.faces.append(top)
        
class PitchRoof(Bounds):
    """a pitched roof i.e. an equal roof"""
    def __init__(self,name,tlf,tlb,blf,blb,trf,trb,brf,brb,left,right):
        Bounds.__init__(self,name,tlf,tlb,blf,blb,trf,trb,brf,brb)
        self.front = front = Face("%s_front"%name,Mix2(tlf,.5,tlb),blf,brf,Mix2(trf,.5,trb))
        self.back = back = Face("%s_back"%name,front.tr,brb,blb,front.tl)
        self.faces.extend((front,back))
        if left:
            self.left = left = Face("%s_left"%name,front.tl,back.br,front.bl)
            self.faces.append(left)
        if right:
            self.right = right = Face("%s_right"%name,front.tr,front.br,back.bl)
            self.faces.append(right)

class Chim(Bounds):
    def __init__(self,name,w,d,anchor,height):
        self.w, self.d, self.anchor = w,d,anchor
        x,z = w*.1,d*.1
        tlf = Rel(anchor,Ofs(x/2,height,-z/2))
        tlb = Rel(anchor,Ofs(x/2,height,z/2))
        blf = Rel(anchor,Ofs(x/2,0,-z/2))
        blb = Rel(anchor,Ofs(x/2,0,z/2))          
        trf = Rel(anchor,Ofs(-x/2,height,-z/2))
        trb = Rel(anchor,Ofs(-x/2,height,z/2))
        brf = Rel(anchor,Ofs(-x/2,0,-z/2))
        brb = Rel(anchor,Ofs(-x/2,0,z/2))
        Bounds.__init__(self,name,tlf,tlb,blf,blb,trf,trb,brf,brb)
        self.height = height = Weight(.8)
        self.base = base = Box("%s_base"%name,        
            Mix2(blf,height,tlf),Mix2(blb,height,tlb),blf,blb,
            Mix2(brf,height,trf),Mix2(brb,height,trb),brf,brb,True)
        self.faces.append(base)

class House(Faces):
    FACE = "face"
    GUIDE = "guide"
    def __init__(self,mgr):
        Faces.__init__(self)
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
        # guides are inverted so faces face inwards
        self.guides = ( \
            Face("guide_front",tlf,trf,brf,blf), #,(0,0,-1)), # front
            Face("guide_right",trf,trb,brb,brf), #,(-1,0,0)), # right
            Face("guide_left",tlb,tlf,blf,blb), #,(1,0,0)), # left
            Face("guide_top",trb,trf,tlf,tlb), #,(0,-1,0)), # top
            Face("guide_bottom",blb,blf,brf,brb), #,(0,1,0)), # bottom
            Face("guide_back",trb,tlb,blb,brb)) #,(0,0,1))) # back
        self.height = height = Weight(.6)
        self.base = base = Box("base",
            Mix2(blf,height,tlf),Mix2(blb,height,tlb),blf,blb,
            Mix2(brf,height,trf),Mix2(brb,height,trb),brf,brb,False)
        self.roof = roof = PitchRoof("roof",tlf,tlb,base.tlf,base.tlb,
            trf,trb,base.trf,base.trb,True,True)
        self.chim = chim = Chim("chim",1,2,Mix2(roof.front.bl,.3,roof.back.bl),height)
        self.faces = [base,roof,chim]
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
                for pt,face in face.intersection(ray_origin,ray_dir):
                    hits.append((pt.distance(ray_origin),typ,face,pt))
        return hits
    def draw(self,event):
        glEnable(GL_CULL_FACE)
        # draw guides
        glLineWidth(1)
        for face in self.guides:
            face.draw(True,True)
            face.draw(True,False)
        # draw model
        Faces.draw(self,False,False)
        # draw guide outlines again
        glLineWidth(2)
        for face in self.guides:
            face.draw(True,True)

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
        elif key in "qQxX":
            gtk.main_quit()
        elif (key in "cC") and self.event_masked(event,gdk.CONTROL_MASK):
            print "BREAK!"
            gtk.main_quit()
        else:
            print "cannot handle key",key,"(%d)"%ord(key)
           
    def _pick(self,x,y,dx,dy,event):
        modelview = numpy.matrix(glGetDoublev(GL_MODELVIEW_MATRIX))
        projection = numpy.matrix(glGetDoublev(GL_PROJECTION_MATRIX))
        viewport = glGetIntegerv(GL_VIEWPORT)
        R = numpy.array([gluUnProject(x,y,-10,modelview,projection,viewport),
            gluUnProject(x,y,10,modelview,projection,viewport)],
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

