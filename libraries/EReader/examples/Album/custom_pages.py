import textwrap
import unifont
import shutil
import os
import os.path
import glob
from PIL import Image
from PIL import ImageFont
from PIL import ImageDraw
import qrcode
import csv
from wif import towif, WIF, PAGE, NAME_FONT_SIZE, NORMAL_FONT_SIZE, WIDTH, HEIGHT

QR_SIZE = 150

DEFAULT = "default.png"
BACKGROUND = "background.png"
HEADSHOT_DIR = 'headshots/'
SCHEDULE_DIR = 'schedule/'
LOGO_DIR = 'logos/'
SPONSOR_DIR = 'sponsors/'
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
        def closewif(page, page_no):
            page.im.save(SCHEDULE_DIR + chr(ord('A') + page_no) + '.png')
            page_no += 1
            page = WIF()
            return page, page_no, 0

        if line.startswith('<P>'): ## new page
            line = line[3:]
            if y > 0:
                page, page_no, y = closewif(page, page_no)

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
            if y > 176 - 9:
                page, page_no, y = closewif(page, page_no)
                
    closewif(page, page_no)
create_sched('schedule.txt')

#OHS2013
BREADCRUMB_SHAPE = [3, 
                    len(glob.glob(os.path.join(SPONSOR_DIR, '[A-Z].png'))),
                    len(glob.glob(os.path.join(SCHEDULE_DIR, '[A-Z].png'))),
                    ]   ### 3 rows of images: 3 images is first row, 5 in next, 3 in last
## Makerfest
BREADCRUMB_SHAPE = [3, 1]
class Attendee:
    def __init__(self, line, header):
        self.line = [cell.strip() for cell in line]
        self.header = [h.lower().strip() for h in header]
        self.line = self.line + [''] * (len(self.header) - len(self.line))
        self.dat = dict(zip(self.header, self.line))
        self.name = '%s %s' % (self.dat['attendee first name'],
                               self.dat['attendee last name'])
    def makecsv(self, fn):
        f = open(fn, 'w')
        writer = csv.writer(f)
        writer.writerows([self.header, self.line])
        print 'wrote', fn
        f.close()
        
    def printme(self):
        print self.line
        print self.first
    
    def __getattr__(self, name):
        return self.dat[name.lower()]

    def __str__(self):
        return '\n'.join([self.name, self.email, self.phone, self.website])

    def getHeadshot(self):
        
        if self.dat['headshot'] and os.path.exists(os.path.join(HEADSHOT_DIR, self.dat['headshot'])):
            out = self.dat['headshot']
        elif os.path.exists(os.path.join(HEADSHOT_DIR, '_'.join(self.name.lower().split()) + '.png')):
            out = '_'.join(self.name.lower().split()) + '.png'
        else:
            out = DEFAULT
        return out
    def getLogo(self):
        if self.dat['logo'] and os.path.exists(os.path.join(LOGO_DIR, self.dat['logo'])):
            out = self.dat['logo']
        elif self.dat['company'] and os.path.exists(os.path.join(LOGO_DIR, '_'.join(self.dat['company'].lower().split()) + '.png')):
            out = '_'.join(self.dat['company'].lower().split()) + '.png'
        else:
            out = DEFAULT
        return out

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
        return qrcode.make(str(self))

### Standard Pages
def wif_directory(dir, level, rotate=False):
    delete_all = glob.glob(os.path.join(dir, '[A-Z].WIF'))
    for file in delete_all:
        os.remove(file)
        print 'deleted', file
    files = glob.glob(os.path.join(dir, '[A-Z].png'))
    files.sort()
    for i, img_fn in enumerate(files):
        page = WIF()
        fn = os.path.split(img_fn)[1]
        page.addImage(Image.open(img_fn), 0, 0)
        page.addBreadCrumb(0, 0, level, i, BREADCRUMB_SHAPE)
        if rotate:
            page.rotate180()
        fn = OUTPUT_DIR + chr(ord('A') + level) + '/' + fn[0] + '.WIF'
        page.saveas(fn)
wif_directory(SPONSOR_DIR, 1)
# wif_directory(SCHEDULE_DIR, 2, rotate=True)

def copy_dir(source, dest):
    if os.path.exists(dest):
        shutil.rmtree(dest)
        assert not os.path.exists(dest)
    shutil.copytree(source, dest)

### Custom Pages    
FILENAME = 'attendees1.csv'
FILENAME = 'MakerFest_BADGEr_List.csv'
people = list(csv.reader(open(FILENAME)))
header = people[0]
people = people[1:]

if os.path.exists(CUSTOM_DIR):
    shutil.rmtree(CUSTOM_DIR)
os.mkdir(CUSTOM_DIR)

for person in people[:]:
    person = Attendee(person, header)
    print person.name, person.headshot, person.logo
    pages = [WIF() for i in range(N_PAGE)]
    
    qr = person.qr().resize((QR_SIZE, QR_SIZE))
    for page in pages:
        page.addImage(Image.open(os.path.join(BACKGROUND)), 0, 0)
    pages[0].addImage(Image.open(os.path.join(HEADSHOT_DIR, person.getHeadshot())), 112, 0)
    pages[1].addImage(qr, WIDTH - QR_SIZE, 0)
    pages[2].addImage(Image.open(os.path.join(LOGO_DIR,  person.getLogo())), 112, 0)
    for page in pages:
        for i, role in enumerate(person.roles):
            page.addUnifont(role[:14], 0, 0 + (i + 1) * 16 + 5, bigascii=False)
            # page.addUnifont(role, 0, 0 + i * 16 + 5, bigascii=False)
            # page.addText(role, 0, 10 + i * (NORMAL_FONT_SIZE + 5) + 5)
        page.addText(person.name, WIDTH/2, HEIGHT, font_size=NAME_FONT_SIZE, halign='center', valign='bottom')
    dir = CUSTOM_DIR + '%04d-%s/ALBUM/' % (int(person.id), '_'.join(person.name.split()))
    copy_dir(OUTPUT_DIR + 'B/', dir + 'B/')
    # copy_dir(OUTPUT_DIR + 'C/', dir + 'C/')
    if not(os.path.exists(dir)):
        os.mkdir(dir)
        dir = dir + 'A/'
        os.mkdir(dir)

    dir = os.path.join(dir, 'A/')
    if os.path.exists(dir):
        os.rmdir(dir)
    os.mkdir(dir)
    for i, page in enumerate(pages):
        page.addBreadCrumb(0, 0, 0, i, BREADCRUMB_SHAPE)
        page.saveas(os.path.join(dir, chr(ord('A') + i) + '.WIF'))
        page.saveas(os.path.join(dir, chr(ord('A') + i) + '.png'))
    fn = os.path.join(dir, 'person.csv')
    person.makecsv(fn)
    # page.show()
