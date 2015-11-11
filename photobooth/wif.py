import PIL.Image as Image
from PIL import ImageFont
import PIL.ImageDraw as ImageDraw
import struct
import unifont
import ascii_5x7

PAGE = (264, 176)
WIDTH = PAGE[0]
HEIGHT = PAGE[1]
WHITE = 255
BLACK = 0
BREADCRUMB_SIZE = 2

FONT_NAME = 'Orbitron' # 'Ovo' # 'Ubuntu'
FONT_STYLE = 'Regular'
FONT_DIR = '/home/justin/Dropbox/WyoLumCode/fonts/'
NAME_FONT_SIZE = 35
NORMAL_FONT_SIZE = 20

class WIF:
    wff_file = open('unifont.wff')
    def __init__(self, size=PAGE, background=None):
        self.size = size
        self.im = Image.new('1', size, WHITE)
        self.draw = ImageDraw.Draw(self.im)
        if background:
            self.im.paste(background)

    def addImage(self, img, x, y):
        box = (x, y, x + img.size[0], y + img.size[1])
        self.im.paste(img, box)

    def addUnifont(self, txt, x, y, **kw):
        unifont.addText(txt, self.im, x, y, self.wff_file, **kw)

    def add_7x5_txt(self, txt, x, y, **kw):
        ascii_5x7.addText(txt, self.im, x, y, **kw)
        
    def addBreadCrumb(self, x, y, row, col, shape, color=BLACK):
        startx = WIDTH - max(shape) * BREADCRUMB_SIZE
        starty = HEIGHT - len(shape) * BREADCRUMB_SIZE
        for i, n_col in enumerate(shape):
            for j in range(n_col):
                self.draw.rectangle((startx + x + BREADCRUMB_SIZE * j          , starty + y + BREADCRUMB_SIZE * i,
                                     startx + x + BREADCRUMB_SIZE * (j + 1) - 1, starty+ y + BREADCRUMB_SIZE * (i + 1) - 1),
                                    BLACK)
        i = row
        j = col
        self.draw.rectangle((startx + x + BREADCRUMB_SIZE * j          , starty + y + BREADCRUMB_SIZE * i,
                             startx + x + BREADCRUMB_SIZE * (j + 1) - 1, starty+ y + BREADCRUMB_SIZE * (i + 1) - 1),
                            WHITE)
        segs = [(startx - 1, HEIGHT),
                (startx - 1, starty - 1),
                (startx + shape[0] * BREADCRUMB_SIZE, starty - 1)]
        for i, n_col in enumerate(shape):
            segs.append((startx + shape[i] * BREADCRUMB_SIZE, (i + 1) * BREADCRUMB_SIZE  + starty - 1))
            if i < len(shape) - 1:
                segs.append((startx + shape[i + 1] * BREADCRUMB_SIZE, (i + 1) * BREADCRUMB_SIZE  + starty - 1))
        self.draw.line(segs, fill=BLACK)

        ## draw outline
        self.draw.line
    def copy(self):
        return WIF(size=self.size, background=self.im)
        
    def show(self):
        self.im.show()
    
    def saveas(self, fn):
        if fn.upper().endswith(".WIF"):
            towif(self.im, fn, WIDTH, HEIGHT)
        else:
            self.im.save(fn)
        print 'wrote', fn

    def rotate180(self):
        self.im = self.im.rotate(180)

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


