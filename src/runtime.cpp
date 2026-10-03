// OpenGr2ndma runtime: implements the granny.h API subset.
// Reads .gr2 files (format 6/7, uncompressed or Oodle-1), converts them into the
// granny.h in-memory structures, and implements skeleton/animation/deformation.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
#include <granny.h>
#include <cstdarg>
#include <cstdio>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <strings.h>
#include <unordered_map>
#include <utility>
#include <vector>

extern "C" {
#include "oodle1.h"
}

namespace {

granny_log_callback g_log = { 0, 0 };

void GrtLog(const char* file, int line, const char* fmt, ...)
{
    char msg[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    if (g_log.Function)
        g_log.Function(GrannyErrorLogMessage, GrannyFileReadingLogMessage, file, line, msg, g_log.UserData);
    else
        fprintf(stderr, "OpenGr2ndma: %s\n", msg);
}

// Size of a pointer in the native (in-memory) granny.h layout.
const int NP = (int)sizeof(void*);

typedef granny_data_type_definition TDef;

#define GM(t, n, r, w) { Granny##t##Member, n, r, w, {0, 0, 0}, 0 }
#define GEND { GrannyEndMember, 0, 0, 0, {0, 0, 0}, 0 }

}

extern TDef grt_Real32[], grt_Int32[], grt_UInt32[], grt_UInt16[], grt_UInt8[], grt_String[];
extern TDef grt_ArtToolInfo[], grt_ExporterInfo[], grt_MipLevel[], grt_TexImage[], grt_PixelLayout[], grt_Texture[];
extern TDef grt_MaterialMap[], grt_Material[], grt_Bone[], grt_Skeleton[], grt_VertexAnnotationSet[], grt_VertexData[];
extern TDef grt_TriMaterialGroup[], grt_TriAnnotationSet[], grt_TriTopology[], grt_MorphTarget[], grt_MaterialBinding[];
extern TDef grt_BoneBinding[], grt_Mesh[], grt_ModelMeshBinding[], grt_Model[], grt_OldCurve[], grt_Curve2[];
extern TDef grt_TransformTrack[], grt_VectorTrack[], grt_TextTrackEntry[], grt_TextTrack[], grt_PeriodicLoop[];
extern TDef grt_TrackGroup[], grt_Animation[], grt_FileInfo[];

TDef grt_Real32[] = { GM(Real32, "Real32", 0, 0), GEND };
TDef grt_Int32[] = { GM(Int32, "Int32", 0, 0), GEND };
TDef grt_UInt32[] = { GM(UInt32, "UInt32", 0, 0), GEND };
TDef grt_UInt16[] = { GM(UInt16, "UInt16", 0, 0), GEND };
TDef grt_UInt8[] = { GM(UInt8, "UInt8", 0, 0), GEND };
TDef grt_String[] = { GM(String, "String", 0, 0), GEND };
TDef grt_ArtToolInfo[] = {
    GM(String, "FromArtToolName", 0, 0), GM(Int32, "ArtToolMajorRevision", 0, 0), GM(Int32, "ArtToolMinorRevision", 0, 0),
    GM(Int32, "ArtToolPointerSize", 0, 0), GM(Real32, "UnitsPerMeter", 0, 0), GM(Real32, "Origin", 0, 3),
    GM(Real32, "RightVector", 0, 3), GM(Real32, "UpVector", 0, 3), GM(Real32, "BackVector", 0, 3),
    GM(VariantReference, "ExtendedData", 0, 0), GEND };
TDef grt_ExporterInfo[] = {
    GM(String, "ExporterName", 0, 0), GM(Int32, "ExporterMajorRevision", 0, 0), GM(Int32, "ExporterMinorRevision", 0, 0),
    GM(Int32, "ExporterCustomization", 0, 0), GM(Int32, "ExporterBuildNumber", 0, 0),
    GM(VariantReference, "ExtendedData", 0, 0), GEND };
TDef grt_MipLevel[] = { GM(Int32, "Stride", 0, 0), GM(ReferenceToArray, "PixelBytes", grt_UInt8, 0), GEND };
TDef grt_TexImage[] = { GM(ReferenceToArray, "MIPLevels", grt_MipLevel, 0), GEND };
TDef grt_PixelLayout[] = { GM(Int32, "BytesPerPixel", 0, 0), GM(Int32, "ShiftForComponent", 0, 4), GM(Int32, "BitsForComponent", 0, 4), GEND };
TDef grt_Texture[] = {
    GM(String, "FromFileName", 0, 0), GM(Int32, "TextureType", 0, 0), GM(Int32, "Width", 0, 0), GM(Int32, "Height", 0, 0),
    GM(Int32, "Encoding", 0, 0), GM(Int32, "SubFormat", 0, 0), GM(Inline, "Layout", grt_PixelLayout, 0),
    GM(ReferenceToArray, "Images", grt_TexImage, 0), GM(VariantReference, "ExtendedData", 0, 0), GEND };
TDef grt_MaterialMap[] = { GM(String, "Usage", 0, 0), GM(Reference, "Map", grt_Material, 0), GEND };
TDef grt_Material[] = {
    GM(String, "Name", 0, 0), GM(ReferenceToArray, "Maps", grt_MaterialMap, 0), GM(Reference, "Texture", grt_Texture, 0),
    GM(VariantReference, "ExtendedData", 0, 0), GEND };
TDef grt_Bone[] = {
    GM(String, "Name", 0, 0), GM(Int32, "ParentIndex", 0, 0), GM(Transform, "Transform", 0, 0),
    GM(Real32, "InverseWorldTransform", 0, 16), GM(Real32, "LODError", 0, 0), GM(VariantReference, "ExtendedData", 0, 0), GEND };
TDef grt_Skeleton[] = {
    GM(String, "Name", 0, 0), GM(ReferenceToArray, "Bones", grt_Bone, 0), GM(Int32, "LODType", 0, 0),
    GM(VariantReference, "ExtendedData", 0, 0), GEND };
TDef grt_VertexAnnotationSet[] = {
    GM(String, "Name", 0, 0), GM(ReferenceToVariantArray, "VertexAnnotations", 0, 0),
    GM(Int32, "IndicesMapFromVertexToAnnotation", 0, 0), GM(ReferenceToArray, "VertexAnnotationIndices", grt_Int32, 0), GEND };
TDef grt_VertexData[] = {
    GM(ReferenceToVariantArray, "Vertices", 0, 0), GM(ReferenceToArray, "VertexComponentNames", grt_String, 0),
    GM(ReferenceToArray, "VertexAnnotationSets", grt_VertexAnnotationSet, 0), GEND };
TDef grt_TriMaterialGroup[] = { GM(Int32, "MaterialIndex", 0, 0), GM(Int32, "TriFirst", 0, 0), GM(Int32, "TriCount", 0, 0), GEND };
TDef grt_TriAnnotationSet[] = {
    GM(String, "Name", 0, 0), GM(ReferenceToVariantArray, "TriAnnotations", 0, 0),
    GM(Int32, "IndicesMapFromTriToAnnotation", 0, 0), GM(ReferenceToArray, "TriAnnotationIndices", grt_Int32, 0), GEND };
TDef grt_TriTopology[] = {
    GM(ReferenceToArray, "Groups", grt_TriMaterialGroup, 0), GM(ReferenceToArray, "Indices", grt_Int32, 0),
    GM(ReferenceToArray, "Indices16", grt_UInt16, 0), GM(ReferenceToArray, "VertexToVertexMap", grt_Int32, 0),
    GM(ReferenceToArray, "VertexToTriangleMap", grt_Int32, 0), GM(ReferenceToArray, "SideToNeighborMap", grt_UInt32, 0),
    GM(ReferenceToArray, "PolygonIndexStarts", grt_Int32, 0), GM(ReferenceToArray, "PolygonIndices", grt_Int32, 0),
    GM(ReferenceToArray, "BonesForTriangle", grt_Int32, 0), GM(ReferenceToArray, "TriangleToBoneIndices", grt_Int32, 0),
    GM(ReferenceToArray, "TriAnnotationSets", grt_TriAnnotationSet, 0), GEND };
TDef grt_MorphTarget[] = {
    GM(String, "ScalarName", 0, 0), GM(Reference, "VertexData", grt_VertexData, 0), GM(Int32, "DataIsDeltas", 0, 0), GEND };
TDef grt_MaterialBinding[] = { GM(Reference, "Material", grt_Material, 0), GEND };
TDef grt_BoneBinding[] = {
    GM(String, "BoneName", 0, 0), GM(Real32, "OBBMin", 0, 3), GM(Real32, "OBBMax", 0, 3),
    GM(ReferenceToArray, "TriangleIndices", grt_Int32, 0), GEND };
TDef grt_Mesh[] = {
    GM(String, "Name", 0, 0), GM(Reference, "PrimaryVertexData", grt_VertexData, 0),
    GM(ReferenceToArray, "MorphTargets", grt_MorphTarget, 0), GM(Reference, "PrimaryTopology", grt_TriTopology, 0),
    GM(ReferenceToArray, "MaterialBindings", grt_MaterialBinding, 0), GM(ReferenceToArray, "BoneBindings", grt_BoneBinding, 0),
    GM(VariantReference, "ExtendedData", 0, 0), GEND };
TDef grt_ModelMeshBinding[] = { GM(Reference, "Mesh", grt_Mesh, 0), GEND };
TDef grt_Model[] = {
    GM(String, "Name", 0, 0), GM(Reference, "Skeleton", grt_Skeleton, 0), GM(Transform, "InitialPlacement", 0, 0),
    GM(ReferenceToArray, "MeshBindings", grt_ModelMeshBinding, 0), GM(VariantReference, "ExtendedData", 0, 0), GEND };
TDef grt_OldCurve[] = {
    GM(Int32, "Degree", 0, 0), GM(ReferenceToArray, "Knots", grt_Real32, 0), GM(ReferenceToArray, "Controls", grt_Real32, 0), GEND };
TDef grt_Curve2[] = { GM(VariantReference, "CurveData", 0, 0), GEND };
TDef grt_TransformTrack[] = {
    GM(String, "Name", 0, 0), GM(Int32, "Flags", 0, 0), GM(Inline, "OrientationCurve", grt_Curve2, 0),
    GM(Inline, "PositionCurve", grt_Curve2, 0), GM(Inline, "ScaleShearCurve", grt_Curve2, 0), GEND };
TDef grt_VectorTrack[] = {
    GM(String, "Name", 0, 0), GM(UInt32, "TrackKey", 0, 0), GM(Int32, "Dimension", 0, 0),
    GM(Inline, "ValueCurve", grt_Curve2, 0), GEND };
TDef grt_TextTrackEntry[] = { GM(Real32, "TimeStamp", 0, 0), GM(String, "Text", 0, 0), GEND };
TDef grt_TextTrack[] = { GM(String, "Name", 0, 0), GM(ReferenceToArray, "Entries", grt_TextTrackEntry, 0), GEND };
TDef grt_PeriodicLoop[] = {
    GM(Real32, "Radius", 0, 0), GM(Real32, "dAngle", 0, 0), GM(Real32, "dZ", 0, 0), GM(Real32, "BasisX", 0, 3),
    GM(Real32, "BasisY", 0, 3), GM(Real32, "Axis", 0, 3), GEND };
TDef grt_TrackGroup[] = {
    GM(String, "Name", 0, 0), GM(ReferenceToArray, "VectorTracks", grt_VectorTrack, 0),
    GM(ReferenceToArray, "TransformTracks", grt_TransformTrack, 0), GM(ReferenceToArray, "TransformLODErrors", grt_Real32, 0),
    GM(ReferenceToArray, "TextTracks", grt_TextTrack, 0), GM(Transform, "InitialPlacement", 0, 0),
    GM(Int32, "AccumulationFlags", 0, 0), GM(Real32, "LoopTranslation", 0, 3), GM(Reference, "PeriodicLoop", grt_PeriodicLoop, 0),
    GM(VariantReference, "ExtendedData", 0, 0), GEND };
TDef grt_Animation[] = {
    GM(String, "Name", 0, 0), GM(Real32, "Duration", 0, 0), GM(Real32, "TimeStep", 0, 0), GM(Real32, "Oversampling", 0, 0),
    GM(ArrayOfReferences, "TrackGroups", grt_TrackGroup, 0), GM(Int32, "DefaultLoopCount", 0, 0), GM(Int32, "Flags", 0, 0),
    GM(VariantReference, "ExtendedData", 0, 0), GEND };
TDef grt_FileInfo[] = {
    GM(Reference, "ArtToolInfo", grt_ArtToolInfo, 0), GM(Reference, "ExporterInfo", grt_ExporterInfo, 0),
    GM(String, "FromFileName", 0, 0), GM(ArrayOfReferences, "Textures", grt_Texture, 0),
    GM(ArrayOfReferences, "Materials", grt_Material, 0), GM(ArrayOfReferences, "Skeletons", grt_Skeleton, 0),
    GM(ArrayOfReferences, "VertexDatas", grt_VertexData, 0), GM(ArrayOfReferences, "TriTopologies", grt_TriTopology, 0),
    GM(ArrayOfReferences, "Meshes", grt_Mesh, 0), GM(ArrayOfReferences, "Models", grt_Model, 0),
    GM(ArrayOfReferences, "TrackGroups", grt_TrackGroup, 0), GM(ArrayOfReferences, "Animations", grt_Animation, 0),
    GM(VariantReference, "ExtendedData", 0, 0), GEND };

namespace {

int ScalarSize(granny_member_type t)
{
    switch (t) {
    case GrannyReal32Member: case GrannyInt32Member: case GrannyUInt32Member: return 4;
    case GrannyInt8Member: case GrannyUInt8Member: case GrannyBinormalInt8Member: case GrannyNormalUInt8Member: return 1;
    case GrannyInt16Member: case GrannyUInt16Member: case GrannyBinormalInt16Member: case GrannyNormalUInt16Member:
    case GrannyReal16Member: return 2;
    default: return 0;
    }
}

bool IsScalar(granny_member_type t) { return ScalarSize(t) != 0; }
int Width(const TDef* m) { return m->ArrayWidth > 0 ? m->ArrayWidth : 1; }

int TypeSize(const TDef* d, int P);

int MemberSize(const TDef* m, int P)
{
    switch (m->Type) {
    case GrannyInlineMember: return m->ReferenceType ? TypeSize(m->ReferenceType, P) * Width(m) : 0;
    case GrannyReferenceMember: case GrannyStringMember: case GrannyEmptyReferenceMember: return P * Width(m);
    case GrannyReferenceToArrayMember: case GrannyArrayOfReferencesMember: return (4 + P) * Width(m);
    case GrannyVariantReferenceMember: return 2 * P * Width(m);
    case GrannyReferenceToVariantArrayMember: return (2 * P + 4) * Width(m);
    case GrannyTransformMember: return 68 * Width(m);
    default: return ScalarSize(m->Type) * Width(m);
    }
}

int TypeSize(const TDef* d, int P)
{
    int s = 0;
    for (; d && d->Type != GrannyEndMember; ++d)
        s += MemberSize(d, P);
    return s;
}

float HalfToFloat(uint16_t h)
{
    uint32_t sign = (h >> 15) & 1, exp = (h >> 10) & 31, man = h & 1023;
    float v;
    if (exp == 0) v = ldexpf((float)man, -24);
    else if (exp == 31) v = man ? NAN : INFINITY;
    else v = ldexpf((float)(man | 1024), (int)exp - 25);
    return sign ? -v : v;
}

double ReadNum(granny_member_type t, const uint8_t* p, int i)
{
    switch (t) {
    case GrannyReal32Member: { float f; memcpy(&f, p + 4 * i, 4); return f; }
    case GrannyInt32Member: { int32_t v; memcpy(&v, p + 4 * i, 4); return v; }
    case GrannyUInt32Member: { uint32_t v; memcpy(&v, p + 4 * i, 4); return v; }
    case GrannyInt8Member: return (int8_t)p[i];
    case GrannyUInt8Member: return p[i];
    case GrannyBinormalInt8Member: return (int8_t)p[i] / 127.0;
    case GrannyNormalUInt8Member: return p[i] / 255.0;
    case GrannyInt16Member: { int16_t v; memcpy(&v, p + 2 * i, 2); return v; }
    case GrannyUInt16Member: { uint16_t v; memcpy(&v, p + 2 * i, 2); return v; }
    case GrannyBinormalInt16Member: { int16_t v; memcpy(&v, p + 2 * i, 2); return v / 32767.0; }
    case GrannyNormalUInt16Member: { uint16_t v; memcpy(&v, p + 2 * i, 2); return v / 65535.0; }
    case GrannyReal16Member: { uint16_t v; memcpy(&v, p + 2 * i, 2); return HalfToFloat(v); }
    default: return 0;
    }
}

void WriteNum(granny_member_type t, uint8_t* p, int i, double v)
{
    switch (t) {
    case GrannyReal32Member: { float f = (float)v; memcpy(p + 4 * i, &f, 4); break; }
    case GrannyInt32Member: { int32_t x = (int32_t)lround(v); memcpy(p + 4 * i, &x, 4); break; }
    case GrannyUInt32Member: { uint32_t x = (uint32_t)llround(v); memcpy(p + 4 * i, &x, 4); break; }
    case GrannyInt8Member: p[i] = (uint8_t)(int8_t)lround(v); break;
    case GrannyUInt8Member: p[i] = (uint8_t)lround(v); break;
    case GrannyBinormalInt8Member: p[i] = (uint8_t)(int8_t)lround(v * 127.0); break;
    case GrannyNormalUInt8Member: p[i] = (uint8_t)lround(v * 255.0); break;
    case GrannyInt16Member: { int16_t x = (int16_t)lround(v); memcpy(p + 2 * i, &x, 2); break; }
    case GrannyUInt16Member: { uint16_t x = (uint16_t)lround(v); memcpy(p + 2 * i, &x, 2); break; }
    case GrannyBinormalInt16Member: { int16_t x = (int16_t)lround(v * 32767.0); memcpy(p + 2 * i, &x, 2); break; }
    case GrannyNormalUInt16Member: { uint16_t x = (uint16_t)lround(v * 65535.0); memcpy(p + 2 * i, &x, 2); break; }
    default: break;
    }
}

bool NamesEqual(const char* a, const char* b) { return a && b && strcasecmp(a, b) == 0; }

int MemberCount(const TDef* d)
{
    int n = 0;
    for (; d && d->Type != GrannyEndMember; ++d) ++n;
    return n;
}

bool PointerFree(const TDef* d)
{
    for (; d && d->Type != GrannyEndMember; ++d) {
        if (d->Type == GrannyInlineMember) { if (!PointerFree(d->ReferenceType)) return false; }
        else if (!IsScalar(d->Type) && d->Type != GrannyTransformMember) return false;
    }
    return true;
}

bool SameLayout(const TDef* a, const TDef* b)
{
    if (a == b) return true;
    int na = MemberCount(a), nb = MemberCount(b);
    if (na != nb) return false;
    for (int i = 0; i < na; ++i) {
        if (a[i].Type != b[i].Type || Width(&a[i]) != Width(&b[i])) return false;
        if (na > 1 && !NamesEqual(a[i].Name, b[i].Name)) return false;
        if (a[i].Type == GrannyInlineMember && !SameLayout(a[i].ReferenceType, b[i].ReferenceType)) return false;
    }
    return true;
}

// Native objects use the granny.h layout (pack(4), native-size pointers); everything we
// write from type definitions follows the same no-padding rule.
void ConvertNative(const TDef* sd, const uint8_t* sp, const TDef* dd, uint8_t* dp);

const TDef* FindMember(const TDef* d, const char* name, int P, int* offset)
{
    int off = 0;
    for (const TDef* m = d; m && m->Type != GrannyEndMember; ++m) {
        if (NamesEqual(m->Name, name)) { *offset = off; return m; }
        off += MemberSize(m, P);
    }
    return 0;
}

struct GrnFile
{
    std::vector<uint8_t*> sectors;
    std::vector<void*> allocs;
    std::unordered_map<const uint8_t*, uint8_t*> fixups;
    std::unordered_map<const uint8_t*, TDef*> typeMemo;
    std::map<std::pair<const void*, const void*>, void*> objMemo;
    granny_file_info* info = 0;
    int P = 4;

