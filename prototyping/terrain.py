
import numpy, math, random, sys
from OpenGL.GL import *
from OpenGL.GLU import *
from OpenGL.GLUT import *

def _vec_cross(a,b):
    return ( \
        (a[1]*b[2]-a[2]*b[1]),
        (a[2]*b[0]-a[0]*b[2]),
        (a[0]*b[1]-a[1]*b[0]))
def _vec_ofs(v,ofs):
    return (v[0]-ofs[0],v[1]-ofs[1],v[2]-ofs[2])
def _vec_normalise(v):
    l = math.sqrt(sum(d**2 for d in v))
    if l > 0:
        return (v[0]/l,v[1]/l,v[2]/l)
    return v
    
def _ray_triangle(ray_origin,ray_dir,T):
    # http://softsurfer.com/Archive/algorithm_0105/algorithm_0105.htm#intersect_RayTriangle%28%29
    # get triangle edge vectors and plane normal
    u = T[1]-T[0]
    v = T[2]-T[0]
    n = numpy.cross(u,v) ### cross product
    if n[0]==0 and n[1]==0 and n[2]==0:            # triangle is degenerate
        raise Exception("%s %s %s %s %s %s"%(ray_origin,ray_dir,T,u,v,n))
        return (-1,None)                 # do not deal with this case
    w0 = ray_origin-T[0]
    a = -numpy.dot(n,w0)
    b = numpy.dot(n,ray_dir)
    if math.fabs(b) < 0.00000001:     # ray is parallel to triangle plane
        if (a == 0):              # ray lies in triangle plane
            return (2,None)
        return (0,None)             # ray disjoint from plane
    # get intersect point of ray with triangle plane
    r = a / b
    if (r < 0.0):                   # ray goes away from triangle
        return (0,None)                  # => no intersect
    # for a segment, also test if (r > 1.0) => no intersect

    I = ray_origin + r * ray_dir           # intersect point of ray and plane
    
    # is I inside T?
    uu = numpy.dot(u,u)
    uv = numpy.dot(u,v)
    vv = numpy.dot(v,v)
    w = I - T[0]
    wu = numpy.dot(w,u)
    wv = numpy.dot(w,v)
    D = uv * uv - uu * vv
    # get and test parametric coords
    s = (uv * wv - vv * wu) / D
    if (s < 0.0 or s > 1.0):        # I is outside T
        return (0,None)
    t = (uv * wu - uu * wv) / D
    if (t < 0.0 or (s + t) > 1.0):  # I is outside T
        return (0,None)
    return (1,I)                      # I is in T
    
def _ray_axis_aligned_bounding_box(O,D,C,e):
    parallel = 0
    found = False
    d = C - O
    for i in xrange(3):
        if (math.fabs(D[i]) < .000001):
            parallel |= 1 << i
        else:
            es = e[i] if (D[i] > 0.) else -e[i]
            invDi = 1. / D[i]
            if not found:
                t0 = (d[i] - es) * invDi
                t1 = (d[i] + es) * invDi
                found = True
            else:
                s = (d[i] - es) * invDi
                if (s > t0):
                    t0 = s
                s = (d[i] + es) * invDi
                if (s < t1):
                    t1 = s
                if (t0 > t1):
                    return False    
    if (parallel>0):
        for i in xrange(3):
            if (parallel & (1 << i)):
                if (math.fabs(d[i] - t0 * D[i]) > e[i]) or (math.fabs(d[i] - t1 * D[i]) > e[i]):
                    return False
    return True

def _ray_sphere(ray_origin,ray_dir,sphere_centre,sphere_radius_sqrd): # ray, sphere-centre, radius
    a = sum(_**2 for _ in ray_dir)
    assert a > 0.
    b = 2. * (numpy.dot(ray_dir,ray_origin) - numpy.dot(sphere_centre,ray_dir))
    c = sum(_**2 for _ in (sphere_centre - ray_origin)) - sphere_radius_sqrd
    disc = b * b - 4 * a * c
    return (disc >= 0.)
    
