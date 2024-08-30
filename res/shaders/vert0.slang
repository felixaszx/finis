StructuredBuffer<float> buffer0;

[shader("vertex")]
float4 main(uint64_t iii: PrimitiveIndex)
{
    return { 0, 0, 0, 1 };
}