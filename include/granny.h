/*
 * OpenGr2ndma - open-source runtime for Granny 3D (.gr2) files.
 *
 * Source-compatible with the subset of the Granny 2.11 C API that game
 * clients of the early 2000s typically use: file loading, the in-memory
 * file-info structures, mesh access and skinning, skeleton world poses,
 * and controlled animation playback.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#ifndef OPENGR2NDMA_GRANNY_H
#define OPENGR2NDMA_GRANNY_H
#define GRANNY_H

#include <stdint.h>
#include <stddef.h>

#define OPENGR2NDMA_VERSION_MAJOR 0
#define OPENGR2NDMA_VERSION_MINOR 1

/* API level this header mirrors. Client code commonly branches on it. */
#define GrannyProductMajorVersion 2
#define GrannyProductMinorVersion 11

#ifdef __cplusplus
#define GRANNY_EXTERN_C extern "C"
#else
#define GRANNY_EXTERN_C
#endif
#define GRANNY_DYNLINK(ret) ret
#define GRANNY_DYNLINKDATA(type) type
#define GRANNY_CALLBACK(ret) ret

#ifndef __cplusplus
#include <stdbool.h>
#endif

typedef float    granny_real32;
typedef int8_t   granny_int8;
typedef uint8_t  granny_uint8;
typedef int16_t  granny_int16;
typedef uint16_t granny_uint16;
typedef int32_t  granny_int32;
typedef uint32_t granny_uint32;
typedef int32_t  granny_int32x;
typedef uint32_t granny_uint32x;
typedef int32_t  granny_bool32;
typedef uintptr_t granny_uintaddrx;
typedef granny_real32 granny_triple[3];
typedef granny_real32 granny_quad[4];
typedef granny_real32 granny_matrix_3x3[3][3];
typedef granny_real32 granny_matrix_4x4[4][4];

/* Member type ids, as stored in .gr2 type definitions. */
typedef enum granny_member_type
{
    GrannyEndMember = 0,
    GrannyInlineMember = 1,
    GrannyReferenceMember = 2,
    GrannyReferenceToArrayMember = 3,
    GrannyArrayOfReferencesMember = 4,
    GrannyVariantReferenceMember = 5,
    GrannyReferenceToVariantArrayMember = 7,
    GrannyStringMember = 8,
    GrannyTransformMember = 9,
    GrannyReal32Member = 10,
    GrannyInt8Member = 11,
    GrannyUInt8Member = 12,
    GrannyBinormalInt8Member = 13,
    GrannyNormalUInt8Member = 14,
    GrannyInt16Member = 15,
    GrannyUInt16Member = 16,
    GrannyBinormalInt16Member = 17,
    GrannyNormalUInt16Member = 18,
    GrannyInt32Member = 19,
    GrannyUInt32Member = 20,
    GrannyReal16Member = 21,
    GrannyEmptyReferenceMember = 22,
    GrannyBool32Member = GrannyInt32Member
} granny_member_type;

/* Native structures are packed to 4 bytes with native-size pointers; every
 * structure below mirrors the member list of its .gr2 type definition. */
#pragma pack(push, 4)

typedef struct granny_data_type_definition
{
    granny_member_type Type;
    char const* Name;
    struct granny_data_type_definition* ReferenceType;
    granny_int32 ArrayWidth;
    granny_int32 Extra[3];
    granny_uintaddrx Reserved;
} granny_data_type_definition;

typedef struct granny_variant
{
    granny_data_type_definition* Type;
    void* Object;
} granny_variant;

typedef struct granny_transform
{
    granny_uint32 Flags;
    granny_triple Position;
    granny_quad Orientation;
    granny_matrix_3x3 ScaleShear;
} granny_transform;

typedef struct granny_art_tool_info
{
    char const* FromArtToolName;
    granny_int32 ArtToolMajorRevision;
    granny_int32 ArtToolMinorRevision;
    granny_int32 ArtToolPointerSize;
    granny_real32 UnitsPerMeter;
    granny_triple Origin;
    granny_triple RightVector;
    granny_triple UpVector;
    granny_triple BackVector;
    granny_variant ExtendedData;
} granny_art_tool_info;

