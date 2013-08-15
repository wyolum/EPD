import textwrap
import unifont
import shutil
import os.path
import glob
from PIL import Image
from PIL import ImageFont
from PIL import ImageDraw
import qrcode
import csv
from wif import towif, WIF, PAGE, NAME_FONT_SIZE, NORMAL_FONT_SIZE, WIDTH, HEIGHT

QR_SIZE = 150

HEADSHOT_DIR = 'headshots/'
SCHEDULE_DIR = 'schedule/'
LOGO_DIR = 'logos/'
OUTPUT_DIR = 'ALBUM/'
CUSTOM_DIR = 'custom/'

HEADSHOT_WIDTH = 150
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
            # page.addUnifont(l, x, y, bigascii=bigascii)
            page.add_7x5_txt(l, x, y)
            y += 9
            if y > 176 - 16:
                page.im.save(SCHEDULE_DIR + chr(ord('A') + page_no) + '.png')
                page_no += 1
                page = WIF()
                y = 9
create_sched('schedule.txt')
BREADCRUMB_SHAPE = [3, 
                    len(glob.glob(LOGO_DIR + '[A-Z].png')),
                    len(glob.glob(SCHEDULE_DIR + '[A-Z].png')),
                    ]   ### 3 rows of images: 3 images is first row, 5 in next, 3 in last

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
        out = ['Summiter']
        if self.organizer[0].lower() == 'y':
            out.append('Organizer')
        if self.speaker[0].lower() == 'y':
            out.append('Speaker')
        if self.sponsor[0].lower() == 'y':
            out.append('Sponsor')
        return out
    roles = property(getRoles)
    def qr(self):
        return qrcode.make(str(self))

create_sched('schedule.txt')
### Standard Pages
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
wif_directory(LOGO_DIR, 1)
wif_directory(SCHEDULE_DIR, 2, rotate=True)

def copy_dir(source, dest):
    if os.path.exists(dest):
        shutil.rmtree(dest)
        assert not os.path.exists(dest)
    shutil.copytree(source, dest)

### Custom Pages    
FILENAME = 'summiters.csv'
people = list(csv.reader(open(FILENAME)))
header = people[0]
people = people[1:]

shutil.rmtree(CUSTOM_DIR)
os.mkdir(CUSTOM_DIR)

for person in people[:]:
    person = Attendee(person, header)
    pages = [WIF() for i in range(N_PAGE)]
    
    qr = person.qr().resize((QR_SIZE, QR_SIZE))
    pages[0].addImage(Image.open(HEADSHOT_DIR + person.headshot), 0, 0)
    pages[1].addImage(qr, WIDTH - QR_SIZE, 0)
    pages[2].addImage(Image.open(LOGO_DIR + person.logo), 0, 0)
    for page in pages:
        for i, role in enumerate(person.roles):
            if len(role) <= 8:
                page.addUnifont(role, 0, 0 + i * 16 + 5, bigascii=True)
            else:
                page.addUnifont(role, 0, 0 + i * 16 + 5, bigascii=False)
            # page.addUnifont(role, 0, 0 + i * 16 + 5, bigascii=False)
            # page.addText(role, 0, 10 + i * (NORMAL_FONT_SIZE + 5) + 5)
        page.addText(person.name, WIDTH/2, HEIGHT, font_size=NAME_FONT_SIZE, halign='center', valign='bottom')
    # pages[0].show()

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
        page.addBreadCrumb(0, 0, 0, i, BREADCRUMB_SHAPE)
        page.saveas(dir + chr(ord('A') + i) + '.WIF')
    # page.show()
