#!/usr/bin/env python

import pygtk; pygtk.require('2.0')
import gtk, gtk.gdk as gdk, gtk.gtkgl as gtkgl, gtk.gdkgl as gdkgl, gobject
from OpenGL.GL import *
from OpenGL.GLU import *
from OpenGL.GLUT import *
import random, numpy, math, itertools

from gameobjects.matrix44 import Matrix44 as Matrix

import zpr
from roads import load_texture

uv_a, uv_b = (0.,1.), (1.,1.)
SELECTED = None

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

def lerp(p1,weight,p2):
    return p1 + (p2-p1) * weight
    
class Rnd:
    def __init__(self,low,high):
        self.low,self.high = low,high
    def __call__(self):
        return random.uniform(self.low,self.high)
        
class Roulette:
    def __init__(self,*choices):
        self.weight = 0.
        self.choices = []
        for weight,choice in choices:
            self.weight += weight
            self.choices.append((weight,choice))
    def __call__(self):
        throw = random.uniform(0.,self.weight)
        for weight,choice in self.choices:
            throw -= weight
            if throw <= 0:
                return choice
        raise Exception("bad roulette!")
        return choice[0]

class Face:
    def __init__(self,matrix,name,*mix):
        self.matrix = matrix.copy()
        self.name = name
        assert len(mix) > 2
        self.m = mix
        if len(mix) == 3:
            self.tl, self.bl, self.br = self.m
        elif len(mix) == 4:
            self.tl, self.bl, self.br, self.tr = self.m
        self.colour = None
        self.texture = 0
        self.texture_coords = [(0,0)]*len(mix)
    def normal(self):
        m0 = self.m[0]
        u = self.m[1]-self.m[0]
        v = self.m[2]-self.m[0]
        return u.cross(v).normal()
    def set_texture(self,texture,a_ofs,a,b):
        self.texture = texture
        self.colour = (1,1,1)
        self.texture_coords = tx = []
        A, B = self.m[a_ofs:a_ofs+2]
        for P in self.m:
            if P == A:
                tx.append(a)
            elif P == B:
                tx.append(b)
            else:
                scale = P.distance(A)/B.distance(A)
                theta = (P-A).dot((B-A)/(P.distance(A)*B.distance(A)))
                theta = math.acos(theta)
                x, y = a[0]-b[0], a[1]-b[1]
                x, y = x*math.cos(theta) - y*math.sin(theta), \
                    x*math.sin(theta) + y*math.cos(theta)
                x, y = a[0]+ x*scale, a[1]+ y*scale
                tx.append((x,y))
    def intersection(self,ray_origin,ray_dir):
        v = [Point(*self.matrix.transform(v)).numpy() for v in self.m]
        I = zpr.ray_triangle(ray_origin,ray_dir,(v[0],v[1],v[2]))
        if (I is None) and (len(v) > 3):
            I = zpr.ray_triangle(ray_origin,ray_dir,(v[2],v[3],v[0]))
        if I is None: return ()
        return ((Point(*I),self),)
    def draw(self,guide,outline):
        t = self.matrix.transform
        if guide: colour = (.4,.4,1.,.2) if outline else (.8,.8,1.,.2)
        else: colour = self.colour if self.colour is not None else (1,0,0)
        glBindTexture(GL_TEXTURE_2D,self.texture)
        glColor(*colour)
        glNormal(*t(self.normal()))
        glBegin(GL_LINE_LOOP if outline else GL_POLYGON)
        for tx,pt in zip(self.texture_coords,self.m):
            glTexCoord(*tx)
            glVertex(*t(pt.xyz()))
        glEnd()
        glBindTexture(GL_TEXTURE_2D,0)
        if self == SELECTED:
            glPushMatrix()
            glTranslate(*self.m[1])
            glColor(1,0,0)
            glutSolidSphere(.02,20,20)
            glPopMatrix()
            glPushMatrix()
            glTranslate(*self.m[2])
            glColor(0,1,0)
            glutSolidSphere(.02,20,20)
            glPopMatrix()
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
    def scale(self,left,right,front,back,top,bottom):
        x,y,z = [abs(d) for d in self.brb-self.tlf]
        left *= x; right *= -x
        front *= -z; back *= z
        top *= -y; bottom *= y
        return Bounds(None,
            self.tlf+Point(left,top,front),self.tlb+Point(left,top,back),
            self.blf+Point(left,bottom,front),self.blb+Point(left,bottom,back),
            self.trf+Point(right,top,front),self.trb+Point(right,top,back),
            self.brf+Point(right,bottom,front),self.brb+Point(right,bottom,back))
    def resize(self,**kwargs):
        b = Bounds(None,*self)
        if "width" in kwargs:
            p = Point(kwargs["width"],0,0)
            b.trf, b.trb, b.brf, b.brb = b.tlf+p, b.tlb+p, b.blf+p, b.blb+p
        if "height" in kwargs:
            p = Point(0,kwargs["height"],0)
            b.tlf, b.tlb, b.trf, b.trb = b.blf+p, b.blb+p, b.brf+p, b.brb+p
        if "depth" in kwargs:
            p = Point(0,0,-kwargs["depth"])
            b.tlb, b.blb, b.trb, b.brb = b.tlf+p, b.blf+p, b.trf+p, b.brf+p
        return b
    def __getitem__(self,i):
        return (self.tlf,self.tlb,self.blf,self.blb,self.trf,self.trb,self.brf,self.brb)[i]

