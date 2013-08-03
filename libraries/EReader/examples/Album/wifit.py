import os, sys
import Tkinter
import tkFileDialog
import Image, ImageTk
import ImageEnhance
import struct
import os.path

w = 264
h = 176
W = 3 * w
H = 3 * h
file_opt={}
ACTIVE_REGION = (w, h, 2 * w, 2 * h)

def towif(im, outfn, width, height):
    ''' 
    image should be sized already (2.7" display=264x276 pixels) in "1" format
    '''
    f = open(outfn, 'w')
    f.write(struct.pack('HH', height, width))
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
    def __init__(self, fn, canvas):
        self.file_open(fn)
        self.canvas = canvas
        canvas.create_rectangle(ACTIVE_REGION, tags="rect")

    def equalize(self):
        if self.image1 is not None:
            contr = ImageEnhance.Contrast(self.image1)
            image = contr.enhance(self.contrast)
            bright = ImageEnhance.Brightness(image)
            image = bright.enhance(self.brightness)
            image = image.resize((int(image.size[0] * self.scale), 
                                  int(image.size[1] * self.scale)))
            self.wif = image.convert('1')

    def show(self):
        self.equalize()
        if self.id is not None:
            self.canvas.delete(self.id)
        self.wiftk = ImageTk.PhotoImage(self.wif)
        x, y = self.wif.size
        self.id = self.canvas.create_image([self.pos[0] + x/2.,
                                            self.pos[1] + y/2.], 
                                           image=self.wiftk, 
                                           tags="image")
        self.canvas.bind('<Button>', self.down)
        self.canvas.bind('<B1-Motion>', self.drag)
        self.canvas.bind('<B3-Motion>', self.resize)
        self.canvas.bind('<ButtonRelease-3>', self.save_scale)
        self.canvas.tag_raise("rect")

    def down(self, event):
        self.start = (event.x, event.y)

    def drag(self, event):
        dx = event.x - self.start[0]
        dy = event.y - self.start[1]
        self.pos = (self.pos[0] + dx, self.pos[1] + dy)
        self.canvas.move(self.id, 
                         dx, 
                         dy)
        self.start = (event.x, event.y)

    def resize(self, event):
        self.scale = self.sscale + (event.x - self.start[0]) / float(W)
        self.pos = (self.start[0] + (self.start[0] - self.pos[0]) * self.scale,
                    self.start[1] + (self.start[1] - self.pos[1]) * self.scale)
        self.show()

    def save_scale(self, event):
        self.sscale = self.scale

    def set_contrast(self, contrast_val):
        self.contrast = float(contrast_val)
        self.show()

    def set_brightness(self, brightness_val):
        self.brightness = float(brightness_val)
        self.show()

    def file_open(self, fn):
        self.image1 = Image.open(fn).convert('L')
        x, y = self.image1.size
        if x > W or y > H:
            ## rescale to fit
            scale = min([ W / float(x), H / float(y) ])
            self.scale = scale
        else:
            self.scale = 1.       ## during drags
        self.sscale = scale      ## saved between drags
        self.fn = fn
        self.contrast = 1.
        self.brightness = 1.
        self.pos = (0, 0)
        self.id = None
        self.start = (None, None)

    def file_save(self, fn):
        wif = self.wif.crop([
                ACTIVE_REGION[0] - self.pos[0],
                ACTIVE_REGION[1] - self.pos[1],
                ACTIVE_REGION[2] - self.pos[0],
                ACTIVE_REGION[3] - self.pos[1]
                ])
        if fn.endswith('.WIF'):
            dir, fn = os.path.split(fn)
            fn = os.path.join(dir, fn.upper())
            towif(wif, fn, W, H)
            print 'wrote WIF', fn
        elif fn.lower().endswith('.png'):
            wif.save(fn)
            print 'wrote PNG', fn
        else:
            pass
    def save(self, event):
        pass
root = Tkinter.Tk()
canvas = Tkinter.Canvas(root, width=W, height=H)
canvas.pack()
wif = WIF('heart_beat.png', canvas)
# wif = WIF('FlowMeter.png', canvas)

control_frame = Tkinter.Frame(root)
contrast = Tkinter.Scale(control_frame, from_=-5, to = 5, 
                         orient=Tkinter.VERTICAL, 
                         label='Contrast', 
                         command=wif.set_contrast, 
                         resolution=.1)
contrast.set(1.)
contrast.pack(side=Tkinter.LEFT)
brightness = Tkinter.Scale(control_frame, 
                           from_=0, to = 5, 
                           orient=Tkinter.VERTICAL, 
                           label='Brightness', 
                           command=wif.set_brightness, 
                           resolution=.1)
brightness.set(1.)
brightness.pack(side=Tkinter.LEFT)
control_frame.pack()
def file_open_dialog():
    out = tkFileDialog.askopenfilename(**file_opt)
    wif.file_open(out)
    wif.show()
def file_save_dialog():
    # wif = myEqualize(im, contrast_val, brightness_val)
    fn = tkFileDialog.asksaveasfilename(
        defaultextension='.png',
        filetypes=[
            ('WyoLum Image Format', '.WIF'),
            ('Portable Network Graphics', '.png')
            ])
    wif.file_save(fn)

menubar = Tkinter.Menu(root)
root.config(menu=menubar)
fileMenu = Tkinter.Menu(menubar)
fileMenu.add_command(label="Open", command=file_open_dialog)
fileMenu.add_command(label="Save", command=file_save_dialog)
fileMenu.add_command(label="Exit", command=root.quit)
menubar.add_cascade(label="File", menu=fileMenu)

wif.show()
root.mainloop()    

