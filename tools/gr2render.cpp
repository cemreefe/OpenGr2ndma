// gr2render: software-rasterises the first model of a .gr2 file, posed by its first
// animation, into binary PPM frames. Used for the README images and as a smoke test
// of loading, skinning and animation sampling without any GPU or windowing code.
//   gr2render model.gr2 out-prefix [frames=1] [size=320] [yaw-degrees=30]
#include <granny.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

struct Vtx { float p[3], n[3], uv[2]; };

static bool ReadFile(const char* path, std::vector<char>& out)
{
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    out.resize(n > 0 ? n : 0);
    bool ok = n > 0 && fread(out.data(), 1, n, f) == (size_t)n;
    fclose(f);
    return ok;
}

static void Transform(const float* m, const float* p, float w, float* out)
{
    for (int j = 0; j < 3; ++j) out[j] = p[0] * m[j] + p[1] * m[4 + j] + p[2] * m[8 + j] + w * m[12 + j];
}

struct Posed { std::vector<Vtx> v; std::vector<int> idx; };

static void Pose(granny_model* model, granny_model_instance* inst, granny_world_pose* world, std::vector<Posed>& out)
{
    granny_skeleton* sk = GrannyGetSourceSkeleton(inst);
    out.clear();
    for (int b = 0; b < model->MeshBindingCount; ++b) {
        granny_mesh* mesh = model->MeshBindings[b].Mesh;
        int vc = GrannyGetMeshVertexCount(mesh);
        Posed pm;
        std::vector<Vtx> src(vc);
        pm.v.resize(vc);
        GrannyCopyMeshVertices(mesh, GrannyPNT332VertexType, src.data());
        granny_mesh_binding* bind = GrannyNewMeshBinding(mesh, sk, sk);
        const granny_int32x* toBone = GrannyGetMeshBindingToBoneIndices(bind);
        if (GrannyMeshIsRigid(mesh)) {
            const float* m = GrannyGetWorldPose4x4(world, toBone[0]);
            for (int i = 0; i < vc; ++i) {
                Transform(m, src[i].p, 1, pm.v[i].p);
                Transform(m, src[i].n, 0, pm.v[i].n);
                pm.v[i].uv[0] = src[i].uv[0];
                pm.v[i].uv[1] = src[i].uv[1];
            }
        } else {
            granny_mesh_deformer* def = GrannyNewMeshDeformer(GrannyGetMeshVertexType(mesh), GrannyPNT332VertexType,
                                                              GrannyDeformPositionNormal, GrannyDontAllowUncopiedTail);
            GrannyDeformVertices(def, toBone, (const granny_real32*)GrannyGetWorldPoseComposite4x4Array(world), vc,
                                 GrannyGetMeshVertices(mesh), pm.v.data());
            GrannyFreeMeshDeformer(def);
        }
        GrannyFreeMeshBinding(bind);
        pm.idx.resize(GrannyGetMeshIndexCount(mesh));
        GrannyCopyMeshIndices(mesh, 4, pm.idx.data());
        out.push_back(pm);
    }
}