class Box(Bounds):
    """four sided box, optionally with a closed top"""
    def __init__(self,matrix,name,top,tlf,tlb,blf,blb,trf,trb,brf,brb):
        Bounds.__init__(self,name,tlf,tlb,blf,blb,trf,trb,brf,brb)
        self.front = front = Face(matrix,"%s_front"%name,tlf,blf,brf,trf)
        self.back = back = Face(matrix,"%s_back"%name,trb,brb,blb,tlb)
        self.left = left = Face(matrix,"%s_left"%name,tlb,blb,blf,tlf)
        self.right = right = Face(matrix,"%s_right"%name,trf,brf,brb,trb)
        self.faces.extend((front,back,left,right))
        if top:
            self.top = top = Face(matrix,"%s_top"%name,trf,trb,tlb,tlf)
            self.faces.append(top)
            
class Column(Bounds):
    MAX = 0.03
    def __init__(self,matrix,name,top,tlf,tlb,blf,blb,trf,trb,brf,brb):
        Bounds.__init__(self,name,tlf,tlb,blf,blb,trf,trb,brf,brb)
        self.matrix = matrix.copy()
        self.top = top
        self.colour = None
    def draw(self,guide,outline):
        # compute the xz
        centre = (self.tlf+self.trb)/2.
        top = centre.y
        bottom = ((self.blf+self.brb)/2.).y
        r = min(*(centre-self.tlf))
        num_segments = 10
        theta = 2. * 3.1415926 / float(num_segments)
        c = math.cos(theta)
        s = math.sin(theta)
        x, z = r, 0
        xz = [(x+centre.x,z+centre.z)]
        for i in xrange(num_segments):
            t = x
            x, z = c * x - s * z, s * x + c * z
            xz.append((x+centre.x,z+centre.z))
        xz.reverse()
        # draw it
        t = self.matrix.transform
        glColor(*self.colour if self.colour is not None else (0,1,0))
        glBegin(GL_QUAD_STRIP)
        for x,z in xz:
            p = Point(x,top,z)
            glNormal(*t(-(p-centre).normal()))
            glVertex(*t(p))
            p = Point(x,bottom,z)
            glNormal(*t(-(p-centre).normal()))
            glVertex(*t(p))
        glEnd()
        glNormal(0,1,0)
        if self.top:
            glBegin(GL_POLYGON)
            for x,z in xz:
                glVertex(*t((x,top,z)))
            glEnd()
            
