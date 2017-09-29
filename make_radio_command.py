f = open("../vbrf-transmitter/src/RadioInterface.h").read()

st = f[f.find('enum RadioConfig'):]
st = st[st.find(' {')+2:]
st = st[:st.find('}')-1]
st = map(lambda x: x.lstrip().rstrip(),st.split(','))
dic = {v: k for (k,v) in enumerate(st)}

import sys, struct
lines = sys.stdin.readlines()
l = []
l.append(len(lines))
fmt = "<b"

for cmd in lines:
    a, b = cmd.rstrip().lstrip().split(',')
    fmt += "bhh"
    l.extend([dic[a], int(b), 0])

print struct.pack(fmt, *l).encode('hex')
