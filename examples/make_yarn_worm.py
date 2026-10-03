#!/usr/bin/env python3
"""Writes yarn_worm.gr2: a small skinned, animated test model.

The model is generated here from scratch (no third-party art), so the output is
free to redistribute. It is a striped tube on a 6-bone chain with a looping
wiggle animation, stored as an uncompressed little-endian 32-bit .gr2 with its
own type definitions, the same way exporter-written files are laid out.

    python3 examples/make_yarn_worm.py examples/yarn_worm.gr2
"""
import math
import struct
import sys

END, INLINE, REF, REF_ARRAY, ARRAY_REFS, VARIANT, VARIANT_ARRAY, STRING, TRANSFORM = 0, 1, 2, 3, 4, 5, 7, 8, 9
REAL32, UINT8, NORMAL_UINT8, INT32 = 10, 12, 14, 19
P = 4

SCALAR = {REAL32: ('<f', 4), UINT8: ('<B', 1), NORMAL_UINT8: ('<B', 1), INT32: ('<i', 4)}


def T(*members):
    return list(members)


Int32T = T((INT32, 'Int32', None, 0))
Real32T = T((REAL32, 'Real32', None, 0))
BoneT = T((STRING, 'Name', None, 0), (INT32, 'ParentIndex', None, 0), (TRANSFORM, 'Transform', None, 0),
          (REAL32, 'InverseWorldTransform', None, 16), (REAL32, 'LODError', None, 0), (VARIANT, 'ExtendedData', None, 0))
SkeletonT = T((STRING, 'Name', None, 0), (REF_ARRAY, 'Bones', BoneT, 0), (INT32, 'LODType', None, 0),
              (VARIANT, 'ExtendedData', None, 0))
VertexT = T((REAL32, 'Position', None, 3), (NORMAL_UINT8, 'BoneWeights', None, 4), (UINT8, 'BoneIndices', None, 4),
            (REAL32, 'Normal', None, 3), (REAL32, 'TextureCoordinates0', None, 2))
VertexDataT = T((VARIANT_ARRAY, 'Vertices', None, 0))
GroupT = T((INT32, 'MaterialIndex', None, 0), (INT32, 'TriFirst', None, 0), (INT32, 'TriCount', None, 0))
TopologyT = T((REF_ARRAY, 'Groups', GroupT, 0), (REF_ARRAY, 'Indices', Int32T, 0))
BoneBindingT = T((STRING, 'BoneName', None, 0), (REAL32, 'OBBMin', None, 3), (REAL32, 'OBBMax', None, 3))
MeshT = T((STRING, 'Name', None, 0), (REF, 'PrimaryVertexData', VertexDataT, 0), (REF, 'PrimaryTopology', TopologyT, 0),
          (REF_ARRAY, 'BoneBindings', BoneBindingT, 0), (VARIANT, 'ExtendedData', None, 0))
MeshBindingT = T((REF, 'Mesh', MeshT, 0))
ModelT = T((STRING, 'Name', None, 0), (REF, 'Skeleton', SkeletonT, 0), (TRANSFORM, 'InitialPlacement', None, 0),
           (REF_ARRAY, 'MeshBindings', MeshBindingT, 0), (VARIANT, 'ExtendedData', None, 0))
CurveT = T((INT32, 'Degree', None, 0), (REF_ARRAY, 'Knots', Real32T, 0), (REF_ARRAY, 'Controls', Real32T, 0))
TrackT = T((STRING, 'Name', None, 0), (INT32, 'Flags', None, 0), (INLINE, 'OrientationCurve', CurveT, 0),
           (INLINE, 'PositionCurve', CurveT, 0), (INLINE, 'ScaleShearCurve', CurveT, 0))
TrackGroupT = T((STRING, 'Name', None, 0), (REF_ARRAY, 'TransformTracks', TrackT, 0),
                (TRANSFORM, 'InitialPlacement', None, 0), (INT32, 'AccumulationFlags', None, 0),
                (REAL32, 'LoopTranslation', None, 3), (VARIANT, 'ExtendedData', None, 0))
AnimationT = T((STRING, 'Name', None, 0), (REAL32, 'Duration', None, 0), (REAL32, 'TimeStep', None, 0),
               (REAL32, 'Oversampling', None, 0), (ARRAY_REFS, 'TrackGroups', TrackGroupT, 0),
               (INT32, 'DefaultLoopCount', None, 0), (INT32, 'Flags', None, 0), (VARIANT, 'ExtendedData', None, 0))
FileInfoT = T((STRING, 'FromFileName', None, 0), (ARRAY_REFS, 'Skeletons', SkeletonT, 0),
              (ARRAY_REFS, 'VertexDatas', VertexDataT, 0), (ARRAY_REFS, 'TriTopologies', TopologyT, 0),
              (ARRAY_REFS, 'Meshes', MeshT, 0), (ARRAY_REFS, 'Models', ModelT, 0),
              (ARRAY_REFS, 'TrackGroups', TrackGroupT, 0), (ARRAY_REFS, 'Animations', AnimationT, 0),
              (VARIANT, 'ExtendedData', None, 0))


