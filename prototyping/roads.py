#!/usr/bin/env python

import pygtk; pygtk.require('2.0')
import gtk, gtk.gdk as gdk, gtk.gtkgl as gtkgl, gtk.gdkgl as gdkgl, gobject
from OpenGL.GL import *
from OpenGL.GLU import *
from OpenGL.GLUT import *
from random import random
import numpy, math

import zpr

def load_texture(filename):
    texture = glGenTextures(1)
    try:
        import Image
        image = Image.open(filename)
        w, h = image.size
        try:
            img = image.tostring("raw","RGBA",0,-1)
            mode = GL_RGBA
        except Exception as e:
            img = image.tostring("raw","RGB",0,-1)
            mode = GL_RGB
        image = img
        glPixelStorei(GL_UNPACK_ALIGNMENT,1)
        glBindTexture(GL_TEXTURE_2D,texture)
        glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP)
        glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP)
        glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR)
        glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR)
        glTexImage2D(GL_TEXTURE_2D,0,mode,w,h,0,mode,GL_UNSIGNED_BYTE,image)
        return (texture,w,h)
    except Exception,e:
        print "Could not load texture",filename
        print e
        return (0,1024,1024)

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
    
def HermiteSpline(Y,SEG=10): # Y is a path of points
    if len(Y) < 2: return
    tension, bias = 0.1, 0.
    vertices = []
    for y in xrange(len(Y)-1):
        y0 = Y[y-1] if (y > 0) else Y[0]
        y1 = Y[y]
        y2 = Y[y+1]
        y3 = Y[y+2] if (y < len(Y)-2) else Y[-1]
        for mu in xrange(SEG+1):
            mu *= 1./SEG
            vertices.append((
                HermiteInterpolate(y0[0],y1[0],y2[0],y3[0],mu,tension,bias),
                HermiteInterpolate(y0[1],y1[1],y2[1],y3[1],mu,tension,bias),
                HermiteInterpolate(y0[2],y1[2],y2[2],y3[2],mu,tension,bias)))
    if len(vertices)<2:
        return
    glBegin(GL_LINE_STRIP)
    for i,vertex in enumerate(vertices):
        if i&1 == 1:
            glColor(0,0,1,1)
        else:
            glColor(0,1,1,1)
        glVertex(*vertex)
    glEnd()


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
    def __sub__(self,rhs):
        if isinstance(rhs,Point):
            return Point(self.x-rhs.x,self.y-rhs.y)
        return Point(self.x-rhs,self.y-rhs)
    def __add__(self,rhs):
        if isinstance(rhs,Point):
            return Point(self.x+rhs.x,self.y+rhs.y)
        return Point(self.x+rhs,self.y+rhs)
    def __mul__(self,rhs):
        if isinstance(rhs,Point):
            return Point(self.x*rhs.x,self.y*rhs.y)
        return Point(self.x*rhs,self.y*rhs)
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
    def __repr__(self):
        return "<%f,%f>"%(self.x,self.y)
    def __getitem__(self,i):
        if i==0: return self.x
        if i==1: return self.y
        if i==2: return 0.
        raise Exception("index %s out of bounds"%i)
        
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
    def __div__(self,rhs):
        x = self.dx / rhs
        y = self.dy / rhs
        return Line(self.a,self.a+Point(x,y))
    def __mul__(self,rhs):
        x = self.dx * rhs
        y = self.dy * rhs
        return Line(self.a,self.a+Point(x,y))
    def normal(self):
        return self / self.length()
    def side(self,pt):
        """0 = on, <0 = right, >0 = left"""
        return (pt.y-self.a.y)*self.dx-(pt.x-self.a.x)*self.dy
    def draw(self,*rgb):
        glColor(*rgb)
        glBegin(GL_LINES)
        glVertex(*self.a.to_3d())
        glVertex(*self.b.to_3d())
        glEnd()
        
def feq(a,b):
    return abs(a-b) < 0.000001
    
