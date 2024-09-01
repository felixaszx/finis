import tools;
import defines;

struct VertOutput
{
    vec4 position_;
    vec3 normal_;
    vec4 tangent_;
    vec2 texcoord_;
    uint32_t prim_idx_;
};

struct VertStage
{
    vec4 POSITION : SV_Position;
    VertOutput OUT : VertOutput;
};

struct FragStage
{
    vec4 POSITION : SV_Target0;
    vec4 NORMAL : SV_Target1;
    vec4 COLOR : SV_Target2;
    vec4 SPECULAR : SV_Target3;
};

struct Pushed
{
    byte* DATA;
    byte* PRIMS;
}

uniform Pushed PUSHED;

[shader("vertex")]
VertStage vertex_main(uint32_t idx_id: SV_VertexID, //
                      uint32_t instance_id: SV_InstanceID)
    : SV_Position
{
    PrimInfo prim_infos = ((PrimInfo*)PUSHED.PRIMS)[get_draw_idx()];
    MeshInfo mesh = *((MeshInfo*)(PUSHED.DATA + prim_infos.mesh_));
    uint32_t idx = ((uint32_t*)(PUSHED.DATA + prim_infos.idx_))[idx_id];

    vec3 position = ((vec3*)(PUSHED.DATA + prim_infos.position_))[idx];
    vec3 normal = { 0, 0, 0 };
    vec4 tangent = { 0, 0, 0, 1 };
    vec2 texcoord = { 0, 0 };
    uvec4 joint_idxs = { -1, -1, -1, -1 };
    vec4 weights = { 0, 0, 0, 0 };

    // fetch vertex data
    {
        if (prim_infos.normal_ != -1)
        {
            normal = ((vec3*)(PUSHED.DATA + prim_infos.normal_))[idx];
        }

        if (prim_infos.tangent_ != -1)
        {
            tangent = ((vec4*)(PUSHED.DATA + prim_infos.tangent_))[idx];
        };

        if (prim_infos.tangent_ != -1)
        {
            texcoord = ((vec2*)(PUSHED.DATA + prim_infos.texcoord_))[idx];
        };

        if (prim_infos.joints_ != -1)
        {
            joint_idxs = ((uvec4*)(PUSHED.DATA + prim_infos.joints_))[idx];
        };

        if (prim_infos.weights_ != -1)
        {
            weights = ((vec4*)(PUSHED.DATA + prim_infos.weights_))[idx];
        };
    }

    // calculate morph data
    if (prim_infos.morph_ != -1)
    {
        MorphInfo morph = *((MorphInfo*)(PUSHED.DATA + prim_infos.morph_));
    }

    VertStage vs;
    vs.POSITION = { position.x, position.y, position.z, 1 };
    return vs;
}

[shader("fragment")]
FragStage fragment_main(VertOutput vs: VertOutput)
{
    FragStage fs;
    fs.POSITION = vs.position_;
    return fs;
}