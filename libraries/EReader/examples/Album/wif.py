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
        unifont.addText(txt, self.wff_file, self.im, x, y, **kw)

    def add_7x5_txt(self, txt, x, y, **kw):
        ascii_5x7.addText(txt, self.im, x, y, **kw)
        
    def addText(self, txt, x, y, 
                font_size=NORMAL_FONT_SIZE, 
                font=FONT_NAME, 
                font_style=FONT_STYLE, 
                color=BLACK,
                valign='top',
                halign='left'):
        if font_style:
            ttf = '%s%s-%s.ttf' % (FONT_DIR, font, font_style)
        else:
            ttf = '%s%s.ttf' % (FONT_DIR, font)
        font = ImageFont.truetype(ttf, font_size)
        text_size = self.draw.textsize(txt, font)
        if text_size[0] > WIDTH:
            txt = txt.split()
            txt = txt[0] + ' ' + txt[-1][0] + '.'
            text_size = self.draw.textsize(txt, font)
        if valign == 'top':
            pass
        elif valign == 'center':
            y -= text_size[1] // 2
        elif valign == 'bottom':
            y -= text_size[1]
        else:
            raise ValueError('Unknown option for valign:"%s"' % valgin)
        if halign == 'left':
            pass
        elif halign == 'center':
            x -= text_size[0] // 2
        elif halign == 'right':
            x -= text_size[0]
        else:
            raise ValueError('Unknown option for halign:"%s"' % halgin)
            
        self.draw.text((x, y), txt, color, font=font)
    
    def addBreadCrumb(self, x, y, row, col, shape, color=BLACK):
        for i, n_col in enumerate(shape):
            for j in range(n_col):
                if i == row and j == col:
                    self.draw.rectangle((x + BREADCRUMB_SIZE * j + 1, y + (BREADCRUMB_SIZE) * i,
                                         x + BREADCRUMB_SIZE * (j + 1), y + BREADCRUMB_SIZE * (i + 1) - 1),
                                        WHITE)
                else:
                    self.draw.rectangle((x + (BREADCRUMB_SIZE + 0) * j, y + BREADCRUMB_SIZE * i,
                                         x + (BREADCRUMB_SIZE + 0) * (j + 1), y + BREADCRUMB_SIZE * (i + 1)),
                                        BLACK)
        
    def copy(self):
        return WIF(size=self.size, background=self.im)
        
    def show(self):
        self.im.show()
    
    def saveas(self, fn):
        towif(self.im, fn, WIDTH, HEIGHT)
        print 'wrote', fn

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