int main(int argc, char** argv)
{
    if (argc < 3) { fprintf(stderr, "usage: gr2render model.gr2 out-prefix [frames] [size] [yaw]\n"); return 2; }
    int frames = argc > 3 ? atoi(argv[3]) : 1, size = argc > 4 ? atoi(argv[4]) : 320;
    float yaw = (argc > 5 ? (float)atof(argv[5]) : 30.f) * 3.14159265f / 180.f;
    std::vector<char> data;
    if (!ReadFile(argv[1], data)) { fprintf(stderr, "cannot read %s\n", argv[1]); return 1; }
    granny_file* file = GrannyReadEntireFileFromMemory((granny_int32x)data.size(), data.data());
    granny_file_info* info = file ? GrannyGetFileInfo(file) : 0;
    if (!info || info->ModelCount < 1) { fprintf(stderr, "no model in %s\n", argv[1]); return 1; }
    granny_model* model = info->Models[0];
    granny_model_instance* inst = GrannyInstantiateModel(model);
    int bones = model->Skeleton->BoneCount;
    granny_local_pose* local = GrannyNewLocalPose(bones);
    granny_world_pose* world = GrannyNewWorldPose(bones);
    float duration = 0;
    if (info->AnimationCount > 0) {
        granny_control* c = GrannyPlayControlledAnimation(0, info->Animations[0], inst);
        GrannySetControlLoopCount(c, 0);
        duration = info->Animations[0]->Duration;
    }

    // Fit the camera to the bind pose (sampled at t=0) so every frame shares one framing.
    std::vector<Posed> posed;
    GrannySetModelClock(inst, 0);
    GrannySampleModelAnimationsAccelerated(inst, bones, 0, local, world);
    Pose(model, inst, world, posed);
    float lo[3] = {1e30f, 1e30f, 1e30f}, hi[3] = {-1e30f, -1e30f, -1e30f};
    for (auto& pm : posed)
        for (auto& v : pm.v)
            for (int j = 0; j < 3; ++j) { lo[j] = std::min(lo[j], v.p[j]); hi[j] = std::max(hi[j], v.p[j]); }
    float c[3], ext = 0;
    for (int j = 0; j < 3; ++j) { c[j] = (lo[j] + hi[j]) / 2; ext = std::max(ext, hi[j] - lo[j]); }
    // Granny files here are Y-up or Z-up; treat the longest of Y/Z as "up".
    bool zUp = (hi[2] - lo[2]) > (hi[1] - lo[1]) * 1.2f;
    float scale = size * 0.62f / (ext > 0 ? ext : 1);

    std::vector<unsigned char> img(size * size * 3);
    std::vector<float> zb(size * size);
    for (int f = 0; f < frames; ++f) {
        float t = frames > 1 && duration > 0 ? duration * f / frames : 0;
        GrannySetModelClock(inst, t);
        GrannySampleModelAnimationsAccelerated(inst, bones, 0, local, world);
        Pose(model, inst, world, posed);
        for (int y = 0; y < size; ++y)
            for (int x = 0; x < size; ++x) {
                float g = 1.f - 0.35f * y / size;
                unsigned char* px = &img[(y * size + x) * 3];
                px[0] = (unsigned char)(250 * g); px[1] = (unsigned char)(238 * g); px[2] = (unsigned char)(222 * g);
            }
        std::fill(zb.begin(), zb.end(), 1e30f);
        const float L[3] = {-0.45f, 0.75f, 0.5f};
        for (auto& pm : posed) {
            std::vector<float> sx(pm.v.size()), sy(pm.v.size()), sz(pm.v.size());
            std::vector<float> shade(pm.v.size());
            for (size_t i = 0; i < pm.v.size(); ++i) {
                float p[3] = {pm.v[i].p[0] - c[0], pm.v[i].p[1] - c[1], pm.v[i].p[2] - c[2]};
                float n[3] = {pm.v[i].n[0], pm.v[i].n[1], pm.v[i].n[2]};
                if (zUp) { std::swap(p[1], p[2]); p[2] = -p[2]; std::swap(n[1], n[2]); n[2] = -n[2]; }
                float rx = p[0] * cosf(yaw) + p[2] * sinf(yaw), rz = -p[0] * sinf(yaw) + p[2] * cosf(yaw);
                float nx = n[0] * cosf(yaw) + n[2] * sinf(yaw), nz = -n[0] * sinf(yaw) + n[2] * cosf(yaw);
                sx[i] = size / 2 + rx * scale;
                sy[i] = size / 2 - p[1] * scale;
                sz[i] = -rz;
                float nl = sqrtf(nx * nx + n[1] * n[1] + nz * nz);
                float d = nl > 0 ? (nx * L[0] + n[1] * L[1] - nz * L[2]) / nl : 0;
                shade[i] = 0.35f + 0.65f * std::max(0.f, d) / 1.0f;
            }
            for (size_t k = 0; k + 2 < pm.idx.size(); k += 3) {
                int a = pm.idx[k], b = pm.idx[k + 1], e = pm.idx[k + 2];
                float area = (sx[b] - sx[a]) * (sy[e] - sy[a]) - (sx[e] - sx[a]) * (sy[b] - sy[a]);
                if (fabsf(area) < 1e-6f) continue;
                int x0 = std::max(0, (int)floorf(std::min({sx[a], sx[b], sx[e]})));
                int x1 = std::min(size - 1, (int)ceilf(std::max({sx[a], sx[b], sx[e]})));
                int y0 = std::max(0, (int)floorf(std::min({sy[a], sy[b], sy[e]})));
                int y1 = std::min(size - 1, (int)ceilf(std::max({sy[a], sy[b], sy[e]})));
                for (int y = y0; y <= y1; ++y)
                    for (int x = x0; x <= x1; ++x) {
                        float px = x + 0.5f, py = y + 0.5f;
                        float w0 = ((sx[b] - px) * (sy[e] - py) - (sx[e] - px) * (sy[b] - py)) / area;
                        float w1 = ((sx[e] - px) * (sy[a] - py) - (sx[a] - px) * (sy[e] - py)) / area;
                        float w2 = 1 - w0 - w1;
                        if (w0 < 0 || w1 < 0 || w2 < 0) continue;
                        float z = w0 * sz[a] + w1 * sz[b] + w2 * sz[e];
                        if (z >= zb[y * size + x]) continue;
                        zb[y * size + x] = z;
                        float s = w0 * shade[a] + w1 * shade[b] + w2 * shade[e];
                        float v = w0 * pm.v[a].uv[1] + w1 * pm.v[b].uv[1] + w2 * pm.v[e].uv[1];
                        float u = w0 * pm.v[a].uv[0] + w1 * pm.v[b].uv[0] + w2 * pm.v[e].uv[0];
                        // Two-tone yarn stripes from the texture coordinates.
                        bool stripe = fmodf(fabsf(v * 24.f + u * 2.f), 2.f) < 1.f;
                        float col[3] = {stripe ? 0.80f : 0.93f, stripe ? 0.27f : 0.80f, stripe ? 0.36f : 0.62f};
                        unsigned char* o = &img[(y * size + x) * 3];
                        for (int j = 0; j < 3; ++j) o[j] = (unsigned char)std::min(255.f, 255 * col[j] * s);
                    }
            }
        }
        char name[512];
        snprintf(name, sizeof(name), "%s%03d.ppm", argv[2], f);
        FILE* out = fopen(name, "wb");
        if (!out) { fprintf(stderr, "cannot write %s\n", name); return 1; }
        fprintf(out, "P6\n%d %d\n255\n", size, size);
        fwrite(img.data(), 1, img.size(), out);
        fclose(out);
    }
    GrannyFreeWorldPose(world);
    GrannyFreeLocalPose(local);
    GrannyFreeModelInstance(inst);
    GrannyFreeFile(file);
    return 0;
}