    ~GrnFile()
    {
        for (void* p : allocs) free(p);
        for (uint8_t* s : sectors) free(s);
    }

    void* Alloc(size_t n)
    {
        void* p = calloc(1, n ? n : 1);
        allocs.push_back(p);
        return p;
    }

    uint8_t* Ptr(const uint8_t* slot) const
    {
        auto it = fixups.find(slot);
        return it == fixups.end() ? 0 : it->second;
    }

    static int32_t I32(const uint8_t* p) { int32_t v; memcpy(&v, p, 4); return v; }

    TDef* Type(const uint8_t* fdef)
    {
        if (!fdef) return 0;
        auto it = typeMemo.find(fdef);
        if (it != typeMemo.end()) return it->second;
        const int defSize = P == 4 ? 32 : 44;
        int n = 0;
        while (I32(fdef + n * defSize) != GrannyEndMember && n < 4096) ++n;
        TDef* out = (TDef*)Alloc(sizeof(TDef) * (n + 1));
        typeMemo[fdef] = out;
        for (int i = 0; i < n; ++i) {
            const uint8_t* e = fdef + i * defSize;
            out[i].Type = (granny_member_type)I32(e);
            out[i].Name = (const char*)Ptr(e + 4);
            out[i].ReferenceType = Type(Ptr(e + 4 + P));
            out[i].ArrayWidth = I32(e + 4 + 2 * P);
            for (int k = 0; k < 3; ++k) out[i].Extra[k] = I32(e + 8 + 2 * P + 4 * k);
        }
        out[n].Type = GrannyEndMember;
        return out;
    }