def member_size(m):
    t, _, ref, w = m
    w = max(w, 1)
    if t == INLINE:
        return type_size(ref) * w
    if t in (REF, STRING):
        return P * w
    if t in (REF_ARRAY, ARRAY_REFS):
        return (4 + P) * w
    if t == VARIANT:
        return 2 * P * w
    if t == VARIANT_ARRAY:
        return (2 * P + 4) * w
    if t == TRANSFORM:
        return 68 * w
    return SCALAR[t][1] * w


def type_size(tdef):
    return sum(member_size(m) for m in tdef)


class Writer:
    def __init__(self):
        self.buf = bytearray()
        self.fixups = []
        self.memo = {}

    def alloc(self, n):
        while len(self.buf) % 4:
            self.buf.append(0)
        off = len(self.buf)
        self.buf += bytes(n)
        return off

    def ptr(self, at, dest):
        if dest is not None:
            self.fixups.append((at, dest))

    def string(self, s):
        key = ('s', s)
        if key not in self.memo:
            data = s.encode() + b'\0'
            off = self.alloc(len(data))
            self.buf[off:off + len(data)] = data
            self.memo[key] = off
        return self.memo[key]

    def typedef(self, tdef):
        key = ('t', id(tdef))
        if key in self.memo:
            return self.memo[key]
        off = self.alloc(32 * (len(tdef) + 1))
        self.memo[key] = off
        for i, (t, name, ref, w) in enumerate(tdef):
            e = off + 32 * i
            struct.pack_into('<i', self.buf, e, t)
            self.ptr(e + 4, self.string(name))
            if ref is not None:
                self.ptr(e + 8, self.typedef(ref))
            struct.pack_into('<i', self.buf, e + 12, w)
        return off

    def obj(self, tdef, val):
        key = ('o', id(val), id(tdef))
        if key in self.memo:
            return self.memo[key]
        off = self.alloc(type_size(tdef))
        self.memo[key] = off
        self.fill(tdef, val, off)
        return off

    def array(self, tdef, vals):
        size = type_size(tdef)
        off = self.alloc(size * len(vals))
        for i, v in enumerate(vals):
            self.fill(tdef, v, off + size * i)
        return off

    def fill(self, tdef, val, off):
        if not isinstance(val, dict):
            val = {tdef[0][1]: val}
        for m in tdef:
            self.member(m, val.get(m[1]), off)
            off += member_size(m)

    def member(self, m, v, at):
        t, _, ref, w = m
        if v is None:
            return
        if t in SCALAR:
            fmt, sz = SCALAR[t]
            for i, x in enumerate(v if isinstance(v, (list, tuple)) else [v]):
                struct.pack_into(fmt, self.buf, at + sz * i, x)
        elif t == TRANSFORM:
            pos, quat = v
            struct.pack_into('<I3f4f9f', self.buf, at, 0 if quat == (0, 0, 0, 1) else 2, *pos, *quat,
                             1, 0, 0, 0, 1, 0, 0, 0, 1)
        elif t == STRING:
            self.ptr(at, self.string(v))
        elif t == INLINE:
            self.fill(ref, v, at)
        elif t == REF:
            self.ptr(at, self.obj(ref, v))
        elif t == REF_ARRAY:
            struct.pack_into('<i', self.buf, at, len(v))
            if v:
                self.ptr(at + 4, self.array(ref, v))
        elif t == ARRAY_REFS:
            struct.pack_into('<i', self.buf, at, len(v))
            if v:
                arr = self.alloc(P * len(v))
                self.ptr(at + 4, arr)
                for i, x in enumerate(v):
                    self.ptr(arr + P * i, self.obj(ref, x))
        elif t == VARIANT_ARRAY:
            vtype, items = v
            self.ptr(at, self.typedef(vtype))
            struct.pack_into('<i', self.buf, at + P, len(items))
            self.ptr(at + P + 4, self.array(vtype, items))

    def file(self, root_type, root):
        root_off = self.obj(root_type, root)
        type_off = self.typedef(root_type)
        while len(self.buf) % 4:
            self.buf.append(0)
        fi_size, sec_hdr = 0x48, 44
        data_off = 0x20 + fi_size + sec_hdr
        fix_off = data_off + len(self.buf)
        total = fix_off + 12 * len(self.fixups)
        out = bytearray(total)
        struct.pack_into('<4I', out, 0, 3400558520, 263286264, 2123133572, 503322974)
        struct.pack_into('<2I', out, 0x10, 0x20 + fi_size + sec_hdr, 0)
        struct.pack_into('<9I', out, 0x20, 7, total, 0, fi_size, 1, 0, type_off, 0, root_off)
        struct.pack_into('<11I', out, 0x20 + fi_size, 0, data_off, len(self.buf), len(self.buf), 4,
                         len(self.buf), len(self.buf), fix_off, len(self.fixups), fix_off, 0)
        out[data_off:fix_off] = self.buf
        for i, (src, dst) in enumerate(self.fixups):
            struct.pack_into('<3I', out, fix_off + 12 * i, src, 0, dst)
        return bytes(out)