typedef struct granny_exporter_info
{
    char const* ExporterName;
    granny_int32 ExporterMajorRevision;
    granny_int32 ExporterMinorRevision;
    granny_int32 ExporterCustomization;
    granny_int32 ExporterBuildNumber;
    granny_variant ExtendedData;
} granny_exporter_info;

typedef struct granny_pixel_layout
{
    granny_int32 BytesPerPixel;
    granny_int32 ShiftForComponent[4];
    granny_int32 BitsForComponent[4];
} granny_pixel_layout;

typedef struct granny_texture_mip_level
{
    granny_int32 Stride;
    granny_int32 PixelByteCount;
    granny_uint8* PixelBytes;
} granny_texture_mip_level;

typedef struct granny_texture_image
{
    granny_int32 MIPLevelCount;
    granny_texture_mip_level* MIPLevels;
} granny_texture_image;

typedef struct granny_texture
{
    char const* FromFileName;
    granny_int32 TextureType;
    granny_int32 Width;
    granny_int32 Height;
    granny_int32 Encoding;
    granny_int32 SubFormat;
    granny_pixel_layout Layout;
    granny_int32 ImageCount;
    granny_texture_image* Images;
    granny_variant ExtendedData;
} granny_texture;

struct granny_material;

typedef struct granny_material_map
{
    char const* Usage;
    struct granny_material* Material;
} granny_material_map;

typedef struct granny_material
{
    char const* Name;
    granny_int32 MapCount;
    granny_material_map* Maps;
    granny_texture* Texture;
    granny_variant ExtendedData;
} granny_material;

typedef struct granny_bone
{
    char const* Name;
    granny_int32 ParentIndex;
    granny_transform LocalTransform;
    granny_matrix_4x4 InverseWorld4x4;
    granny_real32 LODError;
    granny_variant ExtendedData;
} granny_bone;

typedef struct granny_skeleton
{
    char const* Name;
    granny_int32 BoneCount;
    granny_bone* Bones;
    granny_int32 LODType;
    granny_variant ExtendedData;
} granny_skeleton;

typedef struct granny_vertex_annotation_set
{
    char const* Name;
    granny_data_type_definition* VertexAnnotationType;
    granny_int32 VertexAnnotationCount;
    granny_uint8* VertexAnnotations;
    granny_int32 IndicesMapFromVertexToAnnotation;
    granny_int32 VertexAnnotationIndexCount;
    granny_int32* VertexAnnotationIndices;
} granny_vertex_annotation_set;

typedef struct granny_vertex_data
{
    granny_data_type_definition* VertexType;
    granny_int32 VertexCount;
    granny_uint8* Vertices;
    granny_int32 VertexComponentNameCount;
    char const** VertexComponentNames;
    granny_int32 VertexAnnotationSetCount;
    granny_vertex_annotation_set* VertexAnnotationSets;
} granny_vertex_data;

typedef struct granny_tri_material_group
{
    granny_int32 MaterialIndex;
    granny_int32 TriFirst;
    granny_int32 TriCount;
} granny_tri_material_group;

typedef struct granny_tri_annotation_set
{
    char const* Name;
    granny_data_type_definition* TriAnnotationType;
    granny_int32 TriAnnotationCount;
    granny_uint8* TriAnnotations;
    granny_int32 IndicesMapFromTriToAnnotation;
    granny_int32 TriAnnotationIndexCount;
    granny_int32* TriAnnotationIndices;
} granny_tri_annotation_set;

typedef struct granny_tri_topology
{
    granny_int32 GroupCount;
    granny_tri_material_group* Groups;
    granny_int32 IndexCount;
    granny_int32* Indices;
    granny_int32 Index16Count;
    granny_uint16* Indices16;
    granny_int32 VertexToVertexCount;
    granny_int32* VertexToVertexMap;
    granny_int32 VertexToTriangleCount;
    granny_int32* VertexToTriangleMap;
    granny_int32 SideToNeighborCount;
    granny_uint32* SideToNeighborMap;
    granny_int32 PolygonIndexStartCount;
    granny_int32* PolygonIndexStarts;
    granny_int32 PolygonIndexCount;
    granny_int32* PolygonIndices;
    granny_int32 BonesForTriangleCount;
    granny_int32* BonesForTriangle;
    granny_int32 TriangleToBoneCount;
    granny_int32* TriangleToBoneIndices;
    granny_int32 TriAnnotationSetCount;
    granny_tri_annotation_set* TriAnnotationSets;
} granny_tri_topology;

