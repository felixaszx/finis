import tools;
import defines;

struct VertOutput
{
    float4 position_;
    float3 normal_;
    float4 tangent_;
    float2 texcoord_;
    uint32_t prim_idx_;
};

struct VertStage
{
    float4 POSITION : SV_Position;
    VertOutput OUT : VertOutput;
};

struct FragStage
{
    float4 POSITION : SV_Target0;
    float4 NORMAL : SV_Target1;
    float4 COLOR : SV_Target2;
    float4 SPECULAR : SV_Target3;
};

struct Pointers
{
    byte* data_;
    byte* prims_;
}

ConstantBuffer<Pointers> POINTERS;

[shader("vertex")]
VertStage vertex_main(uint idx_id: SV_VertexID, //
                      uint instance_id: SV_InstanceID)
{
    PrimInfo prim_infos = ((PrimInfo*)POINTERS.prims_)[get_draw_idx()];
    MeshInfo mesh = *((MeshInfo*)(POINTERS.data_ + prim_infos.mesh_));
    uint idx = ((uint*)(POINTERS.data_ + prim_infos.idx_))[idx_id];

    float3 position = ((float3*)(POINTERS.data_ + prim_infos.position_))[idx];
    float3 normal = { 0, 0, 0 };
    float4 tangent = { 0, 0, 0, 1 };
    float2 texcoord = { 0, 0 };
    uint4 joint_idxs = { -1, -1, -1, -1 };
    float4 weights = { 0, 0, 0, 0 };

    // fetch vertex data_
    {
        if (prim_infos.normal_ != -1)
        {
            normal = ((float3*)(POINTERS.data_ + prim_infos.normal_))[idx];
        }

        if (prim_infos.tangent_ != -1)
        {
            tangent = ((float4*)(POINTERS.data_ + prim_infos.tangent_))[idx];
        };

        if (prim_infos.tangent_ != -1)
        {
            texcoord = ((float2*)(POINTERS.data_ + prim_infos.texcoord_))[idx];
        };

        if (prim_infos.joints_ != -1)
        {
            joint_idxs = ((uint4*)(POINTERS.data_ + prim_infos.joints_))[idx];
        };

        if (prim_infos.weights_ != -1)
        {
            weights = ((float4*)(POINTERS.data_ + prim_infos.weights_))[idx];
        };
    }

    // calculate morph data_
    if (prim_infos.morph_ != -1)
    {
        MorphInfo morph = *((MorphInfo*)(POINTERS.data_ + prim_infos.morph_));
    }

    VertStage vs;
    vs.POSITION.xyz = position;
    return vs;
}

[shader("fragment")]
FragStage fragment_main(VertOutput vs: VertOutput)
{
    FragStage fs;
    fs.POSITION = vs.position_;
    return fs;
}