    void* Array(const TDef* sd, const uint8_t* arr, int count, const TDef* dd)
    {
        if (!arr || count <= 0 || !sd || !dd) return 0;
        auto key = std::make_pair((const void*)arr, (const void*)dd);
        auto it = objMemo.find(key);
        if (it != objMemo.end()) return it->second;
        int ss = TypeSize(sd, P), ds = TypeSize(dd, NP);
        uint8_t* out = (uint8_t*)Alloc((size_t)ds * count);
        objMemo[key] = out;
        if (PointerFree(sd) && SameLayout(sd, dd))
            memcpy(out, arr, (size_t)ss * count);
        else
            for (int i = 0; i < count; ++i) Struct(sd, arr + (size_t)ss * i, dd, out + (size_t)ds * i);
        return out;
    }

    void* Object(const TDef* sd, const uint8_t* obj, const TDef* dd)
    {
        if (!obj || !sd || !dd) return 0;
        auto key = std::make_pair((const void*)obj, (const void*)dd);
        auto it = objMemo.find(key);
        if (it != objMemo.end()) return it->second;
        uint8_t* out = (uint8_t*)Alloc(TypeSize(dd, NP));
        objMemo[key] = out;
        Struct(sd, obj, dd, out);
        return out;
    }

    void Member(const TDef* sm, const uint8_t* sp, const TDef* dm, uint8_t* dp)
    {
        if (IsScalar(dm->Type) && IsScalar(sm->Type)) {
            int n = (std::min)(Width(dm), Width(sm));
            for (int i = 0; i < n; ++i) WriteNum(dm->Type, dp, i, ReadNum(sm->Type, sp, i));
            return;
        }
        if (dm->Type != sm->Type) return;
        switch (dm->Type) {
        case GrannyTransformMember: memcpy(dp, sp, 68 * (std::min)(Width(dm), Width(sm))); break;
        case GrannyStringMember: { void* s = Ptr(sp); memcpy(dp, &s, NP); break; }
        case GrannyInlineMember: {
            int n = (std::min)(Width(dm), Width(sm));
            int ss = TypeSize(sm->ReferenceType, P), ds = TypeSize(dm->ReferenceType, NP);
            for (int i = 0; i < n; ++i) Struct(sm->ReferenceType, sp + ss * i, dm->ReferenceType, dp + ds * i);
            break;
        }
        case GrannyReferenceMember: {
            void* o = Object(sm->ReferenceType, Ptr(sp), dm->ReferenceType);
            memcpy(dp, &o, NP);
            break;
        }
        case GrannyReferenceToArrayMember: {
            int32_t count = I32(sp);
            void* a = Array(sm->ReferenceType, Ptr(sp + 4), count, dm->ReferenceType);
            if (!a) count = 0;
            memcpy(dp, &count, 4);
            memcpy(dp + 4, &a, NP);
            break;
        }
        case GrannyArrayOfReferencesMember: {
            int32_t count = I32(sp);
            const uint8_t* arr = Ptr(sp + 4);
            void** a = 0;
            if (arr && count > 0) {
                a = (void**)Alloc(sizeof(void*) * count);
                for (int i = 0; i < count; ++i) a[i] = Object(sm->ReferenceType, Ptr(arr + P * i), dm->ReferenceType);
            } else count = 0;
            memcpy(dp, &count, 4);
            memcpy(dp + 4, &a, NP);
            break;
        }
        case GrannyVariantReferenceMember: {
            TDef* t = Type(Ptr(sp));
            void* o = t ? Object(t, Ptr(sp + P), t) : 0;
            memcpy(dp, &t, NP);
            memcpy(dp + NP, &o, NP);
            break;
        }
        case GrannyReferenceToVariantArrayMember: {
            TDef* t = Type(Ptr(sp));
            int32_t count = I32(sp + P);
            void* a = t ? Array(t, Ptr(sp + P + 4), count, t) : 0;
            if (!a) count = 0;
            memcpy(dp, &t, NP);
            memcpy(dp + NP, &count, 4);
            memcpy(dp + NP + 4, &a, NP);
            break;
        }
        default: break;
        }
    }

