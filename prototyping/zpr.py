""" OpenGL Zoom Pan Rotate Widget for PyGTK
    subclass or otherwise assign a 'draw' function to an instance of this class
    The draw gets called back each time it should render
    The OpenGL context has been set up so you can just draw with naked gl* func
    Optionally, provide a 'pick' function to get callbacks when the user clicks
    on an object that has been named with glPushNames()

    translation of the excellent GLT ZPR (Zoom Pan Rotate) C code:
    http://www.nigels.com/glt/gltzpr/
    Released under LGPL: http://www.gnu.org/copyleft/lesser.html
    (c) William Edwards 2010
"""

import pygtk; pygtk.require('2.0')
import gtk, gtk.gdk as gdk, gtk.gtkgl as gtkgl, gtk.gdkgl as gdkgl
from OpenGL.GL import *
from OpenGL.GLU import *
from OpenGL.GLUT import *
import math, numpy

class GLZPR(gtkgl.DrawingArea):
    def __init__(self,w=640,h=480):
        try:
            glconfig = gdkgl.Config(mode = (gdkgl.MODE_RGB|gdkgl.MODE_DOUBLE|gdkgl.MODE_DEPTH))
        except gtk.gdkgl.NoMatches:
            glconfig = gdkgl.Config(mode = (gdkgl.MODE_RGB|gdkgl.MODE_DEPTH))
        gtkgl.DrawingArea.__init__(self,glconfig)
        self.set_size_request(w,h)
        self.connect_after("realize",self._init)
        self.connect("configure_event",self._reshape)
        self.connect("expose_event",self._draw)
        self.connect("button_press_event",self._mouseButton)
        self.connect("button_release_event",self._mouseButton)
        self.connect("motion_notify_event",self._mouseMotion)
        self.connect("scroll_event",self._mouseScroll)
        self.connect("key_press_event",self._keyPress)
        self.set_events(self.get_events()|
            gdk.BUTTON_PRESS_MASK|gdk.BUTTON_RELEASE_MASK|
            gdk.POINTER_MOTION_MASK|gdk.POINTER_MOTION_HINT_MASK|
            gdk.KEY_PRESS_MASK)
        self.set_flags(gtk.HAS_FOCUS|gtk.CAN_FOCUS)
        self.grab_focus()
        self._zNear, self._zFar = -10.0, 10.0
        self._zprReferencePoint = [0.,0.,0.,0.]
        self._mouseX = self._mouseY = 0
        self._dragPosX = self._dragPosY = self._dragPosZ = 0.
        self._mouseRotate = self._mouseZoom = self._mousePan = False
        
    class _Context:
        def __init__(self,widget):
            self._widget = widget
            self._count = 0
            self._modelview = self._projection = None
            self._persist = False
        def __enter__(self):
            assert(self._count == 0)
            self.ctx = gtkgl.widget_get_gl_context(self._widget)
            self.surface = gtkgl.widget_get_gl_drawable(self._widget)
            self._begin = self.surface.gl_begin(self.ctx)
            if self._begin:
                self._count += 1
                if self._projection is not None:
                    glMatrixMode(GL_PROJECTION)
                    glLoadMatrixd(self._projection)
                if self._modelview is not None:
                    glMatrixMode(GL_MODELVIEW)
                    glLoadMatrixd(self._modelview)
                return self
            return
        def __exit__(self,exc_type,exc_value,exc_traceback):
            if self._begin:
                self._count -= 1
                if self._persist and (exc_type is None):
                    self._modelview = glGetDoublev(GL_MODELVIEW_MATRIX)
                    self._projection = glGetDoublev(GL_PROJECTION_MATRIX)
                self.surface.gl_end()
            del self.ctx
            del self.surface
            self._persist = False
            if exc_type is not None:
                import traceback
                traceback.print_exception(exc_type,exc_value,exc_traceback)
            return True # suppress
            
    def open_context(self,persist_matrix_changes = False):
        if not hasattr(self,"_context"):
            self._context = self._Context(self)
        assert(self._context._count == 0)
        self._context._persist = persist_matrix_changes
        return self._context
        
    def get_open_context(self):
        if hasattr(self,"_context") and (self._context._count > 0):
            return self._context
        
    def _init(self,widget):
        assert(widget == self)
        try:
            self.init() ### optionally overriden by subclasses
        except Exception as e:
            import traceback
            traceback.print_exc()
            return False
        return True
        
    def reset(self):
        with self.open_context(True):
            glMatrixMode(GL_MODELVIEW)
            glLoadIdentity()
         
    def init(self):
        glLightfv(GL_LIGHT0,GL_AMBIENT, (0.,0.,0.,1.))
        glLightfv(GL_LIGHT0,GL_DIFFUSE, (1.,1.,1.,1.))
        glLightfv(GL_LIGHT0,GL_SPECULAR,(1.,1.,1.,1.))
        glLightfv(GL_LIGHT0,GL_POSITION,(1.,1.,1.,0.))
        glMaterialfv(GL_FRONT,GL_AMBIENT, (.7,.7,.7,1.))
        glMaterialfv(GL_FRONT,GL_DIFFUSE, (.8,.8,.8,1.))
        glMaterialfv(GL_FRONT,GL_SPECULAR,(1.,1.,1.,1.))
        glMaterialfv(GL_FRONT,GL_SHININESS,100.0)
        glEnable(GL_LIGHTING)
        glEnable(GL_LIGHT0)
        glDepthFunc(GL_LEQUAL)
        glEnable(GL_DEPTH_TEST)
        glEnable(GL_NORMALIZE)
        glEnable(GL_COLOR_MATERIAL)
        
    def _reshape(self,widget,event):
        assert(self == widget) 
        with self.open_context(True):
            x, y, width, height = self.get_allocation()
            glViewport(0,0,width,height);
            self._top    =  1.0
            self._bottom = -1.0
            self._left   = -float(width)/float(height)
            self._right  = -self._left
            glMatrixMode(GL_PROJECTION)
            glLoadIdentity()
            glOrtho(self._left,self._right,self._bottom,self._top,self._zNear,self._zFar)
        if hasattr(self,"reshape"):
            self.reshape(event,x,y,width,height) ### optionally implemented by subclasses
        return True
        
    def _mouseMotion(self,widget,event):
        assert(widget==self)
        if event.is_hint:
            x, y, state = event.window.get_pointer()
        else:
            x = event.x
            y = event.y
            state = event.state
        dx = x - self._mouseX
        dy = y - self._mouseY
        if (dx==0 and dy==0): return
        self._mouseX, self._mouseY = x, y
        with self.open_context(True):
            changed = False
            if self._mouseZoom:
                s = math.exp(float(dy)*0.01)
                self._apply(glScalef,s,s,s)
                changed = True
            elif self._mouseRotate:
                ax, ay, az = dy, dx, 0.
                viewport = glGetIntegerv(GL_VIEWPORT)
                angle = math.sqrt(ax**2+ay**2+az**2)/float(viewport[2]+1)*180.0
                inv = numpy.matrix(glGetDoublev(GL_MODELVIEW_MATRIX)).I
                bx = inv[0,0]*ax + inv[1,0]*ay + inv[2,0]*az
                by = inv[0,1]*ax + inv[1,1]*ay + inv[2,1]*az
                bz = inv[0,2]*ax + inv[1,2]*ay + inv[2,2]*az
                self._apply(glRotatef,angle,bx,by,bz)
                changed = True
            elif self._mousePan:
                px, py, pz = self._pos(x,y);
                modelview = glGetDoublev(GL_MODELVIEW_MATRIX)
                glLoadIdentity()
                glTranslatef(px-self._dragPosX,py-self._dragPosY,pz-self._dragPosZ)
                glMultMatrixd(modelview)
                self._dragPosX = px
                self._dragPosY = py
                self._dragPosZ = pz
                changed = True
            if changed:
                self.queue_draw()
            
    def _apply(self,func,*args):
        glTranslatef(*self._zprReferencePoint[0:3])
        func(*args)
        glTranslatef(*map(lambda x:-x,self._zprReferencePoint[0:3]))

    def _mouseScroll(self,widget,event):
        assert(self == widget)       
        s = 4. if (event.direction == gdk.SCROLL_UP) else -4.
        s = math.exp(s*0.01)
        with self.open_context(True):
            self._apply(glScalef,s,s,s)
            self.queue_draw()
            
    def _keyPress(self,widget,event):
        assert(self == widget)
        if hasattr(self,"keyPress"):
            self.keyPress(event)
        
    @classmethod
    def event_masked(cls,event,mask):
        return (event.state & mask) == mask
    
    @classmethod
    def _button_check(cls,event,button,mask):
        # this shouldn't be so crazy complicated
        if event.button == button:
            return (event.type == gdk.BUTTON_PRESS)                    
        return cls.event_masked(event,mask) 
        
    @classmethod
    def get_left_button_down(cls,event):
        return cls._button_check(event,1,gdk.BUTTON1_MASK)
             
    @classmethod
    def get_middle_button_down(cls,event):
        return cls._button_check(event,2,gdk.BUTTON2_MASK)

    @classmethod
    def get_right_button_down(cls,event):
        return cls._button_check(event,3,gdk.BUTTON3_MASK)

    def _mouseButton(self,widget,event):
        left = self.get_left_button_down(event)
        middle = self.get_middle_button_down(event)
        right = self.get_right_button_down(event)
        self._mouseRotate = left and not (middle or right)
        self._mouseZoom = middle or (left and right)
        self._mousePan = right and self.event_masked(event,gdk.CONTROL_MASK)
        x = self._mouseX = event.x
        y = self._mouseY = event.y
        self._dragPosX, self._dragPosY, self._dragPosZ = self._pos(x,y)
        if (left and not self.event_masked(event,gdk.CONTROL_MASK)) and \
            hasattr(self,"pick"):
            with self.open_context():
                nearest, hits = \
                    self._pick(x,self.get_allocation().height-1-y,3,3,event)
                self.pick(event,nearest,hits) # None if nothing hit
        self.queue_draw()
        
    def pick(self,event,nearest,hits):
        print "picked",nearest
        for hit in hits:
            print hit.near, hit.far, hit.names
        
    def _pos(self,x,y):
        """
        Use the ortho projection and viewport information
        to map from mouse co-ordinates back into world
        co-ordinates
        """  
        viewport = glGetIntegerv(GL_VIEWPORT)
        px = float(x-viewport[0])/float(viewport[2])
        py = float(y-viewport[1])/float(viewport[3])       
        px = self._left + px*(self._right-self._left)
        py = self._top  + py*(self._bottom-self._top)
        pz = self._zNear
        return (px,py,pz)
        
    def _pick(self,x,y,dx,dy,event):
        buf = glSelectBuffer(256)
        glRenderMode(GL_SELECT)
        glInitNames()
        glMatrixMode(GL_PROJECTION)
        glPushMatrix() # remember projection matrix
        viewport = glGetIntegerv(GL_VIEWPORT)
        projection = glGetDoublev(GL_PROJECTION_MATRIX)
        glLoadIdentity()
        gluPickMatrix(x,y,dx,dy,viewport)
        glMultMatrixd(projection)        
        glMatrixMode(GL_MODELVIEW)
        glPushMatrix()
        self.draw(event)
        glPopMatrix()
        hits = glRenderMode(GL_RENDER)
        nearest = []
        minZ = None
        for hit in hits:
            if (len(hit.names) > 0) and \
            ((minZ is None) or (hit.near < minZ)):
                minZ = hit.near
                nearest = hit.names
        glMatrixMode(GL_PROJECTION)
        glPopMatrix() # restore projection matrix 
        glMatrixMode(GL_MODELVIEW)
        return (nearest, hits)

    def _draw(self,widget,event):
        assert(self == widget)   
        try:
            with self.open_context() as ctx:
                glMatrixMode(GL_MODELVIEW)
                self.draw(event) ### implemented by subclasses
                if ctx.surface.is_double_buffered():
                    ctx.surface.swap_buffers()
                else:
                    glFlush()
        except Exception as e:
            import traceback
            traceback.print_exc()
            gtk.main_quit()
            return False
        return True