typedef struct granny_morph_target
{
    char const* ScalarName;
    granny_vertex_data* VertexData;
    granny_int32 DataIsDeltas;
} granny_morph_target;

typedef struct granny_material_binding
{
    granny_material* Material;
} granny_material_binding;

typedef struct granny_bone_binding
{
    char const* BoneName;
    granny_triple OBBMin;
    granny_triple OBBMax;
    granny_int32 TriangleCount;
    granny_int32* TriangleIndices;
} granny_bone_binding;

typedef struct granny_mesh
{
    char const* Name;
    granny_vertex_data* PrimaryVertexData;
    granny_int32 MorphTargetCount;
    granny_morph_target* MorphTargets;
    granny_tri_topology* PrimaryTopology;
    granny_int32 MaterialBindingCount;
    granny_material_binding* MaterialBindings;
    granny_int32 BoneBindingCount;
    granny_bone_binding* BoneBindings;
    granny_variant ExtendedData;
} granny_mesh;

typedef struct granny_model_mesh_binding
{
    granny_mesh* Mesh;
} granny_model_mesh_binding;

typedef struct granny_model
{
    char const* Name;
    granny_skeleton* Skeleton;
    granny_transform InitialPlacement;
    granny_int32 MeshBindingCount;
    granny_model_mesh_binding* MeshBindings;
    granny_variant ExtendedData;
} granny_model;

/* Curve stored as knots plus control points (pre-2.7 layout). */
typedef struct granny_old_curve
{
    granny_int32 Degree;
    granny_int32 KnotCount;
    granny_real32* Knots;
    granny_int32 ControlCount;
    granny_real32* Controls;
} granny_old_curve;

typedef struct granny_curve2
{
    granny_variant CurveData;
} granny_curve2;

typedef struct granny_transform_track
{
    char const* Name;
    granny_int32 Flags;
    granny_curve2 OrientationCurve;
    granny_curve2 PositionCurve;
    granny_curve2 ScaleShearCurve;
} granny_transform_track;

typedef struct granny_vector_track
{
    char const* Name;
    granny_uint32 TrackKey;
    granny_int32 Dimension;
    granny_curve2 ValueCurve;
} granny_vector_track;

typedef struct granny_text_track_entry
{
    granny_real32 TimeStamp;
    char const* Text;
} granny_text_track_entry;

typedef struct granny_text_track
{
    char const* Name;
    granny_int32 EntryCount;
    granny_text_track_entry* Entries;
} granny_text_track;

typedef struct granny_periodic_loop
{
    granny_real32 Radius;
    granny_real32 dAngle;
    granny_real32 dZ;
    granny_triple BasisX;
    granny_triple BasisY;
    granny_triple Axis;
} granny_periodic_loop;

typedef struct granny_track_group
{
    char const* Name;
    granny_int32 VectorTrackCount;
    granny_vector_track* VectorTracks;
    granny_int32 TransformTrackCount;
    granny_transform_track* TransformTracks;
    granny_int32 TransformLODErrorCount;
    granny_real32* TransformLODErrors;
    granny_int32 TextTrackCount;
    granny_text_track* TextTracks;
    granny_transform InitialPlacement;
    granny_int32 Flags; /* accumulation flags */
    granny_triple LoopTranslation;
    granny_periodic_loop* PeriodicLoop;
    granny_variant ExtendedData;
} granny_track_group;

typedef struct granny_animation
{
    char const* Name;
    granny_real32 Duration;
    granny_real32 TimeStep;
    granny_real32 Oversampling;
    granny_int32 TrackGroupCount;
    granny_track_group** TrackGroups;
    granny_int32 DefaultLoopCount;
    granny_int32 Flags;
    granny_variant ExtendedData;
} granny_animation;

typedef struct granny_file_info
{
    granny_art_tool_info* ArtToolInfo;
    granny_exporter_info* ExporterInfo;
    char const* FromFileName;
    granny_int32 TextureCount;
    granny_texture** Textures;
    granny_int32 MaterialCount;
    granny_material** Materials;
    granny_int32 SkeletonCount;
    granny_skeleton** Skeletons;
    granny_int32 VertexDataCount;
    granny_vertex_data** VertexDatas;
    granny_int32 TriTopologyCount;
    granny_tri_topology** TriTopologies;
    granny_int32 MeshCount;
    granny_mesh** Meshes;
    granny_int32 ModelCount;
    granny_model** Models;
    granny_int32 TrackGroupCount;
    granny_track_group** TrackGroups;
    granny_int32 AnimationCount;
    granny_animation** Animations;
    granny_variant ExtendedData;
} granny_file_info;

