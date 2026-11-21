"""Lossless word-column second-difference packing for bounded GPU diagnostic facts only.
Not an Earthcall save codec. Codex / GPT-6.1 Sol / Sixth-One Sun /
01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-28.
"""
from array import array
import struct
import sys
import zlib

MAGIC=b'ECFACT2\0'
MAX_BYTES=64*1024*1024


def need(ok,message):
    if not ok:raise ValueError(message)


def encode(raw):
    need(0<len(raw)<=MAX_BYTES and len(raw)%64==0,'bounded fact bytes')
    words=array('I');words.frombytes(raw)
    if sys.byteorder!='little':words.byteswap()
    n=len(words)//16;out=array('I',[0])*len(words)
    for k in range(16):
        previous=0;older=0
        for i in range(n):
            v=words[i*16+k];q=(v-2*previous+older)&0xffffffff
            signed=q if q<0x80000000 else q-0x100000000
            out[k*n+i]=((signed<<1)^(signed>>31))&0xffffffff
            older=previous;previous=v
    if sys.byteorder!='little':out.byteswap()
    return struct.pack('<8sII',MAGIC,16,n)+zlib.compress(out.tobytes(),6)


def decode(packed,expected_bytes):
    need(0<expected_bytes<=MAX_BYTES and expected_bytes%64==0,'bounded expected fact bytes')
    need(len(packed)>=16,'fact header')
    magic,columns,n=struct.unpack('<8sII',packed[:16])
    need(magic==MAGIC and columns==16 and n*64==expected_bytes,'fact header identity')
    decoder=zlib.decompressobj();data=decoder.decompress(packed[16:],expected_bytes+1)
    need(len(data)==expected_bytes and decoder.eof and not decoder.unused_data and not decoder.unconsumed_tail,'fact compressed extent')
    words=array('I');words.frombytes(data)
    if sys.byteorder!='little':words.byteswap()
    out=array('I',[0])*len(words)
    for k in range(16):
        previous=0;older=0
        for i in range(n):
            zig=words[k*n+i];q=(zig>>1)^-(zig&1)
            v=(q+2*previous-older)&0xffffffff;out[i*16+k]=v
            older=previous;previous=v
    if sys.byteorder!='little':out.byteswap()
    return out.tobytes()