def build():
    bones_n, seg, rings_per_bone, sides = 6, 1.0, 6, 16
    length = bones_n * seg
    bones = []
    for b in range(bones_n):
        inv = [1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, -b * seg, 0, 1]
        bones.append({'Name': 'Bone%d' % b, 'ParentIndex': b - 1,
                      'Transform': ((0, 0 if b == 0 else seg, 0), (0, 0, 0, 1)),
                      'InverseWorldTransform': inv})
    skeleton = {'Name': 'Spine', 'Bones': bones}

    verts, idx = [], []
    rings = bones_n * rings_per_bone + 1
    for r in range(rings):
        y = length * r / (rings - 1)
        radius = 0.45 * (1.0 - 0.55 * (y / length) ** 2) + 0.05 * math.sin(r * 1.7)
        f = min(y / seg, bones_n - 1e-4)
        b0 = min(int(f), bones_n - 1)
        frac = f - b0
        if frac < 0.5 and b0 > 0:
            pair, w1 = (b0 - 1, b0), 0.5 + frac
        elif b0 < bones_n - 1:
            pair, w1 = (b0, b0 + 1), frac - 0.5
        else:
            pair, w1 = (b0, b0), 1.0
        w1 = max(0.0, min(1.0, w1))
        wa = int(round(255 * (1 - w1)))
        for s in range(sides + 1):
            a = 2 * math.pi * s / sides
            nx, nz = math.cos(a), math.sin(a)
            verts.append({'Position': (radius * nx, y, radius * nz), 'BoneWeights': (wa, 255 - wa, 0, 0),
                          'BoneIndices': (pair[0], pair[1], 0, 0), 'Normal': (nx, 0, nz),
                          'TextureCoordinates0': (s / sides, r / (rings - 1))})
    for r in range(rings - 1):
        for s in range(sides):
            a, b = r * (sides + 1) + s, (r + 1) * (sides + 1) + s
            idx += [a, b, a + 1, a + 1, b, b + 1]

    vdata = {'Vertices': (VertexT, verts)}
    topo = {'Groups': [{'MaterialIndex': 0, 'TriFirst': 0, 'TriCount': len(idx) // 3}], 'Indices': idx}
    mesh = {'Name': 'Yarn', 'PrimaryVertexData': vdata, 'PrimaryTopology': topo,
            'BoneBindings': [{'BoneName': b['Name'], 'OBBMin': (-0.5, 0, -0.5), 'OBBMax': (0.5, seg, 0.5)} for b in bones]}
    model = {'Name': 'YarnWorm', 'Skeleton': skeleton, 'InitialPlacement': ((0, 0, 0), (0, 0, 0, 1)),
             'MeshBindings': [{'Mesh': mesh}]}

    duration, keys = 2.0, 17
    tracks = []
    for b in range(bones_n):
        knots, controls = [], []
        for k in range(keys):
            t = duration * k / (keys - 1)
            ang = (0.10 + 0.05 * b) * math.sin(2 * math.pi * t / duration - b * 0.9)
            twist = 0.08 * math.cos(2 * math.pi * t / duration - b * 0.6)
            qz = (0, 0, math.sin(ang / 2), math.cos(ang / 2))
            qx = (math.sin(twist / 2), 0, 0, math.cos(twist / 2))
            q = (qz[3] * qx[0], qz[2] * qx[0], qz[2] * qx[3], qz[3] * qx[3])
            knots.append(t)
            controls += list(q)
        tracks.append({'Name': 'Bone%d' % b, 'OrientationCurve': {'Degree': 1, 'Knots': knots, 'Controls': controls}})
    group = {'Name': 'YarnWorm', 'TransformTracks': tracks, 'InitialPlacement': ((0, 0, 0), (0, 0, 0, 1))}
    anim = {'Name': 'Wiggle', 'Duration': duration, 'TimeStep': duration / (keys - 1), 'Oversampling': 1.0,
            'TrackGroups': [group]}
    return {'FromFileName': 'yarn_worm', 'Skeletons': [skeleton], 'VertexDatas': [vdata], 'TriTopologies': [topo],
            'Meshes': [mesh], 'Models': [model], 'TrackGroups': [group], 'Animations': [anim]}


if __name__ == '__main__':
    out = sys.argv[1] if len(sys.argv) > 1 else 'yarn_worm.gr2'
    with open(out, 'wb') as f:
        f.write(Writer().file(FileInfoT, build()))
