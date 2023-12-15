import os

rootdir = "."
for root, dirs, files in os.walk(rootdir):
    for f in files:
        if(f.find(".bmp") != -1):
            filename = os.path.join(root, f)
            print("//" + f.split(".bmp")[0])
            fp = open(filename,"rb")

            a = fp.read()
            bytesarr = a[0x436:-2]

            for i in range(15,-1,-1):
                hexdata = 0x0000
                for j in range(0,16):
                    if(bytesarr[i * 16 + j] > 0):
                        hexdata += (0b1 << (15 - j))
                print('0x{:04X},'.format(hexdata))
            fp.close()