#pragma pack(pop)

/* Opaque runtime objects. */
typedef struct granny_file granny_file;
typedef struct granny_model_instance granny_model_instance;
typedef struct granny_control granny_control;
typedef struct granny_local_pose granny_local_pose;
typedef struct granny_world_pose granny_world_pose;
typedef struct granny_mesh_binding granny_mesh_binding;
typedef struct granny_mesh_deformer granny_mesh_deformer;
typedef struct granny_conversion_handler granny_conversion_handler;

typedef enum granny_standard_section_index
{
    GrannyStandardMainSection = 0,
    GrannyStandardRigidVertexSection = 1,
    GrannyStandardRigidIndexSection = 2,
    GrannyStandardDeformableVertexSection = 3,
    GrannyStandardDeformableIndexSection = 4,
    GrannyStandardTextureSection = 5,
    GrannyStandardDiscardableSection = 6,
    GrannyStandardUnloadedSection = 7
} granny_standard_section_index;

typedef enum granny_material_texture_type
{
    GrannyUnknownTextureType,
    GrannyAmbientColorTexture,
    GrannyDiffuseColorTexture,
    GrannySpecularColorTexture,
    GrannySelfIlluminationTexture,
    GrannyOpacityTexture,
    GrannyBumpHeightTexture,
    GrannyReflectionTexture,
    GrannyRefractionTexture,
    GrannyDisplacementTexture
} granny_material_texture_type;

typedef enum granny_deformation_type
{
    GrannyDeformPosition = 1,
    GrannyDeformPositionNormal,
    GrannyDeformPositionNormalTangent,
    GrannyDeformPositionNormalTangentBinormal
} granny_deformation_type;

typedef enum granny_deformer_tail_flags
{
    GrannyDontAllowUncopiedTail,
    GrannyAllowUncopiedTail
} granny_deformer_tail_flags;

typedef enum granny_log_message_type
{
    GrannyIgnoredLogMessage,
    GrannyNoteLogMessage,
    GrannyWarningLogMessage,
    GrannyErrorLogMessage
} granny_log_message_type;

typedef enum granny_log_message_origin
{
    GrannyNotImplementedLogMessage,
    GrannyApplicationLogMessage,
    GrannyFileReadingLogMessage,
    GrannyFileWritingLogMessage,
    GrannyMemoryLogMessage,
    GrannyMeshBindingLogMessage,
    GrannyControlLogMessage,
    GrannyAnimationLogMessage,
    GrannyDeformerLogMessage
} granny_log_message_origin;

typedef GRANNY_CALLBACK(void) granny_log_function(granny_log_message_type Type,
                                                  granny_log_message_origin Origin,
                                                  char const* File, granny_int32x Line,
                                                  char const* Message, void* UserData);

typedef struct granny_log_callback
{
    granny_log_function* Function;
    void* UserData;
} granny_log_callback;

#define GrannyVertexPositionName "Position"
#define GrannyVertexNormalName "Normal"
#define GrannyVertexTangentName "Tangent"
#define GrannyVertexBinormalName "Binormal"
#define GrannyVertexBoneWeightsName "BoneWeights"
#define GrannyVertexBoneIndicesName "BoneIndices"
#define GrannyVertexDiffuseColorName "DiffuseColor"
#define GrannyVertexTextureCoordinatesName "TextureCoordinates"

#ifndef GrannyTypeSizeCheck
#define GrannyTypeSizeCheck(expr)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Vertex layout: Position[3], Normal[3], TextureCoordinates0[2] (float). */
extern GRANNY_DYNLINKDATA(granny_data_type_definition*) GrannyPNT332VertexType;

/* Files */
GRANNY_DYNLINK(granny_file*) GrannyReadEntireFileFromMemory(granny_int32x MemorySize, void const* Memory);
GRANNY_DYNLINK(granny_file_info*) GrannyGetFileInfo(granny_file* File);
GRANNY_DYNLINK(void) GrannyFreeFile(granny_file* File);
GRANNY_DYNLINK(void) GrannyFreeFileSection(granny_file* File, granny_int32x SectionIndex);