    void Struct(const TDef* sd, const uint8_t* sp, const TDef* dd, uint8_t* dp)
    {
        const bool singles = MemberCount(sd) == 1 && MemberCount(dd) == 1;
        int doff = 0;
        for (const TDef* dm = dd; dm->Type != GrannyEndMember; ++dm) {
            int soff = 0;
            const TDef* sm = singles ? sd : FindMember(sd, dm->Name, P, &soff);
            if (sm)
                Member(sm, sp + soff, dm, dp + doff);
            else if (dm->Type == GrannyVariantReferenceMember && NamesEqual(dm->Name, "CurveData")) {
                int tmp;
                if (FindMember(sd, "Controls", P, &tmp)) {
                    TDef* t = grt_OldCurve;
                    void* o = Object(sd, sp, grt_OldCurve);
                    memcpy(dp + doff, &t, NP);
                    memcpy(dp + doff + NP, &o, NP);
                }
            }
            doff += MemberSize(dm, NP);
        }
    }
};

const uint32_t kMagics[][4] = {
    {3400558520u, 263286264u, 2123133572u, 503322974u},
    {3228360233u, 726901946u, 2780296485u, 4007814902u},
    {1581882341u, 337601391u, 2850755358u, 3303915152u},
};

bool UnOodle1(uint8_t* comp, uint32_t compLen, uint8_t* out, uint32_t outLen, uint32_t stop0, uint32_t stop1)
{
    if (compLen == 0) return true;
    TParameter params[3];
    memcpy(params, comp, sizeof(params));
    TDecoder dec;
    Decoder_Init(&dec, comp + sizeof(params));
    uint32_t steps[] = {stop0, stop1, outLen};
    uint8_t* p = out;
    for (int i = 0; i < 3; ++i) {
        TDictionary dict;
        Dictionary_Init(&dict, &params[i]);
        while (p < out + steps[i]) {
            uint32_t n = Dictionary_Decompress_Block(&dict, &dec, p);
            if (n == 0) { Dictionary_Free(&dict); return false; }
            p += n;
        }
        Dictionary_Free(&dict);
    }
    return true;
}

GrnFile* LoadFile(const uint8_t* data, size_t len)
{
    if (len < 0x60) return 0;
    int magic = -1;
    for (int i = 0; i < 3; ++i)
        if (memcmp(data, kMagics[i], 16) == 0) magic = i;
    if (magic < 0) { GrtLog(__FILE__, __LINE__, "unsupported gr2 magic"); return 0; }
    GrnFile* f = new GrnFile;
    f->P = magic == 2 ? 8 : 4;
    auto U32 = [&](size_t o) { uint32_t v; memcpy(&v, data + o, 4); return v; };
    const size_t fi = 0x20;
    uint32_t fileInfoSize = U32(fi + 12), sectorCount = U32(fi + 16);
    uint32_t typeSec = U32(fi + 20), typePos = U32(fi + 24), rootSec = U32(fi + 28), rootPos = U32(fi + 32);
    size_t sectorBase = fi + fileInfoSize;
    if (sectorCount > 64 || sectorBase + sectorCount * 44 > len) { delete f; return 0; }
    std::vector<uint32_t> fixOff(sectorCount), fixCnt(sectorCount);
    for (uint32_t s = 0; s < sectorCount; ++s) {
        size_t h = sectorBase + s * 44;
        uint32_t ctype = U32(h), dataOff = U32(h + 4), clen = U32(h + 8), dlen = U32(h + 12);
        uint32_t stop0 = U32(h + 20), stop1 = U32(h + 24);
        fixOff[s] = U32(h + 28);
        fixCnt[s] = U32(h + 32);
        uint8_t* buf = (uint8_t*)calloc(1, dlen + 16);
        f->sectors.push_back(buf);
        if (dlen == 0) continue;
        if (ctype == 0) {
            if (dataOff + dlen > len) { delete f; return 0; }
            memcpy(buf, data + dataOff, dlen);
        } else if (ctype == 1 || ctype == 2) {
            if (dataOff + clen > len) { delete f; return 0; }
            std::vector<uint8_t> comp(clen + 16, 0);
            memcpy(comp.data(), data + dataOff, clen);
            if (!UnOodle1(comp.data(), clen, buf, dlen, stop0, stop1)) { GrtLog(__FILE__, __LINE__, "oodle1 failed"); delete f; return 0; }
        } else {
            GrtLog(__FILE__, __LINE__, "unsupported gr2 compression %u", ctype);
            delete f;
            return 0;
        }
    }
    for (uint32_t s = 0; s < sectorCount; ++s) {
        for (uint32_t k = 0; k < fixCnt[s]; ++k) {
            size_t o = fixOff[s] + k * 12;
            if (o + 12 > len) break;
            uint32_t src = U32(o), dsec = U32(o + 4), doff = U32(o + 8);
            if (dsec >= sectorCount) continue;
            f->fixups[f->sectors[s] + src] = f->sectors[dsec] + doff;
        }
    }
    if (typeSec >= sectorCount || rootSec >= sectorCount) { delete f; return 0; }
    TDef* rootType = f->Type(f->sectors[typeSec] + typePos);
    f->info = (granny_file_info*)f->Object(rootType, f->sectors[rootSec] + rootPos, grt_FileInfo);
    if (!f->info) { delete f; return 0; }
    return f;
}

// ---------------------------------------------------------------- math
// Matrices are stored the D3D way (row vectors, translation in row 3), matching
// how the client reinterprets granny_matrix_4x4 as D3DXMATRIX.
struct Mat { float m[16]; operator float*() { return m; } operator const float*() const { return m; } };

void MatMul(const float* a, const float* b, float* out)
{
    float r[16];
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            r[i * 4 + j] = a[i * 4 + 0] * b[0 * 4 + j] + a[i * 4 + 1] * b[1 * 4 + j] + a[i * 4 + 2] * b[2 * 4 + j] + a[i * 4 + 3] * b[3 * 4 + j];
    memcpy(out, r, sizeof(r));
}

void TransformToMatrix(const granny_transform& t, float* m)
{
    float x = t.Orientation[0], y = t.Orientation[1], z = t.Orientation[2], w = t.Orientation[3];
    float R[3][3] = {
        {1 - 2 * (y * y + z * z), 2 * (x * y - z * w), 2 * (x * z + y * w)},
        {2 * (x * y + z * w), 1 - 2 * (x * x + z * z), 2 * (y * z - x * w)},
        {2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x * x + y * y)},
    };
    float A[3][3];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            A[i][j] = R[i][0] * t.ScaleShear[0][j] + R[i][1] * t.ScaleShear[1][j] + R[i][2] * t.ScaleShear[2][j];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) m[i * 4 + j] = A[j][i];
        m[i * 4 + 3] = 0;
    }
    m[12] = t.Position[0];
    m[13] = t.Position[1];
    m[14] = t.Position[2];
    m[15] = 1;
}

void Identity(granny_transform& t)
{
    memset(&t, 0, sizeof(t));
    t.Orientation[3] = 1;
    t.ScaleShear[0][0] = t.ScaleShear[1][1] = t.ScaleShear[2][2] = 1;
}

// ---------------------------------------------------------------- runtime objects
struct GrnInstance;

struct GrnControl
{
    GrnInstance* inst = 0;
    const granny_animation* anim = 0;
    const granny_track_group* tg = 0;
    std::vector<int> boneTrack;
    float clockBase = 0, localBase = 0, speed = 1;
    int loopCount = 1;
    bool easeIn = false, easeOut = false;
    float eiS = -1e30f, eiE = -1e30f, ei[4] = {1, 1, 1, 1};
    float eoS = -1e30f, eoE = -1e30f, eo[4] = {1, 1, 1, 1};
    bool hasComplete = false;
    float completeClock = 0;
    bool freeOnceUnused = false;
};

struct GrnInstance
{
    const granny_model* model = 0;
    granny_skeleton* skel = 0;
    float clock = 0;
    std::vector<GrnControl*> controls;
};

struct GrnLocalPose { std::vector<granny_transform> t; };
struct GrnWorldPose { std::vector<Mat> world, composite; };
struct GrnMeshBinding { std::vector<granny_int32x> toBone; };

struct GrnDeformer
{
    int inStride = 0, outStride = 0;
    int inPos = -1, inNrm = -1, inW = -1, inI = -1;
    granny_member_type wType = GrannyNormalUInt8Member, iType = GrannyUInt8Member;
    int wCount = 0, iCount = 0;
    int outPos = -1, outNrm = -1;
    struct Copy { const TDef* s; int so; const TDef* d; int dO; };
    std::vector<Copy> copies;
};

