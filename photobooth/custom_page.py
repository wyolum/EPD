import textwrap
import unifont
import shutil
import os.path
import glob
from PIL import Image
from PIL import ImageFont
from PIL import ImageDraw
# import qrcode
import csv
from wif import towif, WIF, PAGE, NAME_FONT_SIZE, NORMAL_FONT_SIZE, WIDTH, HEIGHT

WHITE = 255
QR_SIZE = 152

HEADSHOT_DIR = 'headshots/'
SCHEDULE_DIR = 'schedule/'
LOGO_DIR = 'logos/'
OUTPUT_DIR = 'ALBUM/'
CUSTOM_DIR = 'custom/'

HEADSHOT_WIDTH = 152
N_PAGE = 3

### Create schedule prior to counting pages
def create_sched(filename):
    dat = open(filename).readlines()
    x = 0
    y = 0
    page = WIF()
    page_no = 0
    for line in dat:
        line = line.strip()
        if line.startswith('<B>'):
            line = line[3:]
            bigascii = True
            wrap = 44
        else:
            bigascii = False
            wrap = 44
        for l in textwrap.wrap(line, wrap, subsequent_indent='  '):
            page.add_7x5_txt(l, x, y)
            y += 9
            if y > 176 - 16:
                page.im.save(SCHEDULE_DIR + chr(ord('A') + page_no) + '.png')
                page_no += 1
                page = WIF()
                y = 9
N_HEAD = 3
N_LOGO = 10
N_SCHED = 8
BREADCRUMB_SHAPE = [N_HEAD, 
                    N_LOGO,
                    N_SCHED
                    ]   ### 3 rows of images

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
        out = []
        if self.organizer[0].lower() == 'y':
            out.append('Organizer')
        if self.speaker[0].lower() == 'y':
            out.append('Speaker')
        if self.sponsor[0].lower() == 'y':
            out.append('Sponsor')
        return out
    roles = property(getRoles)
    def qr(self):
        try:
            return qrcode.make(str(self))
        except:
            raise ImportError("Please install qrcode to use this feature")

def wif_directory(dir, level, rotate=False):
    files = glob.glob(os.path.join(dir, '[A-Z].png'))
    files.sort()
    for i, img_fn in enumerate(files):
        page = WIF()
        fn = os.path.split(img_fn)[1]
        page.addImage(Image.open(img_fn), 0, 0)
        page.addBreadCrumb(0, 0, level, i, BREADCRUMB_SHAPE)
        if rotate:
            page.rotate180()
        page.saveas(OUTPUT_DIR + chr(ord('A') + level) + '/' + fn[0] + '.WIF')

def copy_dir(source, dest):
    if os.path.exists(dest):
        shutil.rmtree(dest)
        assert not os.path.exists(dest)
    shutil.copytree(source, dest)

NAME_W = 264
NAME_Y = 128
NAME_H = 48
def create_frontpage(sd, alb='ALBUM/A', filename='person.csv', headshot=None):
    ### Custom Page    
    assert os.path.exists(sd)
    dir = os.path.join(sd, alb)
    if not os.path.exists(dir):
        raise ValueError("Cannot find personal record")

    person = list(csv.reader(open(os.path.join(sd, filename))))
    header = person[0]
    person = person[1]

    person = Attendee(person, header)
    print person.name
    frontpage = WIF()
    ohs2013 = Image.open('ohs2013_thumb.jpg')
    frontpage.addImage(ohs2013, 5, 5)

    if headshot:
        print 'adding headshot', headshot
        print Image.open(headshot).size
        frontpage.addImage(Image.open(headshot), 112, 0)
    for i, role in enumerate(person.roles):
        frontpage.addUnifont(role, 5, 5 + (i + 1) * 16, bigascii=False)
    size = unifont.calcsize(person.name, bigascii=True)
    NAME_AREA = NAME_W, NAME_H
    scale = int(min([NAME_AREA[0] / float(size[0]), NAME_AREA[1] / float(size[1])]))
    image1 = Image.new('L', size, WHITE)
    size = size[0] * scale, size[1] * scale
    unifont.addText(person.name, image1, 0, 0, bigascii=True)
    image2 = image1.resize(size)

    frontpage.im.paste(image2, ((NAME_W - size[0]) / 2, 176 - 24 ), 0)
    frontpage.addBreadCrumb(0, 0, 0, 0, BREADCRUMB_SHAPE)
    frontpage.saveas(os.path.join(dir, 'A.WIF'))
    frontpage.show()
create_frontpage('.', alb='ALBUM/A', filename='person.csv', headshot='DEFAULT_HEADSHOT.png')
