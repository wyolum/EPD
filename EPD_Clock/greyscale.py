import os.path
import struct
# from PIL import Image 
import PIL.Image
import ImageTk
import ImageEnhance
import ImageFilter

from Tkinter import *
import tkFileDialog
file_opt={}
def file_open():
    out = tkFileDialog.askopenfilename(**file_opt)
    display_im(out)

def file_save():
    wif = myEqualize(im, contrast_val, brightness_val)
    wif_fn = tkFileDialog.asksaveasfilename(defaultextension='WIF',
                                            filetypes=[('WyoLum Image Format', '.WIF')])
    if wif_fn:
        dir, fn = os.path.split(wif_fn)
        towif(wif, os.path.join(dir, fn.upper()))
    
def towif(im, outfn):
    ''' 
    image should be 264x176 pixels in "1" format
    '''
    f = open(outfn, 'w')
    w = 264
    h = 176
    f.write(struct.pack('HH', h, w))
    for j in range(h):
        for i in range(0, w, 8):
            byte = 0
            for bit_i in range(8):
                bit = im.getpixel((i + bit_i, j)) < 255
                byte |= bit << bit_i
            f.write(struct.pack('B', byte))
    print 'wrote', outfn

def myEqualize(im, contrast=2, brightness=2):
    im=im.convert('L')
    contr = ImageEnhance.Contrast(im)
    im = contr.enhance(contrast)
    bright = ImageEnhance.Brightness(im)
    im = bright.enhance(brightness)
    #im.show()
    return im

def dummy():
    pass
### start building GUI
root = Tk()
root.title("WyoLum Image Format")

### menu
menubar = Menu(root)
root.config(menu=menubar)
fileMenu = Menu(menubar)
fileMenu.add_command(label="Open", command=file_open)
fileMenu.add_command(label="Save", command=file_save)
fileMenu.add_command(label="Exit", command=root.quit)
menubar.add_cascade(label="File", menu=fileMenu)

### canvas
W = 264
H = 176
canvas = Canvas(root, width=W, height=H * 2 + 10)

## Original
def display_im(im_filename):
    global im, imtk
    im = PIL.Image.open(im_filename)
    im.resize((W, H))
    imtk = ImageTk.PhotoImage(im)
    canvas.create_image([W/2, H/2], image=imtk)
    try:
        image_update()
    except:
        pass
display_im('Brian_Krontz.jpg')

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
    global wiftk, wiftk_id
    wif = myEqualize(im, contrast_val, brightness_val)
    wif= wif.convert('1') # convert image to black and white
    if wiftk_id is not None:
        canvas.delete(wiftk_id)
    wiftk = ImageTk.PhotoImage(wif)
    wiftk_id = canvas.create_image([W/2, H + H/2 + 10], image=wiftk)

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
root.mainloop()

def main():
    import sys
    fn = sys.argv[1]
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
    print 'wrote', outfn

    towif(image_file, wif_fn)
