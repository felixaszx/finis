typealias byte = uint;

struct PrimInfo
{
    uint64_t position_ = -1; // vec3[]
    uint64_t normal_ = -1;   // vec3[]
    uint64_t tangent_ = -1;  // vec4[]
    uint64_t texcoord_ = -1; // vec2[]
    uint64_t joints_ = -1;   // uvec4[]
    uint64_t weights_ = -1;  // vec4[]

    uint64_t idx_ = -1;      // uint32_t[]
    uint64_t mesh_ = -1;     // MeshInfo*
    uint64_t material_ = -1; // MaterialInfo*
    uint64_t morph_ = -1;    // MorphInfo*
};


ConstantBuffer<float4> tt;
Sampler2D textures[];
StructuredBuffer<PrimInfo> PRIM_INFOS_;
StructuredBuffer<PrimInfo> PRIM_INFOS1_;

[shader("vertex")]
float4 vertex_main(uint64_t iii: PrimitiveIndex)
{
    return { 0, PRIM_INFOS_[0].idx_, PRIM_INFOS1_[0].idx_, 0 };
}

[shader("fragment")]
float4 fragment_main()
{
    return { 0, PRIM_INFOS_[0].idx_, PRIM_INFOS1_[0].idx_, textures[0].Sample( { 0, 0 }).r };
}