ROOF_TEXTURES = Roulette((1,"textures/gray_pebble_tiles.jpg"),(1,"textures/fancy_tiles.jpg"))
        
class PitchRoof(Bounds):
    @classmethod
    def DEPTH(cls,floor_height,num_floors):
        return random.randint(2,3)
    def __init__(self,matrix,name,house,left,right,blf,blb,brf,brb,pitch=2,hip=False):
        self.house = house
        self.height = height = abs(blf.z-brb.z)/pitch
        top = Point(0,height,0)
        tlf,tlb,trf,trb = blf+top,blb+top,brf+top,brb+top
        Bounds.__init__(self,name,tlf,tlb,blf,blb,trf,trb,brf,brb)
        l = Point(height,0,0) if (left and hip) else 0
        r = Point(height,0,0) if (right and hip) else 0
        self.front = front = Face(matrix,"%s_front"%name,lerp(tlf,.5,tlb)+l,blf,brf,lerp(trf,.5,trb)-r)
        self.back = back = Face(matrix,"%s_back"%name,front.tr,brb,blb,front.tl)
        self.texture = texture = load_texture(ROOF_TEXTURES())[0]
        front.set_texture(texture,1,uv_a,uv_b)
        back.set_texture(texture,1,uv_a,uv_b)
        self.faces.extend((front,back))
        if left:
            self.left = left = Face(matrix,"%s_left"%name,front.tl,back.br,front.bl)
            if hip: left.set_texture(texture,1,uv_a,uv_b)
            self.faces.append(left)
        if right:
            self.right = right = Face(matrix,"%s_right"%name,front.tr,front.br,back.bl)
            if hip: right.set_texture(texture,1,uv_a,uv_b)
            self.faces.append(right)

class HipRoof(PitchRoof):
    def __init__(self,matrix,name,house,left,right,blf,blb,brf,brb,pitch=2):
       PitchRoof.__init__(self,matrix,name,house,left,right,blf,blb,brf,brb,pitch,True)

class GambrelRoof(Bounds):
    @classmethod
    def DEPTH(cls,floor_height,num_floors):
        return 3 if num_floors > 2 else 2
    TOP = Roulette((1,PitchRoof),(1,HipRoof))
    def __init__(self,matrix,name,house,left,right,blf,blb,brf,brb,mansard=False):
        self.house = house
        self.height = height = house.floor_height
        top = Point(0,height,0)
        tlf,tlb,trf,trb = blf+top,blb+top,brf+top,brb+top
        Bounds.__init__(self,name,tlf,tlb,blf,blb,trf,trb,brf,brb)
        slope = height/2.5
        l = Point(slope if (left and mansard) else 0,0,0)
        r = Point(slope if (right and mansard) else 0,0,0)
        self.front = front = Face(matrix,"%s_front"%name,tlf+Point(0,0,-slope)+l,blf,brf,trf+Point(0,0,-slope)-r)
        self.back = back = Face(matrix,"%s_back"%name,trb+Point(0,0,slope)-l,brb,blb,tlb+Point(0,0,slope)+r)
        self.top = top = self.TOP()(matrix,"%s_top"%name,house,left,right,front.tl,back.tr,front.tr,back.tl,4)
        self.texture = texture = top.texture
        front.set_texture(texture,1,uv_a,uv_b)
        back.set_texture(texture,1,uv_a,uv_b)
        self.faces.extend((front,back,top))
        if left:
            self.left = left = Face(matrix,"%s_left"%name,front.tl,back.tr,back.br,front.bl)
            if mansard: left.set_texture(texture,1,uv_a,uv_b)
            self.faces.append(left)
        if right:
            self.right = right = Face(matrix,"%s_right"%name,front.tr,front.br,back.bl,back.tl)
            if mansard: right.set_texture(texture,1,uv_a,uv_b)
            self.faces.append(right)