def _demo_draw(event):
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT)
    glScalef(0.25,0.25,0.25)
    glPushMatrix() # No name for grey sphere
    glColor3f(0.3,0.3,0.3)
    glutSolidSphere(0.7, 20, 20)
    glPopMatrix()
    glPushMatrix() 
    glPushName(1) # Red cone is 3
    glColor3f(1,0,0)
    glRotatef(90,0,1,0)
    glutSolidCone(0.6, 4.0, 20, 20)
    glPopName()
    glPopMatrix()
    glPushMatrix() 
    glPushName(2) # Green cone is 2 
    glColor3f(0,1,0)
    glRotatef(-90,1,0,0)
    glutSolidCone(0.6, 4.0, 20, 20)
    glPopName()
    glPopMatrix()
    glPushMatrix() 
    glColor3f(0,0,1) # Blue cone is 3
    glPushName(3)
    glutSolidCone(0.6, 4.0, 20, 20)
    glPopName()
    glPopMatrix()


if __name__ == '__main__':
    import sys
    glutInit(sys.argv)
    gtk.gdk.threads_init()
    window = gtk.Window(gtk.WINDOW_TOPLEVEL)
    window.set_title("Zoom Pan Rotate")
    window.set_size_request(640,480)
    window.connect("destroy",lambda event: gtk.main_quit())
    vbox = gtk.VBox(False, 0)
    window.add(vbox)
    zpr = GLZPR()
    zpr.draw = _demo_draw
    vbox.pack_start(zpr,True,True)
    window.show_all()
    gtk.main()