class Circle:
    num_segments = 4*4
    theta = 2.0 * 3.1415926 / float(num_segments)
    c = math.cos(theta)
    s = math.sin(theta)
    def __init__(self,pt,radius):
        self.pt = pt
        self.radius = radius
    def interpolate(self,line,U=.5):
        if (line.a.distance_sqrd(self.pt) != self.radius**2) or \
            (line.b.distance_sqrd(self.pt) != self.radius**2):
            line = Line(self.snap(line.a),self.snap(line.b))
        return self.snap(line.interpolate(U))
    def snap(self,pt):
        return (Line(self.pt,pt).normal() * self.radius).b
    def draw(self,*rgb):
        glColor(*rgb)
        x, y = self.radius, 0
        glBegin(GL_LINE_LOOP)
        for i in xrange(self.num_segments):
            glVertex(x + self.pt.x, y + self.pt.y) 
            t = x
            x = self.c * x - self.s * y
            y = self.s * t + self.c * y
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
        
class Path(list):
    def __init__(self,inner,width,curve):
        list.__init__(self)
        self.path = self
        self.INNER = inner
        self.OUTER = inner+width
        self.WIDTH = width
        self.CURVE = curve
        self.edges = None
    def dirty(self):
        self.edges = None # force recompute
    def get_edges(self):
        if self.edges is not None: return self.edges
        self.edges = []
        if len(self.path) < 2: return self.edges
        pivots = []
        # compute pivots
        LEFT, ON, RIGHT = "L", "-", "R"
        OUTER,INNER = self.OUTER,self.INNER # tidier naming
        pivot = self.path[0]
        from_side, from_inner, from_outer = ON,pivot.circle((OUTER-INNER)/2.),None
        from_left = from_right = None
        tile = 0.
        tiles = 1
        for p in xrange(1,len(self.path)):
            prev = self.path[p-1]
            pt = self.path[p]
            # which side to go on this new point
            if p == len(self.path)-1: #last one?
                pivot = self.path[-1]
                to_side, to_inner, to_outer = ON,pivot.circle((OUTER-INNER)/2.),None
            else:
                next = self.path[p+1]
                pivot = Line(pt,Line(pt.line(prev).normal().b,pt.line(next).normal().b).interpolate(.5))
                pivot = pivot.interpolate(((INNER+OUTER)/2.) / pivot.length())
                side = Line(prev,pt).side(pivot)
                side = LEFT if side > 0. else ON if side == 0. else RIGHT
                inner = pivot.circle(INNER)
                outer = pivot.circle(OUTER) if side is not ON else None
                to_side, to_inner, to_outer = side,inner,outer
            # compute where to go from and to
            if (from_side == LEFT) and (to_side == LEFT):
                left = (INNER,OUTER,INNER,1)
                right = (OUTER,OUTER,OUTER,1)
            elif (from_side == RIGHT) and (to_side == RIGHT):
                left = (OUTER,OUTER,OUTER,0)
                right = (INNER,OUTER,INNER,0)
            elif (from_side == LEFT) and (to_side == RIGHT):
                left = (INNER,INNER,OUTER,1)
                right = (OUTER,INNER,INNER,1)
            elif (from_side == RIGHT) and (to_side == LEFT):
                left = (OUTER,INNER,INNER,0)
                right = (INNER,INNER,OUTER,0)
            elif (from_side == ON) and (to_side == LEFT):
                left = (INNER,INNER,INNER,0)
                right = (INNER,OUTER,OUTER,1)
            elif (from_side == ON) and (to_side == RIGHT):
                left = (INNER,OUTER,OUTER,0)
                right = (INNER,INNER,INNER,1)
            elif (from_side == LEFT) and (to_side == ON):
                left = (INNER,INNER,INNER,1)
                right = (OUTER,OUTER,INNER,1)
            elif (from_side == RIGHT) and (to_side == ON):
                left = (OUTER,OUTER,INNER,0)
                right = (INNER,INNER,INNER,0)
            elif (from_side == ON) and (to_side == ON):
                left = (INNER,OUTER,INNER,0)
                right = (INNER,OUTER,INNER,1)
            else:
                assert False
            # decode left
            left_fr, tan, to, idx = left
            left_fr = from_inner if left_fr == INNER else from_outer
            tan = left_fr.inner_tangents if tan == INNER else left_fr.outer_tangents
            to = to_inner if to == INNER else to_outer
            left = tan(to)[idx]
            # decode right
            right_fr, tan, to, idx = right
            right_fr = from_inner if right_fr == INNER else from_outer
            tan = right_fr.inner_tangents if tan == INNER else right_fr.outer_tangents
            to = to_inner if to == INNER else to_outer
            right = tan(to)[idx]
            # turn it into edges
            if (from_left is not None) and (from_side != ON):
                l = Line(from_left,left.a)
                r = Line(from_right,right.a)
                def curve(l,r,part):
                    length = max(l.length(),r.length())
                    if (length > self.CURVE):
                        lmid = left_fr.interpolate(l)
                        rmid = right_fr.interpolate(r)
                        curve(l.a.line(lmid),r.a.line(rmid),False)
                        curve(lmid.line(l.b),rmid.line(r.b),True)
                    if not part:
                        self.edges.append((l.b,r.b))
                mid = from_inner.pt+((from_inner.snap(prev)-from_inner.pt)*4)
                lmid = left_fr.snap(mid)
                rmid = right_fr.snap(mid)
                curve(l.a.line(lmid),r.a.line(rmid),False)
                curve(lmid.line(l.b),rmid.line(r.b),True)
            self.edges.append((left.a,right.a))
            self.edges.append((left.b,right.b))
            # for next
            from_side, from_inner, from_outer = to_side, to_inner, to_outer
            from_left, from_right = left.b, right.b
        return self.edges