float LocalClock(const GrnControl* c) { return c->localBase + (c->inst->clock - c->clockBase) * c->speed; }

float Bezier(const float* v, float s, float e, float t)
{
    if (t <= s) return v[0];
    if (t >= e || e <= s) return v[3];
    float u = (t - s) / (e - s), iu = 1 - u;
    return v[0] * iu * iu * iu + 3 * v[1] * u * iu * iu + 3 * v[2] * u * u * iu + v[3] * u * u * u;
}

float Weight(const GrnControl* c)
{
    float w = 1, t = c->inst->clock;
    if (c->easeIn) w *= Bezier(c->ei, c->eiS, c->eiE, t);
    if (c->easeOut) w *= Bezier(c->eo, c->eoS, c->eoE, t);
    return w;
}

bool IsComplete(const GrnControl* c) { return c->hasComplete && c->inst->clock >= c->completeClock; }

void DestroyControl(GrnControl* c)
{
    auto& v = c->inst->controls;
    for (size_t i = 0; i < v.size(); ++i)
        if (v[i] == c) { v.erase(v.begin() + i); break; }
    delete c;
}

// Evaluates an old-format (knots + controls) curve at time t into out[dim].
bool SampleCurve(const granny_curve2& curve, float t, int dim, float* out)
{
    if (curve.CurveData.Type != grt_OldCurve || !curve.CurveData.Object) return false;
    const granny_old_curve* c = (const granny_old_curve*)curve.CurveData.Object;
    int n = c->KnotCount;
    if (n <= 0 || c->ControlCount < n * dim || !c->Knots || !c->Controls) return false;
    const float* K = c->Knots;
    const float* C = c->Controls;
    if (n == 1 || t <= K[0]) { memcpy(out, C, dim * 4); return true; }
    if (t >= K[n - 1]) { memcpy(out, C + (n - 1) * dim, dim * 4); return true; }
    int lo = 0, hi = n - 1;
    while (hi - lo > 1) {
        int mid = (lo + hi) / 2;
        if (K[mid] <= t) lo = mid; else hi = mid;
    }
    float span = K[hi] - K[lo];
    float u = span > 0 ? (t - K[lo]) / span : 0;
    if (c->Degree == 0) u = 0;
    const float* a = C + lo * dim;
    const float* b = C + hi * dim;
    if (dim == 4) {
        float dot = a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
        float sb = dot < 0 ? -1.f : 1.f;
        for (int i = 0; i < 4; ++i) out[i] = a[i] * (1 - u) + b[i] * sb * u;
    } else
        for (int i = 0; i < dim; ++i) out[i] = a[i] * (1 - u) + b[i] * u;
    return true;
}

void SampleTrack(const granny_transform_track& tr, float t, const granny_transform& rest, granny_transform& out)
{
    out = rest;
    float q[4];
    if (SampleCurve(tr.OrientationCurve, t, 4, q)) {
        float l = sqrtf(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
        if (l > 1e-8f) for (int i = 0; i < 4; ++i) out.Orientation[i] = q[i] / l;
    }
    SampleCurve(tr.PositionCurve, t, 3, out.Position);
    SampleCurve(tr.ScaleShearCurve, t, 9, &out.ScaleShear[0][0]);
}

void SampleLocalPose(GrnInstance* inst, int boneCount, std::vector<granny_transform>& local)
{
    granny_skeleton* sk = inst->skel;
    local.resize(boneCount);
    std::vector<float> wsum(boneCount, 0.f);
    std::vector<granny_transform> acc(boneCount);
    for (int b = 0; b < boneCount; ++b) memset(&acc[b], 0, sizeof(granny_transform));
    for (GrnControl* c : inst->controls) {
        float w = Weight(c);
        if (w <= 0 || !c->tg) continue;
        float dur = c->anim->Duration;
        float t = LocalClock(c);
        if (dur > 0) {
            if (c->loopCount == 0) { t = fmodf(t, dur); if (t < 0) t += dur; }
            else if (t > dur * c->loopCount) t = dur;
            else if (t > dur) { t = fmodf(t, dur); }
            if (t < 0) t = 0;
        }
        for (int b = 0; b < boneCount && b < (int)c->boneTrack.size(); ++b) {
            int ti = c->boneTrack[b];
            if (ti < 0) continue;
            granny_transform s;
            SampleTrack(c->tg->TransformTracks[ti], t, sk->Bones[b].LocalTransform, s);
            granny_transform& a = acc[b];
            if (wsum[b] > 0) {
                float d = a.Orientation[0] * s.Orientation[0] + a.Orientation[1] * s.Orientation[1] +
                          a.Orientation[2] * s.Orientation[2] + a.Orientation[3] * s.Orientation[3];
                if (d < 0) for (int i = 0; i < 4; ++i) s.Orientation[i] = -s.Orientation[i];
            }
            for (int i = 0; i < 3; ++i) a.Position[i] += s.Position[i] * w;
            for (int i = 0; i < 4; ++i) a.Orientation[i] += s.Orientation[i] * w;
            for (int i = 0; i < 3; ++i) for (int j = 0; j < 3; ++j) a.ScaleShear[i][j] += s.ScaleShear[i][j] * w;
            wsum[b] += w;
        }
    }
    for (int b = 0; b < boneCount; ++b) {
        if (wsum[b] <= 1e-6f) { local[b] = sk->Bones[b].LocalTransform; continue; }
        granny_transform& a = acc[b];
        float inv = 1.f / wsum[b];
        for (int i = 0; i < 3; ++i) a.Position[i] *= inv;
        for (int i = 0; i < 3; ++i) for (int j = 0; j < 3; ++j) a.ScaleShear[i][j] *= inv;
        float l = sqrtf(a.Orientation[0] * a.Orientation[0] + a.Orientation[1] * a.Orientation[1] +
                        a.Orientation[2] * a.Orientation[2] + a.Orientation[3] * a.Orientation[3]);
        if (l > 1e-8f) for (int i = 0; i < 4; ++i) a.Orientation[i] /= l;
        else { a.Orientation[0] = a.Orientation[1] = a.Orientation[2] = 0; a.Orientation[3] = 1; }
        a.Flags = 7;
        local[b] = a;
    }
}

void BuildWorldPose(const granny_skeleton* sk, int boneCount, const std::vector<granny_transform>& local,
                    const float* offset, GrnWorldPose* wp)
{
    if ((int)wp->world.size() < boneCount) { wp->world.resize(boneCount); wp->composite.resize(boneCount); }
    for (int b = 0; b < boneCount; ++b) {
        float m[16];
        TransformToMatrix(local[b], m);
        int p = sk->Bones[b].ParentIndex;
        if (p >= 0 && p < b)
            MatMul(m, wp->world[p], wp->world[b]);
        else if (offset)
            MatMul(m, offset, wp->world[b]);
        else
            memcpy(wp->world[b].m, m, sizeof(m));
        MatMul(&sk->Bones[b].InverseWorld4x4[0][0], wp->world[b], wp->composite[b]);
    }
}

bool FindBone(const granny_skeleton* sk, const char* name, granny_int32x* idx)
{
    if (!sk || !name) return false;
    for (int i = 0; i < sk->BoneCount; ++i)
        if (sk->Bones[i].Name && strcmp(sk->Bones[i].Name, name) == 0) { *idx = i; return true; }
    for (int i = 0; i < sk->BoneCount; ++i)
        if (sk->Bones[i].Name && strcasecmp(sk->Bones[i].Name, name) == 0) { *idx = i; return true; }
    return false;
}

int MemberOffset(const TDef* d, const char* name, const TDef** out)
{
    int off = 0;
    for (const TDef* m = d; m && m->Type != GrannyEndMember; ++m) {
        if (m->Name && strcmp(m->Name, name) == 0) { *out = m; return off; }
        off += MemberSize(m, NP);
    }
    *out = 0;
    return -1;
}

}  // namespace

static TDef s_PNT332VertexType[] = {
    GM(Real32, GrannyVertexPositionName, 0, 3), GM(Real32, GrannyVertexNormalName, 0, 3),
    GM(Real32, "TextureCoordinates0", 0, 2), GEND };

