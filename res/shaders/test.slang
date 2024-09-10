import tools;
import defines;

struct vert_output
{
    float3 position_;
    float3 normal_;
    float3 tangent_;
    float3 bitangent_;
    float2 texcoord_;
    uint32_t prim_idx_;
};

struct vert_stage
{
    float4 POSITION : SV_Position;
    vert_output OUT : vert_output;
};

struct prim_info_ptrs
{
    byte* data_;
    byte* prims_;
}

struct prim_struc_ptrs
{
    byte* address_;
    uint64_t mesh_idx_offset_;
    uint64_t meshes_offset_;
    uint64_t morph_weights_offset_;
    uint64_t tranforms_offset_ = 0;
}

struct prim_skin_ptrs
{
    byte* address_;
    uint64_t joints_offset_;
    uint64_t inv_binds_offset_;
}

ConstantBuffer<prim_info_ptrs> PRIMS;
ConstantBuffer<prim_struc_ptrs> PRIM_STRUCTS;
ConstantBuffer<prim_skin_ptrs> PRIM_SKINS;

[shader("vertex")]
vert_stage vertex_main(uint idx_id: SV_VertexID, //
                       uint instance_id: SV_InstanceID)
{
    prim_info prim_info = ((prim_info*)PRIMS.prims_)[get_draw_idx()];
    uint mesh_idx = ((uint*)(PRIM_STRUCTS.address_ + PRIM_STRUCTS.mesh_idx_offset_))[get_draw_idx()];
    mesh_info mesh_info = ((mesh_info*)(PRIM_STRUCTS.address_ + PRIM_STRUCTS.meshes_offset_))[mesh_idx];
    uint idx = ((uint*)(PRIMS.data_ + prim_info.idx_))[idx_id];

    float3 position = ((float3*)(PRIMS.data_ + prim_info.position_))[idx];
    float3 normal = { 0, 0, 0 };
    float4 tangent = { 0, 0, 0, 1 };
    float2 texcoord = { 0, 0 };
    uint4 joint_idxs = { -1, -1, -1, -1 };
    float4 weights = { 0, 0, 0, 0 };

    // fetch vertex data_
    {
        if (prim_info.normal_ != -1)
        {
            normal = ((float3*)(PRIMS.data_ + prim_info.normal_))[idx];
        }

        if (prim_info.tangent_ != -1)
        {
            tangent = ((float4*)(PRIMS.data_ + prim_info.tangent_))[idx];
        };

        if (prim_info.tangent_ != -1)
        {
            texcoord = ((float2*)(PRIMS.data_ + prim_info.texcoord_))[idx];
        };

        if (prim_info.joints_ != -1)
        {
            joint_idxs = ((uint4*)(PRIMS.data_ + prim_info.joints_))[idx];
        };

        if (prim_info.weights_ != -1)
        {
            weights = ((float4*)(PRIMS.data_ + prim_info.weights_))[idx];
        };
    }

    // calculate morph data_
    if (prim_info.morph_ != -1)
    {
        morph_info morph = *((morph_info*)(PRIMS.data_ + prim_info.morph_));
    }

    vert_stage vs = {};
    vs.POSITION.xyz = { mesh_info.node_, idx, PRIM_SKINS.joints_offset_ };
    vs.OUT.texcoord_ = texcoord;
    vs.OUT.position_ = position;
    vs.OUT.normal_ = normal;
    vs.OUT.prim_idx_ = get_draw_idx();
    return vs;
}

struct frag_stage
{
    float4 POSITION : SV_Target0;
    float4 NORMAL : SV_Target1;
    float4 COLOR : SV_Target2;
    float4 SPECULAR : SV_Target3;
};

Sampler2D tex_arr[];
[shader("fragment")]
frag_stage fragment_main(vert_output vs: vert_output)
{
    frag_stage fs = {};
    fs.POSITION.xyz = vs.position_;
    fs.NORMAL.xyz = tex_arr[vs.prim_idx_].Sample(vs.texcoord_).xyz;
    fs.COLOR.xyz = tex_arr[vs.prim_idx_].Sample(vs.texcoord_).xyz;
    fs.SPECULAR.xyz = tex_arr[vs.prim_idx_].Sample(vs.texcoord_).xyz;
    return fs;
}
