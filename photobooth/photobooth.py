import time
import sys
import os
import os.path
from custom_page import *
import getpass
import glob
import subprocess
import serial
import ImageEnhance

## change this based on your sd card
DEVICE_ID = '9016-4EF8'

CONTRAST = 1.  ## 0.. 5 float
BRIGHT = 1.    ## -5..5 float

def equalize(image1, contrast, brightness):
    contr = ImageEnhance.Contrast(image1)
    image = contr.enhance(contrast)
    bright = ImageEnhance.Brightness(image)
    image = bright.enhance(brightness)
    image = image.resize((int(image.size[0]), 
                          int(image.size[1])))
    out = image.convert('1')
    return out


def ispi():
    return getpass.getuser() == 'pi'

if ispi():
    SD_PATH = '/media/%s/' % DEVICE_ID
else:
    SD_PATH = '/media/9016-4EF8/'
        
ALBUM = 'ALBUM'
ARCHIVE = 'photos'

START_X = 175
START_Y = 200
W = 274
H = 174

HEADSHOT_WIDTH = 152
HEADSHOT_HEIGHT = 128
PAGE_BB = (START_X, START_Y, START_X + W, START_Y + H)
HEADSHOT_BB = (START_X, START_Y, START_X + HEADSHOT_WIDTH, START_Y + HEADSHOT_HEIGHT)

## ck if sd is present
def sd_present():
    return os.path.exists(SD_PATH)

def sd_umount():
    if sd_present():
        subprocess.call(['umount', SD_PATH])


def next_filename():
    if not os.path.exists(ARCHIVE):
        os.mkdir(ARCHIVE)
    n = len(glob.glob(os.path.join(ARCHIVE, '*.png')))
    return os.path.join(ARCHIVE, '%04d.png' % n)

def pi_snap():
    img_filename = next_filename()
    print img_filename
    cmd = 'raspistill -o ' + img_filename + ' -w '+ str(HEADSHOT_WIDTH) + \
          ' -h ' + str(HEADSHOT_HEIGHT)
    subprocess.call(cmd, shell=True)
    ### need to resize prolly
    return Image.open(img_filename)
    
cam = None
def laptop_snap():
    global cam
    import pygame.camera
    if cam is None:
        pygame.camera.init()
        cam = pygame.camera.Camera(pygame.camera.list_cameras()[0])
    cam.start()
    
    img = cam.get_image()
    imgstr = pygame.image.tostring(img, 'RGB')
    im = Image.fromstring('RGB', img.get_size(), imgstr)
    cam.stop()
    return im

def snap(filename="photo.png", bb=HEADSHOT_BB):
    if ispi():
        im = pi_snap()
    else:
        im = laptop_snap()
#    im = im.crop(bb)
    im = equalize(im, CONTRAST, BRIGHT)
    im.save(filename)
    return create_frontpage(SD_PATH, ALBUM, "person.csv", headshot=filename)

def findser():
    if ispi():
        ser = serial.Serial('/dev/ttyS0',19200, timeout=.1)
        print 'using AlaMode'
    else:
        if os.path.exists("/dev/ttyUSB0"):
            ser = serial.Serial("/dev/ttyUSB0", 19200, timeout=1)
        elif os.path.exists("/dev/ttyACM0"):
            ser = serial.Serial("/dev/ttyACM0", 19200, timeout=1)
        else:
            ser = sys.stdin
    return ser

def loop():
    if not sd_present():
        print 'insert SD card'
    while not sd_present():
        time.sleep(1)
    print 'Press button when ready'

    ser = findser()
    command = ser.readline()
    if ('snap' in command):
        snap()
    else:
        print 'command garbled:', command, ';' 
    time.sleep(.1)
    sd_umount()

def main():
    while 1:
        loop()


if __name__ == "__main__":
    main()

