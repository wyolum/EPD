from photobooth import *
from Tkinter import *
import ImageTk

WIDTH = 1280
HEIGHT = 800
SCALE = 4

root = Tk()
Button_enabled = False

def quit():
    root.destroy()

import signal
TIMEOUT = .3 # number of seconds your want for timeout

def interrupted(signum, frame):
    "called when read times out"
    print 'interrupted!'
signal.signal(signal.SIGALRM, interrupted)

def check_and_snap():
    global im, wiftk, Button_enabled

    can.delete("text")
    if not sd_present():
        tid = can.create_text(WIDTH/2, HEIGHT - 85, text="Insert SD card", font=("times", 50), tags="text")
    else:
        tid = can.create_text(WIDTH/2, HEIGHT - 85, text="Press button when ready", font=("times", 50), tags="text")
        can.update()
        if (Button_enabled == False):
            ser.write('e') #enable button
            Button_enabled = True
        command = ser.readline().strip()
        if command == "snap":
            can.delete("text")
            tid = can.create_text(WIDTH/2, HEIGHT - 85, text="Processing Image", font=("times", 50), tags="text")
            can.update()
            Button_enabled = False
            im = snap().im
            x, y = im.size
            x *= SCALE
            y *= SCALE
            im = im.resize((x, y))
            # im.show()
            wiftk = ImageTk.PhotoImage(im)
            
            can.delete("image")
            can.create_image([(WIDTH + x) / 2 - x/2,
                              0 + y / 2], 
                             image=wiftk, 
                             tags="image")
            sd_umount()
            # print 'TODO: umount sd card'
        else:
            print command
    root.after(100, check_and_snap)
            
        

ser = findser()

w, h = root.winfo_screenwidth(), root.winfo_screenheight()
# root.overrideredirect(1)
root.geometry("%dx%d+0+0" % (WIDTH, HEIGHT))
root.focus_set() # <-- move focus to this widget
frame = Frame(root)
#Button(frame, text="Exit", command=quit).pack(side=LEFT)
frame.pack()
can = Canvas(root, width=WIDTH, height=HEIGHT)
can.pack()
root.after(100, check_and_snap)
root.wm_title("Wyolum Photobooth")
root.mainloop()
