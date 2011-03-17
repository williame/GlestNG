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
        
class Sub:
    def __init__(self,anchor,ofs):
        self.anchor, self.ofs = anchor, ofs
    def __call__(self):
        return call(self.anchor) - call(self.ofs)

class Mix2:
    def __init__(self,p1,weight,p2):
        self.p1, self.weight, self.p2 = p1,weight,p2
    def __call__(self):
        p1, p2 = call(self.p1), call(self.p2)
        return p1 + (p2-p1) * call(self.weight)
        
class Axis:
    def __init__(self,pt,axis):
        self.pt, self.axis = pt, axis
    def __call__(self):
        return call(self.pt)[call(self.axis)]

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
        self.colour = None
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
        else: colour = self.colour if self.colour is not None else (1,0,0)
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
    def _front(self,front):
        tlf, blf = Mix2(self.tlf,front,self.tlb), Mix2(self.blf,front,self.blb)
        trf, brf = Mix2(self.trf,front,self.trb), Mix2(self.brf,front,self.brb)
        return Bounds(None,tlf,self.tlb,blf,self.blb,trf,self.trb,brf,self.brb)
    def _back(self,back):
        tlb, blb = Mix2(self.tlb,back,self.tlf), Mix2(self.blb,back,self.blf)
        trb, brb = Mix2(self.trb,back,self.trf), Mix2(self.brb,back,self.brf)
        return Bounds(None,self.tlf,tlb,self.blf,blb,self.trf,trb,self.brf,brb)
    def _left(self,left):
        tlf, tlb = Mix2(self.tlf,left,self.trf), Mix2(self.tlb,left,self.trb)
        blf, blb = Mix2(self.blf,left,self.brf), Mix2(self.blb,left,self.brb)
        return Bounds(None,tlf,tlb,blf,blb,self.trf,self.trb,self.brf,self.brb)
    def _right(self,right):
        trf, trb = Mix2(self.trf,right,self.tlf), Mix2(self.trb,right,self.tlb)
        brf, brb = Mix2(self.brf,right,self.blf), Mix2(self.brb,right,self.blb)
        return Bounds(None,self.tlf,self.tlb,self.blf,self.blb,trf,trb,brf,brb)
    def _top(self,top):
        tlf, tlb = Mix2(self.tlf,top,self.blf), Mix2(self.tlb,top,self.blb)
        trf, trb = Mix2(self.trf,top,self.brf), Mix2(self.trb,top,self.brb)
        return Bounds(None,tlf,tlb,self.blf,self.blb,trf,trb,self.brf,self.brb)
    def _bottom(self,bottom):
        blf, blb = Mix2(self.blf,bottom,self.tlf), Mix2(self.blb,bottom,self.tlb)
        brf, brb = Mix2(self.brf,bottom,self.trf), Mix2(self.brb,bottom,self.trb)
        return Bounds(None,self.tlf,self.tlb,blf,blb,self.trf,self.trb,brf,brb)
    def sub(self,left,right,front,back,top,bottom):
        b = self
        if left != 0: b = b._left(left)
        if right != 0: b = b._right(right/(1.-left))
        if front != 0: b = b._front(front)
        if back != 0: b = b._back(back/(1.-front))
        if top != 0: b = b._top(top)
        if bottom != 0: b = b._bottom(bottom/(1.-top))
        return b
    def __getitem__(self,i):
        return (self.tlf,self.tlb,self.blf,self.blb,self.trf,self.trb,self.brf,self.brb)[i]

class Box(Bounds):
    """four sided box, optionally with a closed top"""
    def __init__(self,name,top,tlf,tlb,blf,blb,trf,trb,brf,brb):
        Bounds.__init__(self,name,tlf,tlb,blf,blb,trf,trb,brf,brb)
        self.front = front = Face("%s_front"%name,tlf,blf,brf,trf)
        self.back = back = Face("%s_back"%name,trb,brb,blb,tlb)
        self.left = left = Face("%s_left"%name,tlb,blb,blf,tlf)
        self.right = right = Face("%s_right"%name,trf,brf,brb,trb)
        self.faces.extend((front,back,left,right))
        if top:
            self.top = top = Face("%s_top"%name,trf,trb,tlb,tlf)
            self.faces.append(top)
            
