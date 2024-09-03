import tools;
import defines;

struct vert_output
{
    float4 position_;
    float3 normal_;
    float4 tangent_;
    float2 texcoord_;
    uint32_t prim_idx_;
};

struct vert_stage
{
    float4 POSITION : SV_Position;
    vert_output OUT : vert_output;
};

struct frag_stage
{
    float4 POSITION : SV_Target0;
    float4 NORMAL : SV_Target1;
    float4 COLOR : SV_Target2;
    float4 SPECULAR : SV_Target3;
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
    uint64_t tranforms_offset_;
    uint64_t joint_idxs_offset_;
}

ConstantBuffer<prim_info_ptrs> PRIMS;
ConstantBuffer<prim_struc_ptrs> PRIM_STRUCTS;

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

    vert_stage vs;
    vs.POSITION.xyz = position;
    return vs;
}

[shader("fragment")]
frag_stage fragment_main(vert_output vs: vert_output)
{
    frag_stage fs;
    fs.POSITION = vs.position_;
    return fs;
}
