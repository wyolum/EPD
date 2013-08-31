import zipfile

def zipit(manifest, filename):
    out = zipfile.ZipFile(filename, "w")
    files = open('files.txt').readlines()
    for l in files:
        l = l.strip()
        out.write(l)
        print 'zipped', l
    out.close()
    print 'wrote', filename

zipit('files.txt', 'wifit.0.0.3.zip')
        
    