class Road:
    def __init__(self,path):
        self.path = path
    def init(self):
        self.texture, self.texture_w, self.texture_h = \
            load_texture("../data/egypt_stone.png")
    def draw(self):
        edges = self.path.get_edges()
        if len(edges) > 0: 
            quads = []
            # tessellate with edges
            tiles = 0.
            TILE_SIZE = self.path.WIDTH # square
            left_prev, right_prev = edges[0]
            for left, right in edges[1:]:
                left = left_prev.line(left)
                right = right_prev.line(right)
                if left.length() > right.length():
                    left_scale = 1.
                    right_scale = right.length() / left.length()
                    length = left.length()
                elif (left.length() == 0.):
                    continue
                else:
                    left_scale = left.length() / right.length()
                    right_scale = 1.
                    length = right.length()
                l = 0
                while l < length:
                    remaining = min(TILE_SIZE-tiles,length-l)
                    tx1 = tiles/TILE_SIZE
                    tx2 = (tiles+remaining)/TILE_SIZE
                    quads.append(( \
                        left.interpolate((l*left_scale)/left.length()),
                        right.interpolate((l*right_scale)/right.length()),
                        (tx1,0.),(tx1,1.),
                        left.interpolate(((l+remaining)*left_scale)/left.length()),
                        right.interpolate(((l+remaining)*right_scale)/right.length()),
                        (tx2,0.),(tx2,1.)))
                    tiles += remaining
                    if tiles >= TILE_SIZE:
                        tiles = 0
                    l += remaining
                left_prev, right_prev = left.b, right.b
            # draw it
            glEnable(GL_TEXTURE_2D)
            glBindTexture(GL_TEXTURE_2D,self.texture)
            glColor(1,1,1,1)
            glBegin(GL_QUADS)
            for i,(a,d,ta,td,b,c,tb,tc) in enumerate(quads):
                glTexCoord(*ta)
                glVertex(*a.to_3d())
                glTexCoord(*tb)
                glVertex(*b.to_3d())
                glTexCoord(*tc)
                glVertex(*c.to_3d())
                glTexCoord(*td)
                glVertex(*d.to_3d())
            glEnd()
            glBindTexture(GL_TEXTURE_2D,0)
        for pt in self.path:
            pt.circle(self.path.WIDTH/3.).draw(1,.3,.3)
        HermiteSpline(self.path)

class RoadMaker(zpr.GLZPR):
    MARK = .01    
    def __init__(self):
        zpr.GLZPR.__init__(self)
        self.ground = Ground()
        self.path = Path(.03,.06,.02)
        self.render = Road(self.path)
    def init(self):
        zpr.GLZPR.init(self)
        try:
            from OpenGL.GL.ARB.multisample import GL_MULTISAMPLE_ARB
            glEnable(GL_MULTISAMPLE_ARB)
        except Exception as e:
            print "Error initializing multisampling:",e
        self.render.init()
        glDisable(GL_DEPTH_TEST)
    def draw(self,event):
        glClearColor(1,1,1,0)
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT)
        glScale(.8,.8,.8)        
        self.ground.draw()
        glLineWidth(2)
        self.render.draw()

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
                if d <= self.path.OUTER:
                    if selected is not None:
                        print "ARGH"
                        error = True
                        break
                    else:
                        selected = i
                elif d <= (self.path.OUTER*2):
                    print "OGH"
                    error = True
                    break
            if error: break
            if selected:
                self.path[selected] = point
            else:
                self.path.append(point)
            self.path.dirty()
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