/* Type reflection */
GRANNY_DYNLINK(granny_int32x) GrannyGetTotalTypeSize(granny_data_type_definition const* TypeDefinition);
GRANNY_DYNLINK(void) GrannyConvertSingleObject(granny_data_type_definition const* SourceType, void const* SourceObject,
                                               granny_data_type_definition const* DestType, void* DestObject,
                                               granny_conversion_handler* OverrideHandler);
GRANNY_DYNLINK(bool) GrannyFindMatchingMember(granny_data_type_definition const* SourceType, void const* SourceObject,
                                              char const* DestMemberName, granny_variant* Result);

/* Materials and skeletons */
GRANNY_DYNLINK(granny_texture*) GrannyGetMaterialTextureByType(granny_material const* Material,
                                                               granny_material_texture_type Type);
GRANNY_DYNLINK(bool) GrannyFindBoneByName(granny_skeleton const* Skeleton, char const* BoneName,
                                          granny_int32x* BoneIndex);

/* Meshes */
GRANNY_DYNLINK(granny_int32x) GrannyGetMeshVertexCount(granny_mesh const* Mesh);
GRANNY_DYNLINK(granny_data_type_definition*) GrannyGetMeshVertexType(granny_mesh const* Mesh);
GRANNY_DYNLINK(void*) GrannyGetMeshVertices(granny_mesh const* Mesh);
GRANNY_DYNLINK(granny_int32x) GrannyGetMeshIndexCount(granny_mesh const* Mesh);
GRANNY_DYNLINK(granny_int32x) GrannyGetMeshTriangleGroupCount(granny_mesh const* Mesh);
GRANNY_DYNLINK(granny_tri_material_group*) GrannyGetMeshTriangleGroups(granny_mesh const* Mesh);
GRANNY_DYNLINK(void) GrannyCopyMeshIndices(granny_mesh const* Mesh, granny_int32x BytesPerIndex, void* DestIndices);
GRANNY_DYNLINK(void) GrannyCopyMeshVertices(granny_mesh const* Mesh, granny_data_type_definition const* VertexType,
                                            void* DestVertices);
GRANNY_DYNLINK(bool) GrannyMeshIsRigid(granny_mesh const* Mesh);

/* Mesh binding and skinning */
GRANNY_DYNLINK(granny_mesh_binding*) GrannyNewMeshBinding(granny_mesh const* Mesh, granny_skeleton const* FromSkeleton,
                                                          granny_skeleton const* ToSkeleton);
GRANNY_DYNLINK(granny_int32x const*) GrannyGetMeshBindingToBoneIndices(granny_mesh_binding const* Binding);
GRANNY_DYNLINK(void) GrannyFreeMeshBinding(granny_mesh_binding* Binding);
GRANNY_DYNLINK(granny_mesh_deformer*) GrannyNewMeshDeformer(granny_data_type_definition const* InputVertexLayout,
                                                            granny_data_type_definition const* OutputVertexLayout,
                                                            granny_deformation_type DeformationType,
                                                            granny_deformer_tail_flags TailFlag);
GRANNY_DYNLINK(void) GrannyFreeMeshDeformer(granny_mesh_deformer* Deformer);
GRANNY_DYNLINK(void) GrannyDeformVertices(granny_mesh_deformer const* Deformer, granny_int32x const* MatrixIndices,
                                          granny_real32 const* MatrixBuffer4x4, granny_int32x VertexCount,
                                          void const* SourceVertices, void* DestVertices);

/* Model instances */
GRANNY_DYNLINK(granny_model_instance*) GrannyInstantiateModel(granny_model const* Model);
GRANNY_DYNLINK(void) GrannyFreeModelInstance(granny_model_instance* ModelInstance);
GRANNY_DYNLINK(granny_skeleton*) GrannyGetSourceSkeleton(granny_model_instance const* Model);
GRANNY_DYNLINK(void) GrannySetModelClock(granny_model_instance const* ModelInstance, granny_real32 NewClock);
GRANNY_DYNLINK(void) GrannyUpdateModelMatrix(granny_model_instance const* ModelInstance, granny_real32 SecondsElapsed,
                                             granny_real32 const* ModelMatrix4x4, granny_real32* DestMatrix4x4,
                                             bool Inverse);

