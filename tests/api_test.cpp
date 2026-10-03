// Asset-free tests for the OpenGr2ndma public API.
#include <granny.h>

#include <cmath>
#include <cstdio>
#include <cstring>

static int g_failures = 0;
#define CHECK(cond) do { if (!(cond)) { fprintf(stderr, "%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #cond); ++g_failures; } } while (0)

struct Src { granny_int32 TwoSided; granny_real32 Shine; granny_uint8 Flags[4]; };
struct Dst { granny_real32 Shine; granny_int32 TwoSided; };

static int g_logs = 0;
static void Log(granny_log_message_type, granny_log_message_origin, char const*, granny_int32x, char const*, void*) { ++g_logs; }

int main()
{
    granny_data_type_definition srcType[] = {
        {GrannyInt32Member, "Two-sided"},
        {GrannyReal32Member, "Shininess"},
        {GrannyUInt8Member, "Flags", 0, 4},
        {GrannyEndMember},
    };
    granny_data_type_definition dstType[] = {
        {GrannyReal32Member, "Shininess"},
        {GrannyInt32Member, "Two-sided"},
        {GrannyEndMember},
    };

    CHECK(GrannyGetTotalTypeSize(srcType) == 12);
    CHECK(GrannyGetTotalTypeSize(dstType) == 8);
    CHECK(GrannyGetTotalTypeSize(GrannyPNT332VertexType) == 32);

    Src s = {1, 0.5f, {1, 2, 3, 4}};
    granny_variant v;
    CHECK(GrannyFindMatchingMember(srcType, &s, "Shininess", &v));
    CHECK(v.Object == &s.Shine);
    CHECK(!GrannyFindMatchingMember(srcType, &s, "Missing", &v) && v.Object == 0);

    Dst d = {0, 0};
    GrannyConvertSingleObject(srcType, &s, dstType, &d, 0);
    CHECK(d.TwoSided == 1 && d.Shine == 0.5f);

    granny_int32 twoSided = 0;
    granny_data_type_definition single[] = {{GrannyInt32Member, "Two-sided"}, {GrannyEndMember}};
    CHECK(GrannyFindMatchingMember(srcType, &s, "Two-sided", &v));
    GrannyConvertSingleObject(v.Type, v.Object, single, &twoSided, 0);
    CHECK(twoSided == 1);

    granny_log_callback cb = {Log, 0};
    GrannySetLogCallback(&cb);
    const char junk[128] = "not a granny file";
    CHECK(GrannyReadEntireFileFromMemory(sizeof(junk), junk) == 0);
    CHECK(GrannyGetFileInfo(0) == 0);
    CHECK(g_logs > 0);
    GrannySetLogCallback(0);
    CHECK(!strcmp(GrannyGetLogMessageTypeString(GrannyErrorLogMessage), "Error"));
    CHECK(!strcmp(GrannyGetLogMessageOriginString(GrannyFileReadingLogMessage), "FileReading"));

    granny_world_pose* wp = GrannyNewWorldPose(3);
    const granny_real32* m = GrannyGetWorldPose4x4(wp, 2);
    CHECK(m && m[0] == 1 && m[5] == 1 && m[10] == 1 && m[15] == 1 && m[1] == 0 && m[12] == 0);
    CHECK(GrannyGetWorldPoseComposite4x4Array(wp) != 0);
    GrannyFreeWorldPose(wp);

    if (g_failures) { fprintf(stderr, "%d failures\n", g_failures); return 1; }
    printf("all tests passed\n");
    return 0;
}