class Bounds:
    _Unbound = [[sys.maxint,sys.maxint,sys.maxint],[-sys.maxint,-sys.maxint,-sys.maxint]]
    def __init__(self,ID):
        self.ID = ID
        self.bounds = numpy.array(Bounds._Unbound,dtype=numpy.float32)
    def reset(self):
        self.bounds.setflags(write=True)
        self.bounds[:] = Bounds._Unbound
    def add(self,pt):
        for i in xrange(3):
            self.bounds[0,i] = min(self.bounds[0,i],pt[i])
            self.bounds[1,i] = max(self.bounds[1,i],pt[i])
    def fix(self):
        self.bounds.setflags(write=False)
        self.sphere_centre = numpy.array([a+(b-a)/2 for a,b in zip(self.bounds[0],self.bounds[1])],dtype=numpy.float32)
        self.sphere_radius_sqrd = sum(((a-b)/2)**2 for a,b in zip(self.bounds[0],self.bounds[1]))        
    def ray_intersects(self,ray_origin,ray_dir):
        return \
            _ray_sphere(ray_origin,ray_dir,self.sphere_centre,self.sphere_radius_sqrd) and \
            _ray_axis_aligned_bounding_box(ray_origin,ray_dir,self.bounds[0],self.bounds[1]-self.bounds[0])
    def intersects_sphere(self,centre,radius):
        diameter_sqrd = (radius*2)**2
        dist = sum((a-b)**2 for a,b in zip(centre,self.sphere_centre))
        return dist < diameter_sqrd

class IcoMesh:

    DIVIDE_THRESHOLD = 3
    
    def __init__(self,terrain,triangle,recursionLevel):
        self.terrain = terrain
        self.ID = len(terrain.meshes)
        self.boundary = numpy.array( \
            [(x,y,z,0) for x,y,z in [terrain.points[t] for t in triangle]],
            dtype=numpy.float32)
        self.bounds = Bounds(self.ID)
        self._projection = numpy.empty((len(triangle),4),dtype=numpy.float32)
        assert recursionLevel <= self.DIVIDE_THRESHOLD
        def num_points(recursionLevel):
            Nc = (15,45,153,561,2145,8385)
            return Nc[recursionLevel-1]
        assert len(triangle) == 3
        self.faces = (triangle,)
        # refine triangles
        for i in xrange(recursionLevel+1):
            faces = numpy.empty((len(self.faces)*4,3),dtype=numpy.int32)
            faces_len = 0
            for tri in self.faces:
                # replace triangle by 4 triangles
                a = terrain._midpoint(tri[0],tri[1])
                b = terrain._midpoint(tri[1],tri[2])
                c = terrain._midpoint(tri[2],tri[0])
                faces[faces_len+0] = (tri[0],a,c)
                faces[faces_len+1] = (tri[1],b,a)
                faces[faces_len+2] = (tri[2],c,b)
                faces[faces_len+3] = (a,b,c)
                faces_len += 4
            assert faces_len == len(faces)
            self.faces = faces
        # make adjacency map
        def add_adjacency_faces(f,p):
            f |= (self.ID << Terrain.FACE_BITS)
            a = terrain.adjacency_faces[p]
            for i in xrange(6):
                if a[i] in (f,-1):
                    a[i] = f
                    return
            assert False, "%s %s"%(a,f)
        def add_adjacency_points(a,b):
            p = terrain.adjacency_points[a]
            for i in xrange(6):
                if p[i] in (b,-1):
                    p[i] = b
                    return
            assert False, "%s %s %s"%(a,p,b)
        for f,(a,b,c) in enumerate(self.faces):
            add_adjacency_faces(f,a)
            add_adjacency_faces(f,b)
            add_adjacency_faces(f,c)
            # assert f == (terrain.find_face(a,b,c) & Terrain.FACE_IDX)
            add_adjacency_points(a,b)
            add_adjacency_points(b,a)
            add_adjacency_points(a,c)
            add_adjacency_points(c,a)
            add_adjacency_points(b,c)
            add_adjacency_points(c,b)
            
    def calculate_bounds(self):
        self.bounds.reset()
        for f in self.faces:
            for f in f:
                self.bounds.add(self.terrain.points[f])
        self.bounds.fix()
            
    def _calc_normals(self):
        # do normals for all faces
        points, normals = self.terrain.points, self.terrain.normals
        for i,f in enumerate(self.faces):
            a = _vec_ofs(points[f[2]],points[f[1]])
            b = _vec_ofs(points[f[0]],points[f[1]])
            pn = _vec_normalise(_vec_cross(a,b))            
            for f in f:
                normals[f] += pn

    def ray_intersection(self,ray_origin,ray_dir):
        T = numpy.empty((3,3),dtype=numpy.float32)
        def test(a,b,c):
            T[:] = a,b,c
            ret,I = _ray_triangle(ray_origin,ray_dir,T)
            if ret == 1:
                return I
        P = self.terrain.points
        for i,(a,b,c) in enumerate(self.faces):
            I = test(P[a],P[b],P[c])
            if I is not None:
                return (self,I,i)
                
    def ray_intersection_with_guess(self,ray_origin,ray_dir,likely):
        T = numpy.empty((3,3),dtype=numpy.float32)
        a,b,c = self.faces[likely]
        P = self.terrain.points
        T[:] = P[a],P[b],P[c]
        ret,I = _ray_triangle(ray_origin,ray_dir,T)
        if ret == 1:
            return (self,I,likely)
        return self.ray_intersection(ray_origin,ray_dir)
        
    def init_gl(self):
        rnd = random.random
        self.indices = self.terrain._vbo(self.faces,GL_ELEMENT_ARRAY_BUFFER)
        self.num_indices = len(self.faces)*3
        
    def draw_gl_ffp(self):
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,self.indices)
        glDrawElements(GL_TRIANGLES,self.num_indices,GL_UNSIGNED_INT,None)
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0)