/* Poses. Matrices are row-major with row vectors (translation in row 3). */
GRANNY_DYNLINK(granny_local_pose*) GrannyNewLocalPose(granny_int32x BoneCount);
GRANNY_DYNLINK(void) GrannyFreeLocalPose(granny_local_pose* LocalPose);
GRANNY_DYNLINK(granny_world_pose*) GrannyNewWorldPose(granny_int32x BoneCount);
GRANNY_DYNLINK(void) GrannyFreeWorldPose(granny_world_pose* WorldPose);
GRANNY_DYNLINK(granny_real32*) GrannyGetWorldPose4x4(granny_world_pose const* WorldPose, granny_int32x BoneIndex);
GRANNY_DYNLINK(granny_real32*) GrannyGetWorldPoseComposite4x4(granny_world_pose const* WorldPose, granny_int32x BoneIndex);
GRANNY_DYNLINK(granny_matrix_4x4*) GrannyGetWorldPoseComposite4x4Array(granny_world_pose const* WorldPose);
GRANNY_DYNLINK(void) GrannySampleModelAnimationsAccelerated(granny_model_instance const* ModelInstance,
                                                           granny_int32x BoneCount, granny_real32 const* Offset4x4,
                                                           granny_local_pose* Scratch, granny_world_pose* Result);

/* Animation controls */
GRANNY_DYNLINK(granny_control*) GrannyPlayControlledAnimation(granny_real32 StartTime, granny_animation const* Animation,
                                                             granny_model_instance* Model);
GRANNY_DYNLINK(void) GrannyFreeControl(granny_control* Control);
GRANNY_DYNLINK(bool) GrannyControlIsComplete(granny_control const* Control);
GRANNY_DYNLINK(void) GrannyCompleteControlAt(granny_control* Control, granny_real32 AtSeconds);
GRANNY_DYNLINK(bool) GrannyFreeControlIfComplete(granny_control* Control);
GRANNY_DYNLINK(void) GrannyFreeControlOnceUnused(granny_control* Control);
GRANNY_DYNLINK(void) GrannyFreeCompletedModelControls(granny_model_instance const* ModelInstance);
GRANNY_DYNLINK(granny_real32) GrannyGetControlLocalDuration(granny_control const* Control);
GRANNY_DYNLINK(granny_int32x) GrannyGetControlLoopCount(granny_control const* Control);
GRANNY_DYNLINK(void) GrannySetControlLoopCount(granny_control* Control, granny_int32x LoopCount);
GRANNY_DYNLINK(granny_real32) GrannyGetControlRawLocalClock(granny_control* Control);
GRANNY_DYNLINK(void) GrannySetControlRawLocalClock(granny_control* Control, granny_real32 LocalClock);
GRANNY_DYNLINK(granny_real32) GrannyGetControlSpeed(granny_control const* Control);
GRANNY_DYNLINK(void) GrannySetControlSpeed(granny_control* Control, granny_real32 Speed);
GRANNY_DYNLINK(void) GrannySetControlEaseIn(granny_control* Control, bool EaseIn);
GRANNY_DYNLINK(void) GrannySetControlEaseOut(granny_control* Control, bool EaseOut);
GRANNY_DYNLINK(void) GrannySetControlEaseInCurve(granny_control* Control, granny_real32 StartSeconds,
                                                 granny_real32 EndSeconds, granny_real32 StartValue,
                                                 granny_real32 StartTangent, granny_real32 EndTangent,
                                                 granny_real32 EndValue);
GRANNY_DYNLINK(void) GrannySetControlEaseOutCurve(granny_control* Control, granny_real32 StartSeconds,
                                                  granny_real32 EndSeconds, granny_real32 StartValue,
                                                  granny_real32 StartTangent, granny_real32 EndTangent,
                                                  granny_real32 EndValue);

/* Logging. Without a callback, warnings and errors go to stderr. */
GRANNY_DYNLINK(void) GrannySetLogCallback(granny_log_callback const* LogCallback);
GRANNY_DYNLINK(char const*) GrannyGetLogMessageTypeString(granny_log_message_type Type);
GRANNY_DYNLINK(char const*) GrannyGetLogMessageOriginString(granny_log_message_origin Origin);

#ifdef __cplusplus
}
#endif

#endif
