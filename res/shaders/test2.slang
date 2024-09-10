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
    float4 position_ : SV_Position;
    VertOutput output : VertOutput;
};

ConstantBuffer<float2[]> tex_coord;
[shader("vertex")]
VertStage vertex_main(uint32_t idx: SV_VertexID, //
                      uint32_t instance_id: SV_InstanceID)
{
    VertStage vs = {};
    vs.position_.xy = tex_coord[idx];
    vs.output.position_.xy = tex_coord[idx];
    vs.output.normal_.xy = tex_coord[idx];
    vs.output.texcoord_.xy = tex_coord[idx];
    vs.output.prim_idx_ = get_draw_idx();
    return vs;
}

struct FragStage
{
    float4 position : SV_Target0;
    float4 normal : SV_Target1;
    float4 color : SV_Target2;
    float4 specular : SV_Target3;
};

Sampler2D tex_arr[];
[shader("fragment")]
FragStage fragment_main(VertOutput vs: VertOutput)
{
    FragStage fs = {};
    fs.position = vs.position_;
    fs.normal.xyz = vs.normal_;
    fs.color = tex_arr[vs.prim_idx_].Sample(vs.texcoord_);
    fs.specular.xyzw = fs.color;
    return fs;
}