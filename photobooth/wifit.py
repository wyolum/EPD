import tempfile
import json
import StringIO
import urllib2
import ascii_5x7
import unifont
import glob
import time
import os, sys
import Tkinter
import tkFileDialog
import Image, ImageTk
import ImageEnhance
import PIL.ImageOps
import struct
import os.path
import subprocess

w = 264
h = 176
W = 3 * w
H = 3 * h
file_opt={}
DEFAULT_IMAGE = 'DEFAULT.PNG'
EPD_LARGE = (w, h, 2 * w, 2 * h)
EPD_MED = (w, h, w + 200, h + 96)
EPD_SMALL = (w, h, w + 128, h + 96)
HEAD_SHOT = (w + 112, h, 2 * w, h + 128)
WHITE = 255
BLACK = 0
BACKSPACE = chr(8)
RETURN = chr(13)
ESC = chr(27)
LEFT_KC = 113
RIGHT_KC = 114
current_event = None

default_path = '.'

def intersect_rectangles(bbox1, bbox2):
    left = max([bbox1[0], bbox2[0]])
    top = max([bbox1[1], bbox2[1]])
    right = min([bbox1[2], bbox2[2]])
    bottom = min([bbox1[3], bbox2[3]])
    if left >= right or top >= bottom:
        out = None
    else:
        out = [left, top, right, bottom]
    # id = canvas.create_rectangle(out, fill='blue')
    # raw_input('...')
    # canvas.delete(id)
    return out

#### Callbacks
def file_open_dialog():
    out = tkFileDialog.askopenfilename(**file_opt)
    if out:
        background.delete_all()
        foreground = WIF(out, background)
        foreground.select()

def RaspiCamera():
    filename='test.jpg'
    cmd = 'raspistill -t 1000 -o '+ filename + ' -w '+ str(w) + ' -h ' +str(h)
    pid = subprocess.call(cmd, shell=True)
    foreground = WIF(filename, background)
    foreground.select()

def file_import_dialog():
    out = tkFileDialog.askopenfilename(**file_opt)
    if out:
        foreground = WIF(out, background)
        foreground.select()

def file_save_dialog():
    fn = tkFileDialog.asksaveasfilename(
        defaultextension='.WIF',
        filetypes=[
            ('WyoLum Image Format', '.WIF'),
            ('Portable Network Graphics', '.png')
            ])
    if fn:
        background.file_save(fn)

def next(step=1):
    current = background.get_current()
    if hasattr(current, 'fn') and current.fn is not None:
        abspath = os.path.abspath(current.fn)
        path, fn = os.path.split(abspath)
    else:
        path = default_path
        fn = 'DEFAULT.PNG'
        abspath = os.path.abspath(os.path.join(path, fn))
    files = glob.glob(path + '/*.png')
    files.extend(glob.glob(path + '/*.PNG'))
    files.extend(glob.glob(path + '/*.wif'))
    files.extend(glob.glob(path + '/*.WIF'))
    files.sort()
    if abspath in files:
        i = files.index(abspath) + step
        i %= len(files)
    else:
        i = 0
    background.delete_all()
    foreground = WIF(files[i], background)
    foreground.select()
    
def curry(func, *args, **kw):
    '''
    function wrapper for use in call back
    '''
    def out():
        func(*args, **kw)
    return out

def towif(im, outfn):
    ''' 
    image should be sized already (2.7" display=264x276 pixels) in "1" format
    '''
    width, height = im.size
    f = open(outfn, 'w')
    f.write(struct.pack('HH', height, width))
    ## faster, but requires numpy download
    ## np.shape(np.sum([d8[:,:,i] << i for i in range(8)], axis=0))
    # data = wif.getdata()
    # for i in range(0, len(data), 8):
    #     byte = data[i:i + 8]
    for j in range(height):
        for i in range(0, width, 8):
            byte = 0
            for bit_i in range(8):
                try:
                    bit = im.getpixel((i + bit_i, j)) < 255
                except:
                    bit = False
                # sys.stdout.write(' X'[bit])
                byte |= bit << bit_i
            f.write(struct.pack('B', byte))

