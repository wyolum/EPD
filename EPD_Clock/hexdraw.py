import PIL.Image as Im
from numpy import *
import struct


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

v = 68
assert reverse_chr(ord(reverse_chr(v))) ==  chr(v)

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
    assert len(out) == 33, '%s != 33' % len(out)
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

f = open('unifont.hex')

def create_index():
    index = open('unifont.index', 'w')
    n = 0
    last = 0
    for l in f.readlines():
        unic = l[:4]
        dec = int(unic, 16)
        while last < dec - 1:
            last += 1
            print >> index, '%04x:ffffff' % (last,)
        print >> index, '%s:%06x' % (unic, n)
        n += len(l)
        last = dec

def check_index():
    w = 12
    f.seek(0)
    index = open('unifont.index').readlines()
    
    for i, l in enumerate(index):
        unic, offset = l.split(':')
        offset = int(offset, 16)
        f.seek(offset)
        if i != int(unic, 16):
            print 'off in line %x' % i
        # assert i == int(unic, 16), 'off in line %x' % i

def my_name():
    font = open('unifont.hex')
    index = open('unifont.index')
    for c in 'JUSTIN':
        index.seek(12 * ord(c))
        unic, i = index.read(12).split(':')
        i = int(i, 16)
        font.seek(i)
        dat = font.readline()
        unpack(dat)
    # check_index()

BLANK = '0' * 32
def create_indexed():
    import struct
    fn = 'unifont.wff'
    indexed = open(fn, 'wb')
    last = 0
    for l in f.readlines():
        l = l.strip()
        unic = l[:4]
        dec = int(unic, 16)
        data =  l[5:]
        while last < dec - 1:
            last += 1
            indexed.write(repack(BLANK))
        indexed.write(repack(data))
        last = dec
    print 'created', fn
REC_LEN = 33
def unrepack(f, i):
    f.seek(i * REC_LEN)
    dat = f.read(REC_LEN)
    n_byte = ord(dat[0])
    data = [ord(b) for b in dat[1:]]
    s = ''.join(['%02x' % d for d in data])
    return '%04x:%s' % (i, s)

def to_array(f, i):
    f.seek(i * REC_LEN)
    dat = f.read(REC_LEN)
    n_byte = ord(dat[0])
    w = 8 * (n_byte / 16)
    dat = dat[1:1 + n_byte]
    data = array([_bits(ord(b)) for b in dat])
    return reshape(data, (16, -1))

def paste_char(f, img, unic, x, y):
    dat = unrepack(f, unic)
    # unpack(dat)
    dat = to_array(f, unic)
    # print dat
    rgba = zeros((dat.shape[0], dat.shape[1], 4), 'uint8') 
    rgba += (1 - dat[:,:,newaxis]) * 255
    im = Im.fromarray(rgba)
    img.paste(im, (x, y))
    
def check_indexed():
    f = open('unifont.wff')
    bigIm = Im.new('RGBA', (260 * 16, 260 * 16), (255, 255, 255, 255))
    bigIm
    for j in range(256):
        paste_char(f, bigIm, ord('%x' % (j // 16)), (j + 2) * 16, 0)
        paste_char(f, bigIm, ord('%x' % (j % 16)), (j + 2) * 16, 16)
    for i in range(256):
        paste_char(f, bigIm, ord('%x' % (i // 16)), 0, (i + 2) * 16)
        paste_char(f, bigIm, ord('%x' % (i % 16)), 8, (i + 2) * 16)

    ## 79 -- 0-20
    for i in range(256):
        for j in range(256):
            try:
                paste_char(f, bigIm, i * 256 + j, (j + 1) * 16, (i + 2) * 16)
            except:
                break
            continue
            dat = to_array(f, i * 256 + j)
            # print dat
            rgba = zeros((16, 16, 4), 'uint8')
            rgba += (1 - dat[:,:,newaxis]) * 255
            im = Im.fromarray(rgba)
            bigIm.paste(im, (j*16, i*16))
    bigIm.show()
check_indexed()
        

# create_indexed()
# check_indexed()

import PIL.Image as Image
f = open('unifont.wff')
def toimg(i):
    dat = unrepack(f, i)
    unpack(dat)
    im = Image.Image('RGBA', (16, 16))


