import PIL
from PIL import ImageFont
from PIL import Image
from PIL import ImageDraw

ttf = '/home/justin/Dropbox/WyoLumCode/fonts/Ubuntu-Light.ttf'
ttf = '/home/justin/Dropbox/WyoLumCode/fonts/Ubuntu-Regular.ttf'
font = ImageFont.truetype(ttf, 25)
img=Image.new("1", (264,176),255)
draw = ImageDraw.Draw(img)
draw.text((0, 0),"This is a test",0,font=font)
draw = ImageDraw.Draw(img)
img.show("a_test.png")
