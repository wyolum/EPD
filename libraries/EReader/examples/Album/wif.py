import struct

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