extern "C" {
GRANNY_DYNLINKDATA(granny_data_type_definition*) GrannyPNT332VertexType = s_PNT332VertexType;

GRANNY_DYNLINK(granny_file*) GrannyReadEntireFileFromMemory(granny_int32x MemorySize, void const* Memory)
{
    if (!Memory || MemorySize <= 0) return 0;
    return (granny_file*)LoadFile((const uint8_t*)Memory, (size_t)MemorySize);
}

GRANNY_DYNLINK(granny_file_info*) GrannyGetFileInfo(granny_file* File)
{
    return File ? ((GrnFile*)File)->info : 0;
}

GRANNY_DYNLINK(void) GrannyFreeFile(granny_file* File) { delete (GrnFile*)File; }
GRANNY_DYNLINK(void) GrannyFreeFileSection(granny_file*, granny_int32x) {}

GRANNY_DYNLINK(granny_int32x) GrannyGetTotalTypeSize(granny_data_type_definition const* TypeDefinition)
{
    return TypeSize(TypeDefinition, NP);
}

GRANNY_DYNLINK(void) GrannyConvertSingleObject(granny_data_type_definition const* SourceType, void const* SourceObject,
                                               granny_data_type_definition const* DestType, void* DestObject,
                                               granny_conversion_handler*)
{
    if (!SourceType || !SourceObject || !DestType || !DestObject) return;
    ConvertNative(SourceType, (const uint8_t*)SourceObject, DestType, (uint8_t*)DestObject);
}

GRANNY_DYNLINK(bool) GrannyFindMatchingMember(granny_data_type_definition const* SourceType, void const* SourceObject,
                                              char const* DestMemberName, granny_variant* Result)
{
    if (Result) { Result->Type = 0; Result->Object = 0; }
    if (!SourceType || !DestMemberName) return false;
    int off = 0;
    const TDef* m = FindMember(SourceType, DestMemberName, NP, &off);
    if (!m) return false;
    if (Result) {
        Result->Type = (granny_data_type_definition*)m;
        Result->Object = SourceObject ? (uint8_t*)SourceObject + off : 0;
    }
    return true;
}

GRANNY_DYNLINK(granny_texture*) GrannyGetMaterialTextureByType(granny_material const* Material, granny_material_texture_type Type)
{
    if (!Material) return 0;
    const char* want = Type == GrannyDiffuseColorTexture ? "Diffuse" : Type == GrannyOpacityTexture ? "Opacity" :
                       Type == GrannySpecularColorTexture ? "Specular" : Type == GrannyBumpHeightTexture ? "Bump" :
                       Type == GrannySelfIlluminationTexture ? "Self" : Type == GrannyAmbientColorTexture ? "Ambient" :
                       Type == GrannyReflectionTexture ? "Reflection" : 0;
    if (want) {
        for (int i = 0; i < Material->MapCount; ++i) {
            const granny_material_map& mm = Material->Maps[i];
            if (mm.Usage && strcasestr(mm.Usage, want) && mm.Material) {
                if (mm.Material->Texture) return mm.Material->Texture;
                granny_texture* t = GrannyGetMaterialTextureByType(mm.Material, GrannyDiffuseColorTexture);
                if (t) return t;
            }
        }
    }
    if (Type == GrannyDiffuseColorTexture) return Material->Texture;
    return 0;
}

GRANNY_DYNLINK(bool) GrannyFindBoneByName(granny_skeleton const* Skeleton, char const* BoneName, granny_int32x* BoneIndex)
{
    return FindBone(Skeleton, BoneName, BoneIndex);
}

// ---- meshes
GRANNY_DYNLINK(granny_int32x) GrannyGetMeshVertexCount(granny_mesh const* Mesh)
{
    return Mesh && Mesh->PrimaryVertexData ? Mesh->PrimaryVertexData->VertexCount : 0;
}

GRANNY_DYNLINK(granny_data_type_definition*) GrannyGetMeshVertexType(granny_mesh const* Mesh)
{
    return Mesh && Mesh->PrimaryVertexData ? Mesh->PrimaryVertexData->VertexType : 0;
}

GRANNY_DYNLINK(void*) GrannyGetMeshVertices(granny_mesh const* Mesh)
{
    return Mesh && Mesh->PrimaryVertexData ? Mesh->PrimaryVertexData->Vertices : 0;
}

GRANNY_DYNLINK(granny_int32x) GrannyGetMeshIndexCount(granny_mesh const* Mesh)
{
    if (!Mesh || !Mesh->PrimaryTopology) return 0;
    return Mesh->PrimaryTopology->IndexCount ? Mesh->PrimaryTopology->IndexCount : Mesh->PrimaryTopology->Index16Count;
}

GRANNY_DYNLINK(granny_int32x) GrannyGetMeshTriangleGroupCount(granny_mesh const* Mesh)
{
    return Mesh && Mesh->PrimaryTopology ? Mesh->PrimaryTopology->GroupCount : 0;
}

GRANNY_DYNLINK(granny_tri_material_group*) GrannyGetMeshTriangleGroups(granny_mesh const* Mesh)
{
    return Mesh && Mesh->PrimaryTopology ? Mesh->PrimaryTopology->Groups : 0;
}

GRANNY_DYNLINK(void) GrannyCopyMeshIndices(granny_mesh const* Mesh, granny_int32x BytesPerIndex, void* DestIndices)
{
    if (!Mesh || !Mesh->PrimaryTopology || !DestIndices) return;
    const granny_tri_topology* t = Mesh->PrimaryTopology;
    int n = GrannyGetMeshIndexCount(Mesh);
    for (int i = 0; i < n; ++i) {
        uint32_t v = t->IndexCount ? (uint32_t)t->Indices[i] : t->Indices16[i];
        if (BytesPerIndex == 2) ((uint16_t*)DestIndices)[i] = (uint16_t)v;
        else if (BytesPerIndex == 1) ((uint8_t*)DestIndices)[i] = (uint8_t)v;
        else ((uint32_t*)DestIndices)[i] = v;
    }
}

GRANNY_DYNLINK(void) GrannyCopyMeshVertices(granny_mesh const* Mesh, granny_data_type_definition const* VertexType, void* DestVertices)
{
    if (!Mesh || !Mesh->PrimaryVertexData || !VertexType || !DestVertices) return;
    const granny_vertex_data* vd = Mesh->PrimaryVertexData;
    int ss = TypeSize(vd->VertexType, NP), ds = TypeSize(VertexType, NP);
    for (int i = 0; i < vd->VertexCount; ++i)
        ConvertNative(vd->VertexType, vd->Vertices + (size_t)ss * i, VertexType, (uint8_t*)DestVertices + (size_t)ds * i);
}

GRANNY_DYNLINK(bool) GrannyMeshIsRigid(granny_mesh const* Mesh)
{
    if (!Mesh || !Mesh->PrimaryVertexData) return true;
    const TDef* m;
    return MemberOffset(Mesh->PrimaryVertexData->VertexType, GrannyVertexBoneIndicesName, &m) < 0;
}

GRANNY_DYNLINK(granny_mesh_binding*) GrannyNewMeshBinding(granny_mesh const* Mesh, granny_skeleton const*, granny_skeleton const* ToSkeleton)
{
    GrnMeshBinding* b = new GrnMeshBinding;
    int n = Mesh ? Mesh->BoneBindingCount : 0;
    b->toBone.resize(n > 0 ? n : 1, 0);
    for (int i = 0; i < n; ++i) {
        granny_int32x idx = 0;
        if (!FindBone(ToSkeleton, Mesh->BoneBindings[i].BoneName, &idx)) idx = 0;
        b->toBone[i] = idx;
    }
    return (granny_mesh_binding*)b;
}

GRANNY_DYNLINK(granny_int32x const*) GrannyGetMeshBindingToBoneIndices(granny_mesh_binding const* Binding)
{
    return Binding ? ((const GrnMeshBinding*)Binding)->toBone.data() : 0;
}

GRANNY_DYNLINK(void) GrannyFreeMeshBinding(granny_mesh_binding* Binding) { delete (GrnMeshBinding*)Binding; }

GRANNY_DYNLINK(granny_mesh_deformer*) GrannyNewMeshDeformer(granny_data_type_definition const* InputVertexLayout,
                                                            granny_data_type_definition const* OutputVertexLayout,
                                                            granny_deformation_type, granny_deformer_tail_flags)
{
    if (!InputVertexLayout || !OutputVertexLayout) return 0;
    GrnDeformer* d = new GrnDeformer;
    d->inStride = TypeSize(InputVertexLayout, NP);
    d->outStride = TypeSize(OutputVertexLayout, NP);
    const TDef* m;
    d->inPos = MemberOffset(InputVertexLayout, GrannyVertexPositionName, &m);
    d->inNrm = MemberOffset(InputVertexLayout, GrannyVertexNormalName, &m);
    d->inW = MemberOffset(InputVertexLayout, GrannyVertexBoneWeightsName, &m);
    if (m) { d->wType = m->Type; d->wCount = Width(m); }
    d->inI = MemberOffset(InputVertexLayout, GrannyVertexBoneIndicesName, &m);
    if (m) { d->iType = m->Type; d->iCount = Width(m); }
    d->outPos = MemberOffset(OutputVertexLayout, GrannyVertexPositionName, &m);
    d->outNrm = MemberOffset(OutputVertexLayout, GrannyVertexNormalName, &m);
    int off = 0;
    for (const TDef* om = OutputVertexLayout; om->Type != GrannyEndMember; ++om) {
        if (om->Name && strcmp(om->Name, GrannyVertexPositionName) && strcmp(om->Name, GrannyVertexNormalName)) {
            const TDef* im;
            int io = MemberOffset(InputVertexLayout, om->Name, &im);
            if (io >= 0) d->copies.push_back({im, io, om, off});
        }
        off += MemberSize(om, NP);
    }
    if (d->inPos < 0 || d->outPos < 0 || d->inI < 0) { delete d; return 0; }
    return (granny_mesh_deformer*)d;
}

GRANNY_DYNLINK(void) GrannyFreeMeshDeformer(granny_mesh_deformer* Deformer) { delete (GrnDeformer*)Deformer; }

GRANNY_DYNLINK(void) GrannyDeformVertices(granny_mesh_deformer const* Deformer, granny_int32x const* MatrixIndices,
                                          granny_real32 const* MatrixBuffer4x4, granny_int32x VertexCount,
                                          void const* SourceVertices, void* DestVertices)
{
    const GrnDeformer* d = (const GrnDeformer*)Deformer;
    if (!d || !MatrixIndices || !MatrixBuffer4x4 || !SourceVertices || !DestVertices) return;
    for (int v = 0; v < VertexCount; ++v) {
        const uint8_t* s = (const uint8_t*)SourceVertices + (size_t)d->inStride * v;
        uint8_t* o = (uint8_t*)DestVertices + (size_t)d->outStride * v;
        float p[3], n[3] = {0, 0, 0}, rp[3] = {0, 0, 0}, rn[3] = {0, 0, 0};
        memcpy(p, s + d->inPos, 12);
        if (d->inNrm >= 0) memcpy(n, s + d->inNrm, 12);
        int cnt = d->iCount;
        float total = 0;
        for (int k = 0; k < cnt; ++k) {
            float w = d->inW >= 0 && k < d->wCount ? (float)ReadNum(d->wType, s + d->inW, k) : (k == 0 ? 1.f : 0.f);
            if (w <= 0) continue;
            int bi = (int)ReadNum(d->iType, s + d->inI, k);
            const float* M = MatrixBuffer4x4 + 16 * MatrixIndices[bi];
            for (int j = 0; j < 3; ++j) {
                rp[j] += w * (p[0] * M[0 * 4 + j] + p[1] * M[1 * 4 + j] + p[2] * M[2 * 4 + j] + M[12 + j]);
                rn[j] += w * (n[0] * M[0 * 4 + j] + n[1] * M[1 * 4 + j] + n[2] * M[2 * 4 + j]);
            }
            total += w;
        }
        if (total > 0 && fabsf(total - 1.f) > 1e-3f)
            for (int j = 0; j < 3; ++j) { rp[j] /= total; rn[j] /= total; }
        if (total <= 0) { memcpy(rp, p, 12); memcpy(rn, n, 12); }
        memcpy(o + d->outPos, rp, 12);
        if (d->outNrm >= 0) memcpy(o + d->outNrm, rn, 12);
        for (const auto& c : d->copies) {
            if (IsScalar(c.d->Type) && IsScalar(c.s->Type)) {
                int w = (std::min)(Width(c.d), Width(c.s));
                for (int i = 0; i < w; ++i) WriteNum(c.d->Type, o + c.dO, i, ReadNum(c.s->Type, s + c.so, i));
            }
        }
    }
}

// ---- model instances, poses, controls
GRANNY_DYNLINK(granny_model_instance*) GrannyInstantiateModel(granny_model const* Model)
{
    if (!Model || !Model->Skeleton) return 0;
    GrnInstance* i = new GrnInstance;
    i->model = Model;
    i->skel = Model->Skeleton;
    return (granny_model_instance*)i;
}

GRANNY_DYNLINK(void) GrannyFreeModelInstance(granny_model_instance* ModelInstance)
{
    GrnInstance* i = (GrnInstance*)ModelInstance;
    if (!i) return;
    for (GrnControl* c : i->controls) delete c;
    delete i;
}

GRANNY_DYNLINK(granny_skeleton*) GrannyGetSourceSkeleton(granny_model_instance const* Model)
{
    return Model ? ((const GrnInstance*)Model)->skel : 0;
}

GRANNY_DYNLINK(void) GrannySetModelClock(granny_model_instance const* ModelInstance, granny_real32 NewClock)
{
    if (ModelInstance) ((GrnInstance*)ModelInstance)->clock = NewClock;
}

GRANNY_DYNLINK(void) GrannyUpdateModelMatrix(granny_model_instance const* ModelInstance, granny_real32 SecondsElapsed,
                                             granny_real32 const* ModelMatrix4x4, granny_real32* DestMatrix4x4, bool)
{
    if (!ModelMatrix4x4 || !DestMatrix4x4) return;
    float m[16];
    memcpy(m, ModelMatrix4x4, 64);
    float d[3] = {0, 0, 0};
    const GrnInstance* inst = (const GrnInstance*)ModelInstance;
    if (inst) {
        for (const GrnControl* c : inst->controls) {
            if (!c->tg || !c->anim || !(c->tg->Flags & 3)) continue;
            float dur = c->anim->Duration;
            float w = Weight(c);
            if (dur <= 0 || w <= 0) continue;
            if (c->loopCount > 0 && LocalClock(c) >= dur * c->loopCount) continue;
            float k = w * SecondsElapsed * c->speed / dur;
            for (int i = 0; i < 3; ++i) d[i] += c->tg->LoopTranslation[i] * k;
        }
    }
    float out[16];
    memcpy(out, m, 64);
    for (int j = 0; j < 3; ++j) out[12 + j] = m[12 + j] + d[0] * m[j] + d[1] * m[4 + j] + d[2] * m[8 + j];
    memcpy(DestMatrix4x4, out, 64);
}

GRANNY_DYNLINK(granny_local_pose*) GrannyNewLocalPose(granny_int32x BoneCount)
{
    GrnLocalPose* p = new GrnLocalPose;
    p->t.resize(BoneCount > 0 ? BoneCount : 1);
    for (auto& t : p->t) Identity(t);
    return (granny_local_pose*)p;
}

GRANNY_DYNLINK(void) GrannyFreeLocalPose(granny_local_pose* LocalPose) { delete (GrnLocalPose*)LocalPose; }

GRANNY_DYNLINK(granny_world_pose*) GrannyNewWorldPose(granny_int32x BoneCount)
{
    GrnWorldPose* p = new GrnWorldPose;
    int n = BoneCount > 0 ? BoneCount : 1;
    p->world.resize(n);
    p->composite.resize(n);
    for (int b = 0; b < n; ++b) {
        memset(p->world[b].m, 0, 64);
        memset(p->composite[b].m, 0, 64);
        p->world[b].m[0] = p->world[b].m[5] = p->world[b].m[10] = p->world[b].m[15] = 1;
        p->composite[b].m[0] = p->composite[b].m[5] = p->composite[b].m[10] = p->composite[b].m[15] = 1;
    }
    return (granny_world_pose*)p;
}

GRANNY_DYNLINK(void) GrannyFreeWorldPose(granny_world_pose* WorldPose) { delete (GrnWorldPose*)WorldPose; }

GRANNY_DYNLINK(granny_real32*) GrannyGetWorldPose4x4(granny_world_pose const* WorldPose, granny_int32x BoneIndex)
{
    GrnWorldPose* p = (GrnWorldPose*)WorldPose;
    if (!p || BoneIndex < 0 || BoneIndex >= (int)p->world.size()) return p && !p->world.empty() ? p->world[0].m : 0;
    return p->world[BoneIndex].m;
}

GRANNY_DYNLINK(granny_real32*) GrannyGetWorldPoseComposite4x4(granny_world_pose const* WorldPose, granny_int32x BoneIndex)
{
    GrnWorldPose* p = (GrnWorldPose*)WorldPose;
    if (!p || BoneIndex < 0 || BoneIndex >= (int)p->composite.size()) return p && !p->composite.empty() ? p->composite[0].m : 0;
    return p->composite[BoneIndex].m;
}

GRANNY_DYNLINK(granny_matrix_4x4*) GrannyGetWorldPoseComposite4x4Array(granny_world_pose const* WorldPose)
{
    GrnWorldPose* p = (GrnWorldPose*)WorldPose;
    return p ? (granny_matrix_4x4*)p->composite.data() : 0;
}

GRANNY_DYNLINK(void) GrannySampleModelAnimationsAccelerated(granny_model_instance const* ModelInstance, granny_int32x BoneCount,
                                                           granny_real32 const* Offset4x4, granny_local_pose* Scratch,
                                                           granny_world_pose* Result)
{
    GrnInstance* inst = (GrnInstance*)ModelInstance;
    GrnWorldPose* wp = (GrnWorldPose*)Result;
    if (!inst || !wp || !inst->skel) return;
    int n = (std::min<int>)(BoneCount, inst->skel->BoneCount);
    if (n <= 0) return;
    std::vector<granny_transform> localTmp;
    std::vector<granny_transform>& local = Scratch ? ((GrnLocalPose*)Scratch)->t : localTmp;
    SampleLocalPose(inst, n, local);
    BuildWorldPose(inst->skel, n, local, Offset4x4, wp);
}

GRANNY_DYNLINK(granny_control*) GrannyPlayControlledAnimation(granny_real32 StartTime, granny_animation const* Animation,
                                                             granny_model_instance* Model)
{
    GrnInstance* inst = (GrnInstance*)Model;
    if (!inst || !Animation || Animation->TrackGroupCount <= 0) return 0;
    GrnControl* c = new GrnControl;
    c->inst = inst;
    c->anim = Animation;
    c->tg = Animation->TrackGroups[0];
    for (int g = 0; g < Animation->TrackGroupCount; ++g)
        if (Animation->TrackGroups[g] && inst->model->Name && NamesEqual(Animation->TrackGroups[g]->Name, inst->model->Name))
            c->tg = Animation->TrackGroups[g];
    int bc = inst->skel->BoneCount;
    c->boneTrack.assign(bc, -1);
    if (c->tg) {
        for (int t = 0; t < c->tg->TransformTrackCount; ++t) {
            granny_int32x b;
            if (FindBone(inst->skel, c->tg->TransformTracks[t].Name, &b) && c->boneTrack[b] < 0) c->boneTrack[b] = t;
        }
    }
    c->clockBase = StartTime;
    c->localBase = 0;
    inst->controls.push_back(c);
    return (granny_control*)c;
}

GRANNY_DYNLINK(void) GrannyFreeControl(granny_control* Control)
{
    if (Control) DestroyControl((GrnControl*)Control);
}

GRANNY_DYNLINK(bool) GrannyControlIsComplete(granny_control const* Control)
{
    return Control && IsComplete((const GrnControl*)Control);
}

GRANNY_DYNLINK(void) GrannyCompleteControlAt(granny_control* Control, granny_real32 AtSeconds)
{
    GrnControl* c = (GrnControl*)Control;
    if (!c) return;
    c->hasComplete = true;
    c->completeClock = AtSeconds;
}

GRANNY_DYNLINK(bool) GrannyFreeControlIfComplete(granny_control* Control)
{
    if (Control && IsComplete((GrnControl*)Control)) { DestroyControl((GrnControl*)Control); return true; }
    return false;
}

GRANNY_DYNLINK(void) GrannyFreeControlOnceUnused(granny_control* Control)
{
    if (Control) ((GrnControl*)Control)->freeOnceUnused = true;
}

GRANNY_DYNLINK(void) GrannyFreeCompletedModelControls(granny_model_instance const* ModelInstance)
{
    GrnInstance* inst = (GrnInstance*)ModelInstance;
    if (!inst) return;
    for (size_t i = 0; i < inst->controls.size();) {
        GrnControl* c = inst->controls[i];
        if (c->freeOnceUnused && IsComplete(c)) { inst->controls.erase(inst->controls.begin() + i); delete c; }
        else ++i;
    }
}

GRANNY_DYNLINK(granny_real32) GrannyGetControlLocalDuration(granny_control const* Control)
{
    return Control ? ((const GrnControl*)Control)->anim->Duration : 0;
}

GRANNY_DYNLINK(granny_int32x) GrannyGetControlLoopCount(granny_control const* Control)
{
    return Control ? ((const GrnControl*)Control)->loopCount : 0;
}

GRANNY_DYNLINK(void) GrannySetControlLoopCount(granny_control* Control, granny_int32x LoopCount)
{
    if (Control) ((GrnControl*)Control)->loopCount = LoopCount;
}

GRANNY_DYNLINK(granny_real32) GrannyGetControlRawLocalClock(granny_control* Control)
{
    return Control ? LocalClock((GrnControl*)Control) : 0;
}

GRANNY_DYNLINK(void) GrannySetControlRawLocalClock(granny_control* Control, granny_real32 LocalClockValue)
{
    GrnControl* c = (GrnControl*)Control;
    if (!c) return;
    c->localBase = LocalClockValue;
    c->clockBase = c->inst->clock;
}

GRANNY_DYNLINK(granny_real32) GrannyGetControlSpeed(granny_control const* Control)
{
    return Control ? ((const GrnControl*)Control)->speed : 0;
}

GRANNY_DYNLINK(void) GrannySetControlSpeed(granny_control* Control, granny_real32 Speed)
{
    GrnControl* c = (GrnControl*)Control;
    if (!c) return;
    c->localBase = LocalClock(c);
    c->clockBase = c->inst->clock;
    c->speed = Speed;
}

GRANNY_DYNLINK(void) GrannySetControlEaseIn(granny_control* Control, bool EaseIn)
{
    if (Control) ((GrnControl*)Control)->easeIn = EaseIn;
}

GRANNY_DYNLINK(void) GrannySetControlEaseOut(granny_control* Control, bool EaseOut)
{
    if (Control) ((GrnControl*)Control)->easeOut = EaseOut;
}

GRANNY_DYNLINK(void) GrannySetControlEaseInCurve(granny_control* Control, granny_real32 StartSeconds, granny_real32 EndSeconds,
                                                 granny_real32 StartValue, granny_real32 StartTangent, granny_real32 EndTangent,
                                                 granny_real32 EndValue)
{
    GrnControl* c = (GrnControl*)Control;
    if (!c) return;
    c->eiS = StartSeconds; c->eiE = EndSeconds;
    c->ei[0] = StartValue; c->ei[1] = StartTangent; c->ei[2] = EndTangent; c->ei[3] = EndValue;
}

GRANNY_DYNLINK(void) GrannySetControlEaseOutCurve(granny_control* Control, granny_real32 StartSeconds, granny_real32 EndSeconds,
                                                  granny_real32 StartValue, granny_real32 StartTangent, granny_real32 EndTangent,
                                                  granny_real32 EndValue)
{
    GrnControl* c = (GrnControl*)Control;
    if (!c) return;
    c->eoS = StartSeconds; c->eoE = EndSeconds;
    c->eo[0] = StartValue; c->eo[1] = StartTangent; c->eo[2] = EndTangent; c->eo[3] = EndValue;
}

GRANNY_DYNLINK(char const*) GrannyGetLogMessageOriginString(granny_log_message_origin Origin)
{
    static const char* const names[] = { "NotImplemented", "Application", "FileReading", "FileWriting", "Memory",
                                         "MeshBinding", "Control", "Animation", "Deformer" };
    return (unsigned)Origin < sizeof(names) / sizeof(names[0]) ? names[Origin] : "Unknown";
}

GRANNY_DYNLINK(char const*) GrannyGetLogMessageTypeString(granny_log_message_type Type)
{
    static const char* const names[] = { "Ignored", "Note", "Warning", "Error" };
    return (unsigned)Type < sizeof(names) / sizeof(names[0]) ? names[Type] : "Unknown";
}

GRANNY_DYNLINK(void) GrannySetLogCallback(granny_log_callback const* LogCallback)
{
    if (LogCallback) g_log = *LogCallback;
    else g_log.Function = 0, g_log.UserData = 0;
}
}

namespace {
void ConvertNative(const TDef* sd, const uint8_t* sp, const TDef* dd, uint8_t* dp)
{
    const bool singles = MemberCount(sd) == 1 && MemberCount(dd) == 1;
    int doff = 0;
    for (const TDef* dm = dd; dm->Type != GrannyEndMember; ++dm) {
        int soff = 0;
        const TDef* sm = singles ? sd : FindMember(sd, dm->Name, NP, &soff);
        if (sm) {
            if (IsScalar(dm->Type) && IsScalar(sm->Type)) {
                int n = (std::min)(Width(dm), Width(sm));
                for (int i = 0; i < n; ++i) WriteNum(dm->Type, dp + doff, i, ReadNum(sm->Type, sp + soff, i));
            } else if (dm->Type == sm->Type && dm->Type == GrannyInlineMember) {
                ConvertNative(sm->ReferenceType, sp + soff, dm->ReferenceType, dp + doff);
            } else if (dm->Type == sm->Type) {
                memcpy(dp + doff, sp + soff, (std::min)(MemberSize(dm, NP), MemberSize(sm, NP)));
            }
        }
        doff += MemberSize(dm, NP);
    }
}
}  // namespace
