import os
import os.path
from custom_page import *
import getpass
import glob
import subprocess

SD_PATH = '/media/usb'
SD_PATH = '.' 
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

def ispi():
    return getpass.getuser() == 'pi'

def next_filename():
    if not os.path.exists(ARCHIVE):
        os.mkdir(ARCHIVE)
    n = len(glob.glob(os.path.join(ARCHIVE, '*.png')))
    return os.path.join(ARCHIVE, '%04d.png' % n)

def pi_snap():
    img_filename = next_filename()
    subprocess.call(["raspistill", '-o', img_filename])
    ### need to resize prolly
    return Image.open(img_filename)
    
def laptop_snap():
    import pygame.camera
    pygame.camera.init()
    cam = pygame.camera.Camera(pygame.camera.list_cameras()[0])
    cam.start()
    img = cam.get_image()
    imgstr = pygame.image.tostring(img, 'RGB')
    im = Image.fromstring('RGB', img.get_size(), imgstr)
    return im

def snap(filename="photo.png", bb=HEADSHOT_BB):
    if ispi():
        im = pi_snap()
    else:
        im = laptop_snap()
    im = im.crop(bb)
    im.save(filename)
    create_frontpage(SD_PATH, ALBUM, "person.csv", headshot=filename)

snap()
