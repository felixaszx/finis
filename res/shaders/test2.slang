import tools;
import defines;

struct FragmentOutput
{
    float4 position : SV_Target0;
    float4 normal : SV_Target1;
    float4 color : SV_Target2;
    float4 specular : SV_Target3;
    float4 vs_position : SV_Position;
};

StructuredBuffer<PrimInfo> PRIM_INFOS_;

[shader("vertex")]
FragmentOutput main(uint32_t INSTANCE_ID: SV_INSTANCEID)
{
    FragmentOutput output;
    output.position = { 0, INSTANCE_ID, 0, 0 };
    output.normal = { 0, 0, 0, 0 };
    output.color = { 0, 0, 0, 0 };
    output.specular = { 0, PRIM_INFOS_[0].idx_, 0, 0 };
    output.vs_position = { 0, 0, 0, 0 };

    return output;
}
