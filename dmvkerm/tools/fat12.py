import struct
class Fat12:
    def __init__(s,img):
        s.d=bytearray(img); b=s.d
        s.bps,s.spc,s.res,s.nf,s.nr,s.tot,s.med,s.spf=struct.unpack('<HBHBHHBH',b[11:24])
        s.rs=s.res+s.nf*s.spf; s.ds=s.rs+s.nr*32//s.bps
        s.ncl=(s.tot-s.ds)//s.spc+2
        s.fat=bytearray(b[s.res*s.bps:(s.res+s.spf)*s.bps])
    def get(s,c):
        v=s.fat[c*3//2]|(s.fat[c*3//2+1]<<8); return (v>>4) if c&1 else v&0xfff
    def set(s,c,x):
        o=c*3//2; v=s.fat[o]|(s.fat[o+1]<<8)
        v=(v&0x000f)|(x<<4) if c&1 else (v&0xf000)|x
        s.fat[o]=v&0xff; s.fat[o+1]=v>>8
    def entry(s,i): o=s.rs*s.bps+i*32; return o
    def find(s,name):
        for i in range(s.nr):
            o=s.entry(i)
            if s.d[o]==0: return None
            if s.d[o:o+11]==name: return i
    def delete(s,name):
        i=s.find(name); o=s.entry(i); c=struct.unpack('<H',s.d[o+26:o+28])[0]
        while 2<=c<0xff0: n=s.get(c); s.set(c,0); c=n
        s.d[o]=0xE5
    def add(s,name,data,date=(1988,5,4),attr=0x20,zeit=(0,0,0)):
        assert len(name)==11,('Dateiname muss 11 Zeichen haben',name)
        need=max(1,-(-len(data)//(s.spc*s.bps))); free=[c for c in range(2,s.ncl) if s.get(c)==0][:need]
        assert len(free)==need,'Diskette voll'
        for a,b in zip(free,free[1:]+[0xfff]): s.set(a,b)
        for k,c in enumerate(free):
            o=(s.ds+(c-2)*s.spc)*s.bps; chunk=data[k*s.spc*s.bps:(k+1)*s.spc*s.bps]
            s.d[o:o+len(chunk)]=chunk
        for i in range(s.nr):
            o=s.entry(i)
            if s.d[o] in (0,0xE5): break
        y,m,dd=date
        s.d[o:o+32]=name+bytes([attr])+b'\0'*10+struct.pack('<HHHI',(zeit[0]<<11)|(zeit[1]<<5)|(zeit[2]//2),((y-1980)<<9)|(m<<5)|dd,free[0],len(data))
    def image(s):
        for k in range(s.nf):
            o=(s.res+k*s.spf)*s.bps; s.d[o:o+len(s.fat)]=s.fat
        return bytes(s.d)
