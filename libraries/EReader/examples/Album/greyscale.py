import glob
import os.path
import struct
# from PIL import Image 
import PIL.Image
import ImageTk
import ImageEnhance
import ImageFilter
import sys
from wif import towif

im = None ### avoid "NameError: global name 'im' is not defined" error

from Tkinter import *
import tkFileDialog
file_opt={}
FILENAME = None

def file_open():
    out = tkFileDialog.askopenfilename(**file_opt)
    display_im(out)

def file_save(fn):
    if fn.endswith('.WIF'):
        dir, fn = os.path.split(fn)
        fn = os.path.join(dir, fn.upper())
        towif(wif, fn, W, H)
        print 'wrote', fn
    elif fn.lower().endswith('.png'):
        wif.save(fn)
        print 'wrote', fn
    else:
        pass

def file_save_dialog():
    # wif = myEqualize(im, contrast_val, brightness_val)
    fn = tkFileDialog.asksaveasfilename(defaultextension='.WIF',
                                        filetypes=[('WyoLum Image Format', '.WIF'),
                                                   ('Portable Network Graphics', '.png')])
    file_save(fn)

def mesh_display(fn):
    import struct
    import numpy
    import pylab
    f = open(fn)
    h, w = struct.unpack('HH', f.read(4))
    bpl = w / 8
    dat = f.read()
    bytes = numpy.array(map(ord, dat)).reshape((h, bpl))

    bits = numpy.zeros((h, w), bool)
    for i, line in enumerate(bytes):
        for j, b in enumerate(line):
            for k in range(8):
                bits[i,j * 8 + k] = (b >> k) & 1
    
    pylab.pcolormesh(bits[::-1], cmap=pylab.cm.binary)
    pylab.show()

def myEqualize(im, contrast=1, brightness=1):
    if im is not None:
        im = im.convert('L')
        contr = ImageEnhance.Contrast(im)
        im = contr.enhance(contrast)
        bright = ImageEnhance.Brightness(im)
        im = bright.enhance(brightness)
        #im.show()
    return im

def curry(func, *args, **kw):
    def out():
        func(*args, **kw)
    return out

## set canvas size dynamically
SIZES = {'SMALL':(128, 96),
         'MEDIUM':(200, 96),
         'LARGE':(264, 176)}

def setWH(w, h):
    global W, H, im
    W = w
    H = h
    if im is not None:
        resize_im()
setWH(*SIZES['LARGE'])

### start building GUI
root = Tk()
root.title("WyoLum Image Format")

### menu
menubar = Menu(root)
root.config(menu=menubar)
fileMenu = Menu(menubar)
fileMenu.add_command(label="Open", command=file_open)
fileMenu.add_command(label="Save", command=file_save_dialog)
fileMenu.add_command(label="Exit", command=root.quit)
menubar.add_cascade(label="File", menu=fileMenu)

sizeMenu = Menu(menubar)
sizeMenu.add_command(label="Small", command=curry(setWH, *SIZES['SMALL']))
sizeMenu.add_command(label="Medium", command=curry(setWH, *SIZES['MEDIUM']))
sizeMenu.add_command(label="Large", command=curry(setWH, *SIZES['LARGE']))
menubar.add_cascade(label="Size", menu=sizeMenu)

### canvas

canvas = Canvas(root, width=SIZES['LARGE'][0], height=SIZES['LARGE'][1] * 2 + 10)

def resize_im():
    global im, imtk
    im = PIL.Image.open(FILENAME)

    im = im.resize((W, H))
    imtk = ImageTk.PhotoImage(im)
    canvas.create_image([W/2, H/2], image=imtk)
    try:
        image_update()
    except:
        pass
    
## Original
def display_im(im_filename):
    global FILENAME
    FILENAME = im_filename
    resize_im()

# display_im('Brian_Krontz.jpg')

## WIF
def wifme_contrast(contrast):
    global contrast_val
    contrast_val = float(contrast)
    image_update()

def wifme_brightness(brightness):
    global brightness_val
    brightness_val = float(brightness)
    image_update()

wiftk_id = None
def image_update():
    global wiftk, wiftk_id, wif
    wif = myEqualize(im, contrast_val, brightness_val)
    if wif is not None:
        wif= wif.convert('1') # convert image to black and white
        if wiftk_id is not None:
            canvas.delete(wiftk_id)
        wiftk = ImageTk.PhotoImage(wif)
        wiftk_id = canvas.create_image([W/2, H + H/2 + 10], image=wiftk)
def load_next_image():
    dir, fn = os.path.split(FILENAME)
    pngs = glob.glob(os.path.join(dir, '*.png'))
    pngs.sort()
    i = pngs.index(FILENAME) + 1
    if i >= len(pngs):
        i = 0
    display_im(pngs[i])

def png_and_next():
    if FILENAME and FILENAME.endswith('.png'):
        wif_fn = FILENAME[:-4] + '.png'
        file_save(wif_fn)
        load_next_image()
    

canvas.pack()
## sliders
f = Frame(root)
contrast_val = 1.
brightness_val = 1.
contrast = Scale(f, from_=-5, to = 5, orient=VERTICAL, label='Contrast', command=wifme_contrast, resolution=.1)
contrast.set(contrast_val)
contrast.pack(side=LEFT)
brightness = Scale(f, from_=0, to = 5, orient=VERTICAL, label='Brightness', command=wifme_brightness, resolution=.1)
brightness.set(brightness_val)

brightness.pack(side=LEFT)
f.pack()
f = Frame(root)
Button(f, text='Next', command=load_next_image).pack(side=LEFT)
Button(f, text='PNG', command=png_and_next).pack(side=LEFT)
f.pack()
print sys.argv
if len(sys.argv) > 1:
    fn = sys.argv[1]
    display_im(fn)
    print fn
root.mainloop()

def main():
    outfn = fn[:3] + 'bmp'
    wif_fn = fn[:3] + 'WIF'
    
if False:    
    image_file = Image.open(fn) # open colour image
    image_file= image_file.convert('L') # convert image to monochrome - this works
    import ImageEnhance

    image_file = myEqualize(image_file)
    image_file= image_file.convert('1') # convert image to black and white
    image_file = image_file.resize((264, 176))
    image_file.save(outfn)

    towif(image_file, wif_fn)
