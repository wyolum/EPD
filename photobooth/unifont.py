import PIL.Image as Im
from numpy import *
import struct

REC_LEN = 33
BLANK = '0' * 32
UNIFONT_FN = 'unifont.wff'
UNIFONT_FILE = open(UNIFONT_FN)

def _bits(num):
    return [num >> i & 1 for i in range(8)]

def bits(byte):
    out = ''
    try:
        num = int(byte, 16)
    except ValueError:
        print '"%s"' % byte, 'HERE'
        raise
    for i in range(3, -1, -1):
        bit = (num >> i) & 1
        out += '-@'[bit]
    return out

def bitstr(bytes):
    return ''.join([bits(b) for b in bytes])

FILLER = chr(0) * 16
def reverse_chr(v):
    out = 0
    for i in range(8):
        out += ((v >> i) & 1) << (7 - i);
    return chr(out)

v = ord('v')
k = ord('k')
assert reverse_chr(ord(reverse_chr(v))) ==  chr(v)
assert not ord(reverse_chr(k)) ==  chr(k)
assert reverse_chr(ord(reverse_chr(k))) ==  chr(k)


def repack(s):
    '''
    return a struct packed string in 4-byte ints
    '''
    N = len(s)
    vals = []
    for i in range(0, N, 2):
        val = int(s[i:i+2], 16)
        vals.append(val)
    out = ''.join(map(reverse_chr, vals))
    if len(out) == 16:
        out = chr(len(out)) + out + FILLER ## pad short chars
    else:
        assert len(out) == 32, '%s is not 16 or 32?! %s\n    %s\n    %s' % (len(out), s, out, vals)
        out = chr(len(out)) + out
    assert len(out) == REC_LEN, '%s != %s' % (len(out), REC_LEN)
    return out

def unpack(s):
    unic, data = s.strip().split(':')
    unic = int(unic, 16)
    N = len(data)
    out = []
    if N == 32:
        w = 2
    else: 
        w = 4
    for i in range(16):
        l = data[i * w:(i + 1) * w]
        print '    ', bitstr(l)
    print

HEX_FILE = open('unifont.hex')

def create_index(filename='unifont.index'):
    index = open(filename, 'w')
    n = 0
    last = 0
    for l in HEX_FILE.readlines():
        unic = l[:4]
        dec = int(unic, 16)
        while last < dec - 1:
            last += 1
            print >> index, '%04x:ffffff' % (last,)
        print >> index, '%s:%06x' % (unic, n)
        n += len(l)
        last = dec

def check_index(f, wff_filename='unifont.index'):
    w = 12
    HEX_FILE.seek(0)
    index = open(wff_filename).readlines()
    
    for i, l in enumerate(index):
        unic, offset = l.split(':')
        offset = int(offset, 16)
        HEX_FILE.seek(offset)
        if i != int(unic, 16):
            print 'off in line %x' % i
        # assert i == int(unic, 16), 'off in line %x' % i


def create_wff(out_filename=UNIFONT_FN):
    '''
    create a font file with each char the same taking up the same size: REC_LEN byes.
    '''
    import struct
    wff = open(out_filename, 'wb')
    last = 0
    HEX_FILE.seek(0)
    for l in HEX_FILE.readlines():
        l = l.strip()
        unic = l[:4]
        dec = int(unic, 16)
        data =  l[5:]
        while last < dec - 1: ## write blanks for all skipped chars
            last += 1
            wff.write(repack(BLANK))
        wff.write(repack(data))
        last = dec
    print 'created', out_filename
# create_wff() ##only need to call once

def wff_to_hex(wff_file, idx):
    '''
    convert a wff char back into hex format
    '''
    wff_file.seek(idx * REC_LEN)
    dat = wff_file.read(REC_LEN)
    n_byte = ord(dat[0])
    data = [ord(b) for b in dat[1:]]
    s = ''.join(['%02x' % d for d in data])
    return '%04x:%s' % (idx, s)

def to_array(wff_file, idx):
    '''
    read charcter at given index into an array
    '''
    wff_file.seek(idx * REC_LEN)
    dat = wff_file.read(REC_LEN)
    n_byte = ord(dat[0])
    # width = 8 * (n_byte / 16) ### not used
    dat = dat[1:1 + n_byte]
    data = array([_bits(ord(b)) for b in dat])
    return reshape(data, (16, -1))

def paste_char(wff_file, img, unic, x, y):
    '''
    paste a bitmap of char at unic indix at pos x, y into img
    '''
    dat = to_array(wff_file, unic)
    ones = zeros((dat.shape[0], dat.shape[1]), 'uint8')
    ones += (1 - dat) * 255
    im = Im.fromarray(ones)
    img.paste(im, (x, y))
    return dat.shape[1]

def check_wff(wff_filename=UNIFONT_FN):
    wff_file = open(wff_filename)
    # bigIm = Im.new('RGBA', (260 * 16, 260 * 16), (255, 255, 255, 255)) ## was
    bigIm = Im.new('1', (260 * 16, 260 * 16), 255)
    for j in range(256):
        paste_char(wff_file, bigIm, ord('%x' % (j // 16)), (j + 2) * 16, 0)
        paste_char(wff_file, bigIm, ord('%x' % (j % 16)), (j + 2) * 16, 16)
    for i in range(256):
        paste_char(wff_file, bigIm, ord('%x' % (i // 16)), 0, (i + 2) * 16)
        paste_char(wff_file, bigIm, ord('%x' % (i % 16)), 8, (i + 2) * 16)

    ## 79 -- 0-20
    for i in range(256):
        for j in range(256):
            try:
                paste_char(wff_file, bigIm, i * 256 + j, (j + 1) * 16, (i + 2) * 16)
            except:
                break
    # bigIm.show()
    bigIm.save('unifont.jpg')
    print 'wrote unifont.jpg'
# check_wff() ## only call once
BIGTEXT_OFFSET = 0xfee0
def calcsize(text, bigascii=False, wff_file=UNIFONT_FILE):
    if bigascii:
        utxt = ''.join([unichr(ord(c) + BIGTEXT_OFFSET) for c in text])
        x, y = calcsize(utxt, bigascii=False, wff_file=wff_file)
    else:
        x = 0
        y = 16
        for unic in text:
            dat = to_array(wff_file, ord(unic))
            x += dat.shape[1]
    return x, y

LEFT = 0
CENTER = 1
RIGHT = 2
def addText(text, im, x, y, wff_file=UNIFONT_FILE, bigascii=False):
    '''
    paste an image of text into im at position x, y.
    if bigascii == True, then use large chars near end of UNIFONT file
    '''
    if bigascii:
        utxt = ''.join([unichr(ord(c) + BIGTEXT_OFFSET) for c in text])
        addText(utxt, im, x, y, bigascii=False, wff_file=wff_file)
    else:
        for unic in text:
            x += paste_char(wff_file, im, ord(unic), x, y)

def addText__test__():
    wff_file = open(UNIFONT_FN)
    im = Im.new('1', (264, 175), 255)
    addText('JUSTIN', im, 0, 0, wff_file)
    txt = 'Justin'
    addText(txt, im, 0, 20, bigascii=True, wff_file=UNIFONT_FILE)
    im.show()
# addText__test__()