class MansardRoof(GambrelRoof):
    def __init__(self,matrix,name,house,left,right,blf,blb,brf,brb):
        GambrelRoof.__init__(self,matrix,name,house,left,right,blf,blb,brf,brb,True)

class Chimney(Bounds):
    def __init__(self,matrix,name,w,d,top,bottom):
        self.w, self.d, self.top, self.bottom = w,d,top,bottom
        x,z = (w*.1)/2,(d*.1)/2
        tlf = top+Point(x,0,-z)
        tlb = top+Point(x,0,z)
        blf = bottom+Point(x,0,-z)
        blb = bottom+Point(x,0,z)
        trf = top+Point(-x,0,-z)
        trb = top+Point(-x,0,z)
        brf = bottom+Point(-x,0,-z)
        brb = bottom+Point(-x,0,z)
        Bounds.__init__(self,name,tlf,tlb,blf,blb,trf,trb,brf,brb)
        self.base = base = Box(matrix,"%s_base"%name,True,*self.scale(0,0,0,0,.2,0))
        self.faces.append(base)
        self.pot = pot = Column(matrix,"%s_pot"%name,True,*self.scale(0,0,0,0,0,.8))
        pot.colour = (.3,.3,.2)
        self.faces.append(pot)

class House(Faces):
    FLOOR_HEIGHT = Rnd(.2,.3)
    ROOF_TYPE = Roulette((1,GambrelRoof),(.4,PitchRoof),(1,MansardRoof))
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
        self.matrix = matrix = Matrix()
        # guides are inverted so faces face inwards
        self.guides = ( \
            Face(matrix,"guide_front",tlf,trf,brf,blf), #,(0,0,-1)), # front
            Face(matrix,"guide_right",trf,trb,brb,brf), #,(-1,0,0)), # right
            Face(matrix,"guide_left",tlb,tlf,blf,blb), #,(1,0,0)), # left
            Face(matrix,"guide_top",trb,trf,tlf,tlb), #,(0,-1,0)), # top
            Face(matrix,"guide_bottom",blb,blf,brf,brb), #,(0,1,0)), # bottom
            Face(matrix,"guide_back",trb,tlb,blb,brb)) #,(0,0,1))) # back
        #matrix.make_x_rotation(.5)
        height = .6
        self.floor_height = floor_height = self.FLOOR_HEIGHT()
        self.num_floors = num_floors = random.randint(1,int(math.floor(height/floor_height)))
        self.height = height = floor_height*num_floors
        roof = self.ROOF_TYPE()
        depth = roof.DEPTH(floor_height,num_floors)
        self.sz = sz = (depth*2,num_floors,depth)
        unit_width = floor_height
        inner = Bounds(None,tlf,tlb,blf,blb,trf,trb,brf,brb). \
            resize(width=sz[0]*unit_width,height=sz[1]*floor_height,depth=sz[2]*unit_width)
        self.base = base = Box(matrix,"base",False,*inner)
        self.roof = roof = roof(matrix,"roof",self,True,True,base.tlf,base.tlb,base.trf,base.trb)
        self.chimney_pos = chimney_pos = .2
        chimney = roof.resize(height=roof.tlf.distance(roof.blf)*1.2)
        self.chimney = chimney = Chimney(matrix,"chimney",1,2,
            lerp(chimney.tlf,chimney_pos,chimney.trb),lerp(chimney.blf,chimney_pos,chimney.brb))
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
    def init(self):
        zpr.GLZPR.init(self)
        glEnable(GL_BLEND)
        glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA)
        glEnable(GL_TEXTURE_2D)
        glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT)
        glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_REPEAT)
        glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR)
        glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR)
        self.model = House(self)
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
        elif key == " ":
            self.model = House(self)
            self.queue_draw()
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
        global SELECTED
        SELECTED = None
        for dist,typ,face,pt in hits:
            if typ == House.FACE:
                SELECTED = face
                self.queue_draw()
                break
        return ((),())
    def pick(self,*args):
        pass

if __name__ == '__main__':
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

