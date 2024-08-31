module defines;

public struct PrimInfo
{
    public uint64_t position_; // vec3[]
    public uint64_t normal_;   // vec3[]
    public uint64_t tangent_;  // vec4[]
    public uint64_t texcoord_; // vec2[]
    public uint64_t joints_;   // uvec4[]
    public uint64_t weights_;  // vec4[]

    public uint64_t idx_;      // uint32_t[]
    public uint64_t mesh_;     // MeshInfo*
    public uint64_t material_; // MaterialInfo*
    public uint64_t morph_;    // MorphInfo*
};

public struct MorphInfo
{
    public uint64_t position_; // vec3[]
    public uint64_t normal_;   // vec3[]
    public uint64_t tangent_;  // vec4[]

    public uint64_t position_count_ = 0; // scalar
    public uint64_t normal_count_ = 0;   // scalar
    public uint64_t tangent_count_ = 0;  // scalar
};

public struct MeshInfo
{
    public uint64_t node_;          // mat4*
    public uint64_t morph_weights_; // float[]
    public uint64_t joint_;         // uint32_t[]
};

public struct MaterialInfo
{
    public float4 color_factor_;
    public float4 emissive_factor_;       // [3] = emissive strength
    public float4 sheen_color_factor_;    // [3] = sheen roughness factor
    public float4 specular_color_factor_; // [3] = specular factor

    public float alpha_cutoff_; // -1 means blend, 0 means opaque, otherwise means mask
    public float metallic_factor_;
    public float roughness_factor_;

    public uint32_t color_;
    public uint32_t metallic_roughness_;
    public uint32_t normal_;
    public uint32_t emissive_;
    public uint32_t occlusion_;

    public float anisotropy_rotation_;
    public float anisotropy_strength_;
    public uint32_t anisotropy_;

    public uint32_t specular_;
    public uint32_t spec_color_;

    public uint32_t sheen_color_;
    public uint32_t sheen_roughness_;
};
