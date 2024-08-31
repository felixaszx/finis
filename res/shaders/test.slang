import tools;
import defines;

StructuredBuffer<PrimInfo> PRIM_INFOS_;

[shader("vertex")]
float4 vertex_main()
{
    uint32_t aaa = get_draw_idx();
    return { aaa, aaa, 0, 0 };
}

[shader("fragment")]
float4 fragment_main()
{
    return { 0, 0, 0, 0 };
}
