[[push_constant]]
cbuffer push_in
{
    ConstBufferPointer<float4> buffer0;
}

[shader("vertex")]
float4 main(uint64_t iii: PrimitiveIndex)
{
    return buffer0[0];
}
