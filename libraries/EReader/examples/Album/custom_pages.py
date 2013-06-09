import shutil
import os.path
import glob
from PIL import Image
from PIL import ImageFont
from PIL import ImageDraw
import qrcode
import csv
from wif import towif

QR_SIZE = 150
PAGE = 264, 176
WIDTH = PAGE[0]
HEIGHT = PAGE[1]
FONT_NAME = 'Orbitron' # 'Ovo' # 'Ubuntu'
FONT_STYLE = 'Regular'
FONT_DIR = '/home/justin/Dropbox/WyoLumCode/fonts/'
NAME_FONT_SIZE = 35
NORMAL_FONT_SIZE = 20


HEADSHOT_DIR = 'headshots/'
SCHEDULE_DIR = 'schedule/'
LOGO_DIR = 'logos/'
OUTPUT_DIR = 'ALBUM/'
CUSTOM_DIR = 'custom/'

HEADSHOT_WIDTH = 150
WHITE = 255
BLACK = 0
BREADCRUMB_SIZE = 3
N_PAGE = 3

BREADCRUMB_SHAPE = [3, 
                    len(glob.glob(LOGO_DIR + '[A-Z].png')),
                    len(glob.glob(SCHEDULE_DIR + '[A-Z].png')),
                    ]   ### 3 rows of images: 3 images is first row, 5 in next, 3 in last

def front_page(who):
    print who

class Attendee:
    def __init__(self, line, header):
        self.line = [cell.strip() for cell in line]
        self.header = [h.lower().strip() for h in header]
        self.dat = dict(zip(self.header, self.line))
        
    def printme(self):
        print self.line
        print self.first
    
    def __getattr__(self, name):
        return self.dat[name.lower()]

    def __str__(self):
        return '\n'.join([self.name, self.email, self.phone, self.website])
    
    def getRoles(self):
        out = ['Participant']
        if self.speaker[0].lower() == 'y':
            out.append('Speaker')
        if self.sponsor[0].lower() == 'y':
            out.append('Sponsor')
        return out
    roles = property(getRoles)
    def qr(self):
        return qrcode.make(str(self))

class WIF:
    def __init__(self, size=PAGE, background=None):
        self.size = size
        self.im = Image.new('1', size, WHITE)
        self.draw = ImageDraw.Draw(self.im)
        if background:
            self.im.paste(background)

    def addImage(self, img, x, y):
        box = (x, y, x + img.size[0], y + img.size[1])
        self.im.paste(img, box)
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
    
    def addBreadCrumb(self, x, y, row, col, shape=BREADCRUMB_SHAPE, color=BLACK):
        for i, n_col in enumerate(shape):
            for j in range(n_col):
                if i == row and j == col:
                    self.draw.rectangle((x + BREADCRUMB_SIZE * j + 1, y + BREADCRUMB_SIZE * i,
                                         x + BREADCRUMB_SIZE * (j + 1), y + BREADCRUMB_SIZE * (i + 1) - 1),
                                        WHITE)
                else:
                    self.draw.rectangle((x + BREADCRUMB_SIZE * j, y + BREADCRUMB_SIZE * i,
                                         x + BREADCRUMB_SIZE * (j + 1), y + BREADCRUMB_SIZE * (i + 1)),
                                        BLACK)
        
    def copy(self):
        return WIF(size=self.size, background=self.im)
        
    def show(self):
        self.im.show()
    
    def saveas(self, fn):
        towif(self.im, fn, WIDTH, HEIGHT)
        print 'wrote', fn

### Standard Pages
def wif_directory(dir, level):
    files = glob.glob(os.path.join(dir, '[A-Z].png'))
    files.sort()
    for i, img_fn in enumerate(files):
        page = WIF()
        fn = os.path.split(img_fn)[1]
        page.addImage(Image.open(img_fn), 0, 0)
        page.addBreadCrumb(0, 0, level, i)
        page.saveas(OUTPUT_DIR + chr(ord('A') + level) + '/' + fn[0] + '.WIF')

wif_directory(LOGO_DIR, 1)
wif_directory(SCHEDULE_DIR, 2)

def copy_dir(source, dest):
    if os.path.exists(dest):
        shutil.rmtree(dest)
        assert not os.path.exists(dest)
    shutil.copytree(source, dest)

### Custom Pages    
FILENAME = 'attendees.csv'
people = list(csv.reader(open(FILENAME)))
header = people[0]
people = people[1:]

shutil.rmtree(CUSTOM_DIR)
os.mkdir(CUSTOM_DIR)

for person in people[1:2]:
    person = Attendee(person, header)
    pages = [WIF() for i in range(N_PAGE)]
    
    qr = person.qr().resize((QR_SIZE, QR_SIZE))
    pages[0].addImage(Image.open(HEADSHOT_DIR + person.headshot), 0, 0)
    pages[1].addImage(qr, WIDTH - QR_SIZE, 0)
    pages[2].addImage(Image.open(LOGO_DIR + person.logo), 0, 0)
    for page in pages:
        for i, role in enumerate(person.roles):
            page.addText(role, 0, 10 + i * (NORMAL_FONT_SIZE + 5) + 5)
        page.addText(person.name, WIDTH/2, HEIGHT, font_size=NAME_FONT_SIZE, halign='center', valign='bottom')
    

    dir = CUSTOM_DIR + '%04d-%s/ALBUM/' % (int(person.id), '_'.join(person.name.split()))
    copy_dir(OUTPUT_DIR + 'B/', dir + 'B/')
    copy_dir(OUTPUT_DIR + 'C/', dir + 'C/')
    if not(os.path.exists(dir)):
        os.mkdir(dir)
        dir = dir + 'A/'
        os.mkdir(dir)

    dir = dir + 'A/'
    if os.path.exists(dir):
        os.rmdir(dir)
    os.mkdir(dir)
    for i, page in enumerate(pages):
        page.addBreadCrumb(0, 0, 0, i)
        page.saveas(dir + chr(ord('A') + i) + '.WIF')
        page.show()
    here