class Column(Bounds):
    MAX = 0.03
    def __init__(self,name,top,tlf,tlb,blf,blb,trf,trb,brf,brb):
        Bounds.__init__(self,name,tlf,tlb,blf,blb,trf,trb,brf,brb)
        self.top = top
        self.colour = None
    def draw(self,guide,outline):
        # compute the xz
        tlf = call(self.tlf)
        centre = (tlf+call(self.trb))/2.
        top = centre.y
        bottom = ((call(self.blf)+call(self.brb))/2.).y
        r = min(*(centre-tlf))
        num_segments = 10
        theta = 2. * 3.1415926 / float(num_segments)
        c = math.cos(theta)
        s = math.sin(theta)
        x, z = r, 0
        xz = [(x+centre.x,z+centre.z)]
        for i in xrange(num_segments):
            t = x
            x = c * x - s * z
            z = s * t + c * z
            xz.append((x+centre.x,z+centre.z))
        xz.reverse()
        # draw it
        glColor(*self.colour if self.colour is not None else (0,1,0))
        glBegin(GL_QUAD_STRIP)
        for x,z in xz:
            p = Point(x,top,z)
            glNormal(*-(p-centre).normal())
            glVertex(*p)
            p = Point(x,bottom,z)
            glNormal(*-(p-centre).normal())
            glVertex(*p)
        glEnd()
        glNormal(0,1,0)
        if self.top:
            glBegin(GL_POLYGON)
            for x,z in xz:
                glVertex(x,top,z)
            glEnd()
        
class PitchRoof(Bounds):
    """a pitched roof i.e. an equal roof"""
    def __init__(self,name,left,right,tlf,tlb,blf,blb,trf,trb,brf,brb):
        Bounds.__init__(self,name,tlf,tlb,blf,blb,trf,trb,brf,brb)
        self.front = front = Face("%s_front"%name,Mix2(tlf,.5,tlb),blf,brf,Mix2(trf,.5,trb))
        self.back = back = Face("%s_back"%name,front.tr,brb,blb,front.tl)
        front.colour = back.colour = (.3,.3,.2)
        self.faces.extend((front,back))
        if left:
            self.left = left = Face("%s_left"%name,front.tl,back.br,front.bl)
            self.faces.append(left)
        if right:
            self.right = right = Face("%s_right"%name,front.tr,front.br,back.bl)
            self.faces.append(right)

class Chimney(Bounds):
    def __init__(self,name,w,d,top,bottom):
        self.w, self.d, self.top, self.bottom = w,d,top,bottom
        x,z = (w*.1)/2,(d*.1)/2
        tlf = Rel(top,Ofs(x,0,-z))
        tlb = Rel(top,Ofs(x,0,z))
        blf = Rel(bottom,Ofs(x,0,-z))
        blb = Rel(bottom,Ofs(x,0,z))
        trf = Rel(top,Ofs(-x,0,-z))
        trb = Rel(top,Ofs(-x,0,z))
        brf = Rel(bottom,Ofs(-x,0,-z))
        brb = Rel(bottom,Ofs(-x,0,z))
        Bounds.__init__(self,name,tlf,tlb,blf,blb,trf,trb,brf,brb)
        self.base = base = Box("%s_base"%name,True,*self.sub(0,0,0,0,.2,0))
        self.faces.append(base)
        self.pot = pot = Column("%s_pot"%name,True,*self.sub(0,0,0,0,0,.8))
        pot.colour = (.3,.3,.2)
        self.faces.append(pot)

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
        inner = Bounds(None,tlf,tlb,blf,blb,trf,trb,brf,brb).sub(.1,.1,.1,.1,0,0)
        self.base = base = Box("base",False,*inner.sub(0,0,0,0,.4,0))
        self.roof = roof = PitchRoof("roof",True,True,*inner.sub(0,0,0,0,.1,.6))
        self.chimney_pos = chimney_pos = Weight(.2)
        self.chimney = chimney = Chimney("chimney",1,2,
            Mix2(inner.tlf,chimney_pos,inner.trb),Mix2(roof.blf,chimney_pos,roof.brb))
        self.faces = [base,roof,chimney]
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

