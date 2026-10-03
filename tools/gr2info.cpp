// gr2info: print what OpenGr2ndma reads from .gr2 files.
//   gr2info file.gr2...         summary per file
//   gr2info --check <dir>       load every .gr2 under dir, fail if any fails
#include <granny.h>

#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <string>
#include <sys/stat.h>
#include <vector>

static bool ReadFile(const std::string& path, std::vector<char>& out)
{
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    out.resize(n > 0 ? n : 0);
    bool ok = n > 0 && fread(out.data(), 1, n, f) == (size_t)n;
    fclose(f);
    return ok;
}

static bool Inspect(const std::string& path, bool verbose)
{
    std::vector<char> data;
    if (!ReadFile(path, data)) { fprintf(stderr, "%s: cannot read\n", path.c_str()); return false; }
    granny_file* file = GrannyReadEntireFileFromMemory((granny_int32x)data.size(), data.data());
    granny_file_info* info = file ? GrannyGetFileInfo(file) : 0;
    if (!info) { fprintf(stderr, "%s: failed to load\n", path.c_str()); if (file) GrannyFreeFile(file); return false; }

    bool ok = true;
    for (int m = 0; m < info->ModelCount; ++m) {
        granny_model* model = info->Models[m];
        granny_model_instance* inst = GrannyInstantiateModel(model);
        if (!inst) ok = false;
        else GrannyFreeModelInstance(inst);
    }
    if (verbose) {
        printf("%s\n", path.c_str());
        printf("  textures %d  materials %d  skeletons %d  meshes %d  models %d  animations %d\n",
               info->TextureCount, info->MaterialCount, info->SkeletonCount, info->MeshCount,
               info->ModelCount, info->AnimationCount);
        for (int i = 0; i < info->SkeletonCount; ++i)
            printf("  skeleton '%s': %d bones\n", info->Skeletons[i]->Name ? info->Skeletons[i]->Name : "",
                   info->Skeletons[i]->BoneCount);
        for (int i = 0; i < info->MeshCount; ++i) {
            granny_mesh* mesh = info->Meshes[i];
            printf("  mesh '%s': %d vertices, %d indices, %d groups, %s\n", mesh->Name ? mesh->Name : "",
                   GrannyGetMeshVertexCount(mesh), GrannyGetMeshIndexCount(mesh),
                   GrannyGetMeshTriangleGroupCount(mesh), GrannyMeshIsRigid(mesh) ? "rigid" : "skinned");
        }
        for (int i = 0; i < info->AnimationCount; ++i)
            printf("  animation '%s': %.3fs, %d track groups\n", info->Animations[i]->Name ? info->Animations[i]->Name : "",
                   info->Animations[i]->Duration, info->Animations[i]->TrackGroupCount);
    }
    GrannyFreeFile(file);
    return ok;
}

static void Walk(const std::string& dir, std::vector<std::string>& out)
{
    DIR* d = opendir(dir.c_str());
    if (!d) return;
    while (dirent* e = readdir(d)) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        std::string p = dir + "/" + e->d_name;
        struct stat st;
        if (stat(p.c_str(), &st)) continue;
        if (S_ISDIR(st.st_mode)) Walk(p, out);
        else if (p.size() > 4 && !strcasecmp(p.c_str() + p.size() - 4, ".gr2")) out.push_back(p);
    }
    closedir(d);
}

int main(int argc, char** argv)
{
    if (argc < 2) { fprintf(stderr, "usage: gr2info file.gr2... | gr2info --check <dir>\n"); return 2; }
    if (!strcmp(argv[1], "--check")) {
        if (argc < 3) return 2;
        std::vector<std::string> files;
        Walk(argv[2], files);
        int failed = 0;
        for (size_t i = 0; i < files.size(); ++i)
            if (!Inspect(files[i], false)) ++failed;
        printf("%zu files, %d failed\n", files.size(), failed);
        return files.empty() || failed ? 1 : 0;
    }
    int failed = 0;
    for (int i = 1; i < argc; ++i)
        if (!Inspect(argv[i], true)) ++failed;
    return failed ? 1 : 0;
}