class WIF:
    def __init__(self, fn, parent, size=None):
        self.contrast = 1.
        self.brightness = 1.
        self.init_image(fn, size=size)
        self.parent = parent
        self.children = []
        if not isinstance(self.parent, Tkinter.Canvas):
            self.parent.children.insert(0, self) ## put new child on top of stack
        self.owns_event = False ## keep track of widget that owns a mouse drag
        self.selected = False   ## select with left mouse no drag
        self.dragging = False

    def invert_image(self, event=None):
        child = self.get_selected()
        if child:
            try:
                child.image1 = PIL.ImageOps.invert(child.image1.convert('L'))
                background.show()
            except IOError:
                raise

    def ctrl_c(self, event):
        global tempf
        child = self.get_selected()
        if child is not None:
            root.tempf = tempfile.TemporaryFile()
            child.image1.save(root.tempf, 'png')
            dump = json.dumps([{'contrast':child.contrast, 
                                'brightness':child.brightness, 
                                'sscale':child.sscale, 
                                'pos':(child.pos[0], child.pos[1])}])
            paste = 'copy://' + dump
            root.clipboard_clear()
            root.clipboard_append(paste)

    def ctrl_v(self, event):
        global current_event
        current_event = event
        try:
            path = root.clipboard_get()
            foreground = WIF(path, self)
            foreground.select()
            foreground.show()
        except Exception, e:
            print e
            pass

    def key_event(self, event):
        out = False
        child = self.get_selected()
        if child is not None and child != self: ## prevent infinite loops
            out = child.key_event(event)
        if not out:
            ## event is not handled.  see if it is an vi edit command
            if event.char == 'i':
                wtext = WText(background, '', unifont_f=True, bigascii=False)
                out = True
            elif event.char == 'I':
                wtext = WText(background, '', unifont_f=True, bigascii=True)
                out = True
            elif event.char == 'T':
                wtext = WText(background, '', unifont_f=False)
                out = True
        return out

    def get_current(self):
        '''
        return selected if something is selected else get top if there is a top else return None
        '''
        out = self.get_selected()
        if out is None:
            if len(self.children) > 0:
                out = self.children[0]
        return out

    def get_selected(self):
        out = None
        if self.selected:
            out = self
        else:
            for child in self.children:
                out = child.get_selected()
                if out:
                    break
        return out
    
    def z_order_change(self, delta):
        i = self.parent.children.index(self)
        self.parent.children.pop(i)
        if delta > 0:
            if i - delta > 0:
                self.parent.children.insert(i - delta, self)
            else:
                self.parent.children.insert(0, self)
        else:
            n = len(self.parent.children)
            if i - delta < n:
                self.parent.children.insert(i - delta, self)
            else:
                self.parent.children.insert(n, self)
        background.show()

    def z_order_push(self):
        self.z_order_change(-1)

    def z_order_pull(self):
        self.z_order_change(1)
    
    def z_order_top(self):
        self.parent.children.remove(self)
        self.parent.children.insert(0, self)
        background.show()

    def z_order_bottom(self):
        self.parent.children.remove(self)
        n = len(self.parent.children)
        self.parent.children.insert(n, self)
        background.show()

    def select(self):
        background.unselect()
        self.selected = True
        contrast.set(self.contrast)
        brightness.set(self.brightness)
        background.show()
        title = 'WyoLum Image Format!   '
        if self.fn is not None:
            title = title + os.path.split(self.fn)[-1]
        root.wm_title(title)

    def unselect(self):
        self.selected = False
        self.canvas.delete("highlight")
        for child in self.children:
            child.unselect()

    def contains(self, x, y):
        out = self.bbox[0] < x and x < self.bbox[2]
        out *= self.bbox[1] < y and y < self.bbox[3]
        return out

    def getCanvas(self):
        if isinstance(self.parent, Tkinter.Canvas):
            out = self.parent
        else:
            out = self.parent.getCanvas()
        return out
    canvas = property(getCanvas)

    def set_area(self, area):
        self.canvas.delete("active")
        self.active_region = area
        self.active_region_id = canvas.create_rectangle(self.active_region, tags=("rect", "active"))
        ar_p1 = list(self.active_region)
        ar_p1[0] -= 1
        ar_p1[1] -= 1
        ar_p1[2] += 1
        ar_p1[3] += 1
        self.active_region_id = canvas.create_rectangle(ar_p1, tags=("rect", "active"), outline='white')

    def equalize(self):
        if self.image1 is not None:
            contr = ImageEnhance.Contrast(self.image1)
            image = contr.enhance(self.contrast)
            bright = ImageEnhance.Brightness(image)
            image = bright.enhance(self.brightness)
            image = image.resize((int(image.size[0] * self.scale), 
                                  int(image.size[1] * self.scale)))
            self.wif = image.convert('1')

    def show(self, subordinate=False):
        self.equalize()
        if self.id is not None:
            self.canvas.delete(self.id)
        self.wiftk = ImageTk.PhotoImage(self.wif)
        x, y = self.wif.size
        self.id = self.canvas.create_image([self.pos[0] + x / 2.,
                                            self.pos[1] + y / 2.], 
                                           image=self.wiftk, 
                                           tags="image")
        self.bbox = [self.pos[0], self.pos[1], self.pos[0] + x, self.pos[1] + y]
        if self.selected:
            self.canvas.delete("highlight")
            self.highlight_id = self.canvas.create_rectangle((self.bbox[0] - 1,
                                                              self.bbox[1] - 1,
                                                              self.bbox[2] + 1,
                                                              self.bbox[3] + 1), 
                                                             tags=["highlight", "rect"],
                                                             outline='blue')
        for child in self.children[::-1]:
            child.show(subordinate=True)
        
        if not subordinate:
            self.canvas.tag_raise("rect")

    def move(self, dx, dy):
        if self != background:
            self.pos = (self.pos[0] + dx, self.pos[1] + dy)
            self.canvas.move(self.id, dx, dy)
            self.bbox[0] += dx
            self.bbox[2] += dx
            self.bbox[1] += dy
            self.bbox[3] += dy
            ## lock children?
            # for child in self.children:
            #     child.move(dx, dy)
            if self.selected:
                self.canvas.move(self.highlight_id, dx, dy)
    def locate_handler(self, event):
        for child in self.children:
            if child.owns_event:
                out = child
                break
        else:
            if self.owns_event:
                out = self
            else:
                for child in self.children:
                    if child.contains(event.x, event.y):
                        out = child
                        break
                else:
                    out = self
        return out

    def button_down(self, event):
        global current_event
        current_event = event
        handler = self.locate_handler(event)
        handler.start = (event.x, event.y)
        handler.owns_event = True
        if handler != background:
            handler.select()
        handler.dragging = False

    def drag(self, event):
        self.dragging = True
        handler = self.locate_handler(event)
        if handler.start[0] is not None:
            dx = event.x - handler.start[0]
            dy = event.y - handler.start[1]
            handler.move(dx, dy)
            handler.start = (event.x, event.y)

    def resize(self, event, subordinate=False):
        handler = self.locate_handler(event)
        if handler != background:
            handler.dragging = False
            if handler.start[0] is None:
                handler.button_down(event)
            if False:
                x0 = handler.bbox[0]
                x1minus = handler.bbox[2]
                delta_x = event.x - handler.start[0]
                x1plus = x1minus + delta_x
                if (x1minus == x0):
                    handler.scale = 0
                else:
                    handler.scale = float(x1plus - x0) / (x1minus - x0)
            else:
                handler.scale = handler.sscale + (event.x - handler.start[0]) / float(W)
            handler.bbox[2] = handler.bbox[0] + handler.scale * (handler.bbox[2] - handler.bbox[0])
            handler.bbox[3] = handler.bbox[1] + handler.scale * (handler.bbox[3] - handler.bbox[1])
            background.show()

    def release_event(self, event):
        handler = self.locate_handler(event)
        handler.owns_event = False
        if not handler.dragging: ## must have been a click
            ## unselect all
            background.unselect()
            
            ## select the clicked object
            handler.select()
            background.show()
        else:
            self.selected = False

    def save_scale(self, event):
        handler = self.locate_handler(event)
        handler.sscale = handler.scale
        for child in handler.children:
            child.sscale = child.scale
        handler.owns_event = False

    def delete_selected(self, event):
        child = self.get_selected()
        if child is not None:
            child.delete()
            self.children.remove(child)

    def delete(self):
        self.unselect()
        self.canvas.delete(self.id)

        try:
            self.canvas.delete(self.highlight_id)
            self.canvas.delete(self.id)
            background.show()
        except:
            pass

    def delete_all(self, event=None):
        for child in self.children:
            child.delete()
            self.children.remove(child)
        del self.children
        self.children = []
    def push_selected(self, event):
        child = self.get_selected()
        if child is not None:
            child.z_order_push()

    def pull_selected(self, event):
        child = self.get_selected()
        if child is not None:
            child.z_order_pull()

    def top_selected(self, event):
        child = self.get_selected()
        if child is not None:
            child.z_order_top()
        
    def bottom_selected(self, event):
        child = self.get_selected()
        if child is not None:
            child.z_order_bottom()

    def set_contrast(self, contrast_val):
        select = self.get_selected()
        if select:
            select.contrast = float(contrast_val)
            background.show()

    def set_brightness(self, brightness_val):
        select = self.get_selected()
        if select:
            select.brightness = float(brightness_val)
            background.show()

    def init_image(self, fn, size=None):
        global default_path
        prescaled = False

        if fn is None:
            if size is None: ## default to full screen
                size = (W + 2, H + 2)
            ## background image (all white)
            self.image1 = Image.new('L', size, WHITE)
        elif fn.startswith('copy://'):
            data = fn[len('copy://'):]
            data = json.loads(data)
            root.tempf.seek(0)
            buff = root.tempf.read()
            buff = StringIO.StringIO(buff)
            self.image1 = Image.open(buff)
            self.contrast  = data[0]['contrast']
            self.brightness = data[0]['brightness']
            self.scale = self.sscale = data[0]['sscale']
            self.pos = (current_event.x - self.image1.size[0] * self.scale / 2, 
                        current_event.y - self.image1.size[1] * self.scale / 2)
            #'pos':(event.mouse.x, event.mous.y)            
            prescaled = True
            
        elif fn.startswith('http://'):
            try:
                data = StringIO.StringIO(urllib2.urlopen(fn).read())
                self.image1 = Image.open(data)
                size = self.image1.size
            except:
                raise
                pass
        elif fn.upper().endswith('.WIF'):
            ## read in WIF format
            def bit(val, i): 
                '''
                Return the ith bit from byte val
                '''
                return ord(val) >> i & 1

            f = open(fn)
            try:
                height, width = struct.unpack('HH', f.read(4))
                size = (width, height)
                dat = f.read(height * width // 8)
            ## turn each byte into 8 pixel values
                ldat = [(1 - bit(byte, j)) * 255 for byte in dat for j in range(8)]
            except:
                ldat = []
                size = (w, h)
            self.image1 = Image.new('L', size, WHITE)
            self.image1.putdata(ldat)
        else:
            ## read in all other formats
            self.image1 = Image.open(fn)
        x, y = self.image1.size
        if prescaled:
            pass
        elif x > W or y > H:
            ## rescale to fit
            scale = min([ W / float(x), H / float(y) ])
            self.scale = scale
            self.pos = (0, 0)
        elif ((HEAD_SHOT[2] - HEAD_SHOT[0] == x) and
              (HEAD_SHOT[3] - HEAD_SHOT[1] == y)):
            self.scale = 1.
            self.pos = HEAD_SHOT[:2]
        elif ((EPD_MED[2] - EPD_MED[0] == x) and
              (EPD_MED[3] - EPD_MED[1] == y)):
            self.scale = 1.
            self.pos = EPD_MED[:2]
        elif ((EPD_SMALL[2] - EPD_SMALL[0] == x) and
              (EPD_SMALL[3] - EPD_SMALL[1] == y)):
            self.scale = 1.
            self.pos = EPD_SMALL[:2]
        else:
            self.scale = 1.
            self.pos = ((W - x)/2, (H - y)/2)
        if prescaled:
            print 'prescaled', self.pos, self.sscale
            pass
        else:
            self.contrast = 1.
            self.brightness = 1.
            self.sscale = self.scale      ## saved between drags
        self.fn = fn
        self.id = None
        self.start = (None, None)
        if fn:
            path, fn = os.path.split(fn)
            default_path = os.path.abspath(path)
    def copy_active_region(self):
        x, y = self.wif.size
        bbox = intersect_rectangles(background.active_region, (self.pos[0],
                                                         self.pos[1],
                                                         self.pos[0] + x,
                                                         self.pos[1] + y))
        if bbox is not None:
            out = self.wif.crop([
                    # self.active_region[0] - self.pos[0],
                    # self.active_region[1] - self.pos[1],
                    # self.active_region[2] - self.pos[0],
                    # self.active_region[3] - self.pos[1]
                    bbox[0] - self.pos[0],
                    bbox[1] - self.pos[1],
                    bbox[2] - self.pos[0],
                    bbox[3] - self.pos[1]
                    ])
                ### copy sub to out
        else:
            out = None
            bbox = None
        return out, bbox

    def file_save(self, fn):
        im, my_bbox = self.copy_active_region()
        for c in self.children[::-1]:
            ar, c_bbox = c.copy_active_region()
            if ar:
                # canvas.create_rectangle(my_bbox,fill='red')
                # canvas.create_rectangle(c_bbox,fill='blue')
                im.paste(ar, 
                          (max([c_bbox[0] - my_bbox[0], 0]),
                           max([c_bbox[1] - my_bbox[1], 0])))
        if fn.upper().endswith('.WIF'):
            dir, fn = os.path.split(fn)
            fn = os.path.join(dir, fn.upper())
            towif(im, fn)
            print 'wrote WIF', fn
        elif fn.lower().endswith('.png'):
            im.save(fn)
            print 'wrote PNG', fn
        else:
            pass 

class WText(WIF):
    def __init__(self, parent, text='', unifont_f=True, bigascii=False):
        self.text=text
        self.unifont_f = unifont_f
        self.bigascii = bigascii ## needed for self.size
        WIF.__init__(self, fn=None, parent=parent, size=self.size)
        if current_event is not None:
            self.pos = current_event.x, current_event.y
        self.layout_text()
        self.cursor = len(self.text)
        self.select()

    def layout_text(self):
        '''
        Put the text on the image.  Does not resize image1
        '''
        if self.unifont_f:
            unifont.addText(self.text, self.image1, 0, 0, self.bigascii)
        else: ## default to 5x7 font
            ascii_5x7.addText(self.text, self.image1, 0, 0)

    def insert(self, text, index=None):
        if index is not None:
            self.cursor = index
        self.text = self.text[:self.cursor] + text + self.text[self.cursor:]
        self.cursor += len(text)
        self.canvas.delete(self.id) ## delete old image
        self.image1 = Image.new('L', self.size, WHITE)
        self.layout_text()
        background.show()

    def get_size(self):
        if self.unifont_f:
            size = unifont.calcsize(self.text, bigascii=self.bigascii)
        else: ## default to 5x7 font
            size = ascii_5x7.calcsize(self.text)
        if size[0] == 0:
            size = (1, size[1])
        return size
    size = property(get_size)
    
    def backspace(self):
        if self.cursor > 0:
            self.text = self.text[:self.cursor - 1] + self.text[self.cursor:]
            self.canvas.delete(self.id) ## delete old image
            self.image1 = Image.new('L', self.size, WHITE)
            self.layout_text()
            background.show()
            self.cursor -= 1
    def key_event(self, event):
        char = event.char
        out = False
        if event.keycode == LEFT_KC:
            if self.cursor > 0:
                self.cursor -= 1
            out = True
        elif event.keycode == RIGHT_KC:
            if self.cursor < len(self.text):
                self.cursor += 1
            out = True
        elif char == BACKSPACE:
            self.backspace()
            out = True
        elif char == RETURN:
            current_event.y += self.size[1]
            self.unselect()
            out = True
        elif char == ESC:
            self.unselect()
            out = True
        else:
            self.insert(char, self.cursor)
            out = True
        return out
    
root = Tkinter.Tk()
root.wm_title('WyoLum Image Format!')
canvas = Tkinter.Canvas(root, width=W, height=H)
canvas.pack()


# main
import sys
background = WIF(None, canvas)
if len(sys.argv) > 1:
    for fn in sys.argv[1:]:
        WIF(fn, background)
else:
    WIF(DEFAULT_IMAGE, background)

def printstack(*args, **kw):
    print background
    for child in background.children:
        print '  ', child
        for gc in child.children:
            print '    ', gc
    print
# foreground = WIF('WYO.WIF', background)
canvas.bind('<Button>', background.button_down)
canvas.bind('<B1-Motion>', background.drag)
canvas.bind('<B3-Motion>', background.resize)
canvas.bind('<ButtonRelease-1>', background.release_event)
canvas.bind('<ButtonRelease-3>', background.save_scale)
root.bind('<Control-i>', background.invert_image)
root.bind('<Control-c>', background.ctrl_c)
root.bind('<Control-v>', background.ctrl_v)
root.bind('<Control-a>', printstack)
root.bind('<Key>', background.key_event)
root.bind('<Delete>', background.delete_selected)
root.bind('<Prior>', background.pull_selected) ## page up
root.bind('<Next>', background.push_selected)  ## page down
root.bind('<Home>', background.top_selected)
root.bind('<End>', background.bottom_selected)


control_frame = Tkinter.Frame(root)
prev_b = Tkinter.Button(control_frame, text="Prev", command=curry(next, -1))
prev_b.pack(side=Tkinter.LEFT)
contrast = Tkinter.Scale(control_frame, from_=-5, to = 5, 
                         orient=Tkinter.HORIZONTAL, 
                         label='Contrast', 
                         command=background.set_contrast, 
                         resolution=.01)
contrast.set(1.)
contrast.pack(side=Tkinter.LEFT)
brightness = Tkinter.Scale(control_frame, 
                           from_=0, to = 5, 
                           orient=Tkinter.HORIZONTAL, 
                           label='Brightness', 
                           command=background.set_brightness, 
                           resolution=.01)
brightness.set(1.)
brightness.pack(side=Tkinter.LEFT)
next_b = Tkinter.Button(control_frame, text="Next", command=next)
next_b.pack(side=Tkinter.RIGHT)
control_frame.pack()

menubar = Tkinter.Menu(root)
root.config(menu=menubar)
fileMenu = Tkinter.Menu(menubar)
fileMenu.add_command(label="Open", command=file_open_dialog)
fileMenu.add_command(label="Save", command=file_save_dialog)
fileMenu.add_command(label="Exit", command=root.quit)
menubar.add_cascade(label="File", menu=fileMenu)

editMenu = Tkinter.Menu(menubar)
editMenu.add_command(label="Invert", command=background.invert_image)
menubar.add_cascade(label="Edit", menu=editMenu)

sizeMenu = Tkinter.Menu(menubar)
sizeMenu.add_command(label="EPD_LARGE", command=curry(background.set_area, EPD_LARGE))
sizeMenu.add_command(label="EPD_MED", command=curry(background.set_area, EPD_MED))
sizeMenu.add_command(label="EPD_SMALL", command=curry(background.set_area, EPD_SMALL))
sizeMenu.add_command(label="HEAD_SHOT", command=curry(background.set_area, HEAD_SHOT))
menubar.add_cascade(label="Size", menu=sizeMenu)

insertMenu = Tkinter.Menu(menubar)
insertMenu.add_command(label="Take Picture", command=RaspiCamera)
insertMenu.add_command(label="Image", command=file_import_dialog)
insertMenu.add_command(label="Big ASCII Text", command=curry(WText, background, '', unifont_f=True, bigascii=True))
insertMenu.add_command(label="Unifont Text", command=curry(WText, background, '', unifont_f=True, bigascii=False))
insertMenu.add_command(label="5x7 Text",  command=curry(WText, background, '', unifont_f=False, bigascii=False))
insertMenu.add_command(label="4x4 Text")
menubar.add_cascade(label="Insert", menu=insertMenu)

background.set_area(EPD_LARGE)
background.show()
root.mainloop()    

