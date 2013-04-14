import glob
import struct
import pylab
import numpy
import re

def fromxbm(fn):
    '''
    return byte array of xmb file stored in fn
    '''
    base = fn[:-4]
    wh = re.compile(b"\s*#define[ \t]+%s*_width[ \t]+(?P<width>[0-9]+)[\r\n]+"
                    b"\s*#define[ \t]+%s*_height[ \t]+(?P<height>[0-9]+)[\r\n]+" % (base, base))
    byte_matcher = re.compile('0x([0-9a-fA-F]{2})')
    f = open(fn);
    line1 = f.readline()
    line2 = f.readline()
    wh = wh.match('\n'.join([line1, line2]))
    width, height = map(int, wh.groups())
    print fn, width, height
    bytes_per_line = width / 8    
    out = numpy.zeros((height, bytes_per_line), numpy.ubyte)
    idx = 0
    for m in byte_matcher.finditer(f.read()):
        dat = m.group(1)
        val = int(dat, 16)
        i, j = divmod(idx, bytes_per_line)
        out[i, j] = val
        idx += 1
    return out

def fromdat(fn):
    f = open(fn)
    height, width = struct.unpack('HH', f.read(4))
    bytes_per_line = width / 8    
    out = numpy.zeros((height, bytes_per_line), numpy.ubyte)
    dat = f.read()
    n_byte = bytes_per_line * height
    assert len(dat) == n_byte, 'bad dat file for image'
    dat = struct.unpack('%dB' % n_byte, dat)
    return numpy.array(dat).reshape(height, bytes_per_line)

def display(bytes):
    h, bpl = bytes.shape
    w = bpl * 8
    bits = numpy.zeros((h, w), bool)
    for i, line in enumerate(bytes):
        for j, b in enumerate(line):
            for k in range(8):
                bits[i,j * 8 + k] = (b >> k) & 1
    pylab.pcolormesh(bits)
def save(bytes, fn):
    h, bpl = bytes.shape
    w = 8 * bpl;
    f = open(fn, 'w')
    f.write(struct.pack('HH', h, w))
    for line in bytes:
        f.write(struct.pack('%dB' % len(line), *line))

xbm = 'venus_2_0.xbm'
for xbm in glob.glob("*.xbm"):
    print xbm
    dat = xbm[:-3] + 'dat'
    fig = fromxbm(xbm)
    # pylab.figure(1); pylab.axis('equal')
    save(fig, dat)
    print 'wrote', dat
    fig2 = fromdat(dat)
    assert numpy.max(numpy.abs(fig - fig2)) == 0

