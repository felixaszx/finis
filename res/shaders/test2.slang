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
    VertOutput output : VSOutput;
};

struct FragStage
{
    float4 position : SV_Target0;
    float4 normal : SV_Target1;
    float4 color : SV_Target2;
    float4 specular : SV_Target3;
};

uniform float4 aaaaaa;

[shader("vertex")]
VertStage vertex_main(uint32_t idx: SV_VertexID, //
                      uint32_t instance_id: SV_InstanceID)
{
    VertStage vs;
    vs.output.tangent_ = aaaaaa;
    return vs;
}

[shader("fragment")]
FragStage fragment_main(VertOutput vs: VertOutput)
{
    FragStage fs;
    fs.color = vs.position_;
    fs.normal.xyz = vs.normal_;
    fs.color = vs.tangent_;
    fs.specular.xy = vs.prim_idx_ * vs.texcoord_;
    return { 0, 0, 0, 0 };
}