class Terrain:
    
    LAND_LEVEL = 0.97
    WATER_LEVEL = 0.95
    
    FACE_BITS = 18
    FACE_IDX = (1<<FACE_BITS)-1
    
    WATER = 0
    LAND = 1
    
    def __init__(self):
        self.meshes = []
        self._selection = self._selection_point = None
                
    def create_ico(self,recursionLevel):
        t = (1.0 + math.sqrt(5.0)) / 2.0
        self.midpoints = {}
        self.points = numpy.empty((5 * pow(2,2*recursionLevel+3) + 2,3),dtype=numpy.float32)
        self.points_len = 0
        self.adjacency_faces = numpy.empty((len(self.points),6),dtype=numpy.int32)
        self.adjacency_faces.fill(-1)
        self.adjacency_points = numpy.empty((len(self.points),6),dtype=numpy.int32)
        self.adjacency_points.fill(-1)
        for p in ( \
                (-1, t, 0),( 1, t, 0),(-1,-t, 0),( 1,-t, 0),
                ( 0,-1, t),( 0, 1, t),( 0,-1,-t),( 0, 1,-t),
                ( t, 0,-1),( t, 0, 1),(-t, 0,-1),(-t, 0, 1)):
            self._addpoint(p,True)
        for triangle in ( \
            (0,11,5),(0,5,1),(0,1,7),(0,7,10),(0,10,11),
            (1,5,9),(5,11,4),(11,10,2),(10,7,6),(7,1,8),
            (3,9,4),(3,4,2),(3,2,6),(3,6,8),(3,8,9),
            (4,9,5),(2,4,11),(6,2,10),(8,6,7),(9,8,1)):
            def divide(tri,depth):
                if (recursionLevel-depth) < IcoMesh.DIVIDE_THRESHOLD:
                    self.meshes.append(IcoMesh(self,tri,recursionLevel-depth))
                else:
                    depth += 1
                    a = self._midpoint(tri[0],tri[1])
                    b = self._midpoint(tri[1],tri[2])
                    c = self._midpoint(tri[2],tri[0])
                    divide((tri[0],a,c),depth) 
                    divide((tri[1],b,a),depth)
                    divide((tri[2],c,b),depth)
                    divide((a,b,c),depth)
            divide(triangle,0)
            
        print len(self.meshes),"meshes,",len(self.points),"points",
        print sum(len(mesh.faces) for mesh in self.meshes),"faces",
        print "at",len(self.meshes[0].faces),"each."
        
        assert(self.points_len == len(self.points))

        print "terraforming..."
        self._type = numpy.empty(len(self.points),dtype=numpy.uint8)
        self._type.fill(Terrain.WATER)
        
        for mesh in self.meshes:
            mesh.calculate_bounds()
        
        if False:
            for _ in xrange(50):
                self._spolge(random.randint(0,len(self.points)-1),random.random()*0.5,Terrain.LAND)
            for i,p in enumerate(self.points):
                typ = self._type[i]
                if typ == Terrain.LAND:
                    p *= Terrain.LAND_LEVEL
                elif typ == Terrain.WATER:
                    p *= Terrain.WATER_LEVEL
        else:
            self._gen(500)           
        
        for mesh in self.meshes:
            mesh.calculate_bounds()

        # meshes apply their face normals to our vertices
        self.normals = numpy.zeros((len(self.points),3),dtype=numpy.float32)
        for mesh in self.meshes:
            mesh._calc_normals()
        # average vertex normals
        for i in xrange(len(self.points)):
            f = sum(1 if a != -1 else 0 for a in self.adjacency_points[i])
            assert f >= 2 and f <= 6
            np = self.normals[i] / f
            self.normals[i] = _vec_normalise(np)
            
        # apply colour-scheme
        self.colours = numpy.zeros((len(self.points),3),dtype=numpy.uint8)
        colours = {Terrain.WATER:(0,0,0xff),Terrain.LAND:(0,0xff,0)}
        for i,(t,p) in enumerate(zip(self._type,self.points)):
            if (p[1] > .8) or (p[1] < -.8): # poles
                self.colours[i] = (0xff,0xff,0xff)
            else:
                self.colours[i] = colours[t]
                
    def _gen(self,iterations):
        # http://freespace.virgin.net/hugo.elias/models/m_landsp.htm
        n = numpy.empty(3,dtype=numpy.float32)
        v = numpy.empty(3,dtype=numpy.float32)
        adj = numpy.zeros(len(self.points),dtype=numpy.float32)
        for it in xrange(iterations):
            while True:
                n[0] = (random.random()-.5)*2.
                n[1] = (random.random()-.5)*2.
                n[2] = (random.random()-.5)*2.
                if sum(a**2 for a in n) > 0.:
                    break
            m = -1 if (random.random() > .5) else 1
            for i,p in enumerate(self.points):
                v[:] = p
                v -= n
                if numpy.dot(n,v) > 0:
                    adj[i] += m
                else:
                    adj[i] -= m
        mn, mx = min(adj), max(adj)
        s = mx-mn
        t = (1.-Terrain.WATER_LEVEL)*2
        for i,a in enumerate(adj):
            tmp = 1. - (((a-mn)/s) * t)
            adj[i] = tmp
        for i,(a,p) in enumerate(zip(adj,self.points)):
            if a > Terrain.WATER_LEVEL:
                self._type[i] = Terrain.LAND
                p *= a
            else:
                p *= Terrain.WATER_LEVEL
            
    def _spolge(self,centre,radius,typ):
        centre = self.points[centre]
        radius_sqrd = radius**2
        for mesh in self.meshes:
            if not mesh.bounds.intersects_sphere(centre,radius): continue
            for face in mesh.faces:
                for p in face:
                    if self._type[p] == typ: continue
                    dist_sqrd = sum((a-b)**2 for a,b in zip(self.points[p],centre))
                    if dist_sqrd < radius_sqrd:
                        self._type[p] = typ
            
    def pick(self,x,y):
        glScale(.8,.8,.8)
        modelview = numpy.matrix(glGetDoublev(GL_MODELVIEW_MATRIX))
        projection = numpy.matrix(glGetDoublev(GL_PROJECTION_MATRIX))
        viewport = glGetIntegerv(GL_VIEWPORT)
        old_pt = self._selection_point
        self._selection = self._selection_point = None
        R = numpy.array([gluUnProject(x,y,10,modelview,projection,viewport),
            gluUnProject(x,y,-10,modelview,projection,viewport)],
            dtype=numpy.float32)
        ray_origin, ray_dir = R[0], R[1]-R[0]
        return self._ray_intersection(ray_origin,ray_dir)
        
    def _ray_intersection(self,ray_origin,ray_dir):
        candidates = []
        for mesh in self.meshes:
            if mesh.bounds.ray_intersects(ray_origin,ray_dir):
                candidates.append(( \
                    -sum((a-b)**2 for a,b in zip(mesh.bounds.sphere_centre,ray_origin)), # distance from ray
                    mesh))
        candidates.sort() # sort by distance, nearest first
        for _,mesh in candidates:
            I = mesh.ray_intersection(ray_origin,ray_dir)
            if I is not None: return I
        
    def find_face_for_point(self,p):
        return self._ray_intersection(numpy.zeros(3,dtype=numpy.float32),
            numpy.array(_vec_normalise(p),dtype=numpy.float32))
        
    def find_face_for_point_with_guess(self,p,mesh,likely):
        ray_origin = numpy.zeros(3,dtype=numpy.float32)
        ray_dir = numpy.array(_vec_normalise(p),dtype=numpy.float32)
        I = mesh.ray_intersection_with_guess(ray_origin,ray_dir,likely)
        if I is not None:
            return I
        return self._ray_intersection(ray_origin,ray_dir)

    def route(self,start,end):
        # use A*
        I = self.find_face_for_point(start)
        if I is None:
            print "Bad route: no start face",start,end
            return
        start_face = (I[0].ID << Terrain.FACE_BITS)|I[2]
        I = self.find_face_for_point(end)
        if I is None:
            print "Bad route: no end face",start,start_face,end
            return
        end_face = (I[0].ID << Terrain.FACE_BITS)|I[2]
        closedset = set()
        openset = set()
        openset.add(start_face)
            
    def _vbo(self,array,target):
        handle = glGenBuffers(1)
        assert handle > 0
        glBindBuffer(target,handle)
        glBufferData(target,array,GL_STATIC_DRAW)
        glBindBuffer(target,0)
        return handle

    def _addpoint(self,point,normalise):
        slot = self.points_len
        self.points_len += 1
        if normalise:
            dist = math.sqrt(sum(_**2 for _ in point))
            for i in xrange(3): self.points[slot,i] = point[i]/dist
        else:
            self.points[slot] = point
        return slot
        
    def _midpoint_key(self,a,b):
        return (min(a,b) << 32) + max(a,b)

    def _midpoint(self,a,b):
        key = self._midpoint_key(a,b)
        if key not in self.midpoints:
            a_pt = self.points[a]
            b_pt = self.points[b]
            mid = tuple((p1+p2)/2. for p1,p2 in zip(a_pt,b_pt))
            self.midpoints[key] = self._addpoint(mid,True)
        return self.midpoints[key]
        
    def find_face(self,a,b,c,insist=True):
        faces = self.adjacency_faces
        extra = self._adjacency_faces_extra
        face = set(extra[a] if a in extra else faces[a])
        face &= set(extra[b] if b in extra else faces[b])
        face &= set(extra[c] if c in extra else faces[c])
        if len(face) == 0:
            assert not insist,"Couldn't find face %s %s %s"%(a,b,c)
            return
        f = face.pop()
        if f == -1:
            f = face.pop()
        assert len(face) == 0
        return f
        
    def find_other_common_point(self,a,b,c,insist=True):
        points = self.adjacency_points
        point = set(points[a])
        point &= set(points[b])
        if len(point) == 0:
            assert not insist
            return
        while True:
            p = point.pop()
            if p in (-1,c): continue
            return p
                
    def init_gl(self):
        for mesh in self.meshes:
            mesh.init_gl()
        self._vbo_vertices = self._vbo(self.points,GL_ARRAY_BUFFER)
        self._vbo_normals = self._vbo(self.normals,GL_ARRAY_BUFFER)
        self._vbo_colours = self._vbo(self.colours,GL_ARRAY_BUFFER)
            
    def draw_gl_ffp(self,event):
        glClearColor(1,1,1,1)
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT)
        glScale(.8,.8,.8)
        glEnable(GL_CULL_FACE)
        glEnableClientState(GL_VERTEX_ARRAY)
        glBindBuffer(GL_ARRAY_BUFFER,self._vbo_vertices)
        glVertexPointer(3,GL_FLOAT,0,None)
        glEnableClientState(GL_NORMAL_ARRAY)
        glBindBuffer(GL_ARRAY_BUFFER,self._vbo_normals)
        glNormalPointer(GL_FLOAT,0,None)
        glEnableClientState(GL_COLOR_ARRAY)
        glBindBuffer(GL_ARRAY_BUFFER,self._vbo_colours)
        glColorPointer(3,GL_UNSIGNED_BYTE,0,None)
        glBindBuffer(GL_ARRAY_BUFFER,0)
        modelview = numpy.matrix(glGetDoublev(GL_MODELVIEW_MATRIX))
        for mesh in self.meshes:
            if mesh == self._selection:
                glDisableClientState(GL_COLOR_ARRAY)
                glColor(1,0,0,1)
            mesh.draw_gl_ffp()
            if mesh == self._selection:
                glEnableClientState(GL_COLOR_ARRAY)
        glDisableClientState(GL_COLOR_ARRAY)
        glDisableClientState(GL_VERTEX_ARRAY)
        glDisableClientState(GL_NORMAL_ARRAY)
        if self._selection_point is not None:
            glColor(0,0,1,1)
            glTranslate(*self._selection_point)
            glutSolidSphere(0.03,20,20)
        glDisable(GL_CULL_FACE)

