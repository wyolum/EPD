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
        if 'name' not in self.dat:
            self.name = '%s %s' % (self.dat['attendee first name'],
                                   self.dat['attendee last name'])
        
    def printme(self):
        print self.line
        print self.first
    
    def __getattr__(self, name):
        return self.dat[name.lower()]

    def __str__(self):
        return '\n'.join([self.name, self.email, self.phone, self.website])
    
    def getRoles(self):
        out = []
        if self.organizer:
            out.append('Organizer')
        if self.speaker:
            out.append('Speaker')
        if self.sponsor:
            out.append('Sponsor')
        if self.volunteer:
            out.append('Volunteer')
        if self.demo :
            out.append('Demo')
        if self.poster:
            out.append('Poster')
        if self.press:
            out.append('Press')
        if self.company:
            out.append(self.company)
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
def create_frontpage(sd, alb='ALBUM/A', filename=None, headshot=None, 
                     headshot_bbox=None):
    assert os.path.exists(sd)
    photo_only = False

    frontpage = WIF()

    if headshot is None or not os.path.exists(headshot):
        headshot = 'DEFAULT_HEADSHOT.png'
    ### Custom Page    
    dir = os.path.join(sd, alb)
    image_dir = os.path.join(dir, 'A')

    if not os.path.exists(dir):
        print "%s not present" % alb
        dir = '.'
    if filename is None:
        photo_only = True
    else:
        filename = os.path.join(dir, 'A', filename)
        if not os.path.exists(filename):
            photo_only = True
            print "Cannot find personal record:", filename
    if headshot:
        print 'adding headshot "%s"' % headshot
        head = Image.open(headshot)
        print "head.size", head.size
        if headshot_bbox is None:
            if photo_only:
                headshot_bbox = ((WIDTH - head.size[0])/2,
                                 (HEIGHT - head.size[1])/2, 
                                 (WIDTH + head.size[0])/2,
                                 (HEIGHT + head.size[1])/2
                                 )
            else:
                headshot_bbox = (112, 0, 264, 128)

        frontpage.addImage(head, 
                           headshot_bbox[0], headshot_bbox[1])
    if not photo_only:
        person = list(csv.reader(open(filename)))
        header = person[0]
        person = person[1]

        person = Attendee(person, header)
        print person.name
        ohs2013 = Image.open('ohs2013_thumb.jpg')
        frontpage.addImage(ohs2013, 5, 5)

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
    frontpage.saveas(os.path.join(image_dir, 'A.WIF'))
    frontpage.saveas(os.path.join(image_dir, 'A.png'))
    # frontpage.show()
    return frontpage
# create_frontpage('.', alb='ALBUM/A', filename='person.csv', headshot='DEFAULT_HEADSHOT.png')
# create_frontpage('.', headshot='DEFAULT_HEADSHOT.png')                )
