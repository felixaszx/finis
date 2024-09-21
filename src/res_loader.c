#include "res_loader.h"

#define GET_IDX(this, first) (this ? (this - first) : -1)

VkFilter get_filter(cgltf_int filter)
{
    switch (filter)
    {
        case 9984:
        case 9986:
        case 9728:
            return VK_FILTER_NEAREST;
        case 9985:
        case 9987:
        case 9729:
            return VK_FILTER_LINEAR;
    }
    return VK_FILTER_LINEAR;
}

VkSamplerMipmapMode get_mip_map(cgltf_int filter, VkSamplerMipmapMode* mode)
{
    switch (filter)
    {
        case 9984:
        case 9985:
            *mode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            return true;
        case 9986:
        case 9987:
            *mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            return true;
    }
    return false;
}

VkSamplerAddressMode get_wrapping(cgltf_int wrapping)
{
    switch (wrapping)
    {
        case 33071:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        case 33648:
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        case 10497:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    }
    return VK_SAMPLER_ADDRESS_MODE_REPEAT;
}

IMPL_OBJ_NEW(gltf_file, const char* file_path)
{
    cgltf_options options = {};
    cgltf_result result = cgltf_parse_file(&options, file_path, &this->data_);
    if (result != cgltf_result_success)
    {
        cgltf_free(this->data_);
        this->data_ = nullptr;
    }
    cgltf_load_buffers(&options, this->data_, file_path);

    this->material_count_ = this->data_->materials_count;
    this->materials_ = alloc(vk_material, this->material_count_);
    for (size_t i = 0; i < this->material_count_; i++)
    {
        cgltf_texture* first_tex = this->data_->textures;
        cgltf_material* material_in = this->data_->materials + i;
        vk_material* material = this->materials_ + i;
        construct_vk_material(material);

        if (material_in->has_pbr_metallic_roughness)
        {
            material->color_ = GET_IDX(material_in->pbr_metallic_roughness.base_color_texture.texture, first_tex);
            material->metallic_roughness_ =
                GET_IDX(material_in->pbr_metallic_roughness.metallic_roughness_texture.texture, //
                        first_tex);
            memcpy(material->color_factor_, material_in->pbr_metallic_roughness.base_color_factor,
                   sizeof(material->color_factor_));
        }

        material->normal_ = GET_IDX(material_in->normal_texture.texture, first_tex);

        switch (material_in->alpha_mode)
        {
            case cgltf_alpha_mode_opaque:
                material->alpha_cutoff_ = 0;
            case cgltf_alpha_mode_mask:
                material->alpha_cutoff_ = material_in->alpha_cutoff;
            case cgltf_alpha_mode_blend:
                material->alpha_cutoff_ = -1;
            case cgltf_alpha_mode_max_enum:
                break;
        }

        material->occlusion_ = GET_IDX(material_in->occlusion_texture.texture, first_tex);
        material->emissive_ = GET_IDX(material_in->emissive_texture.texture, first_tex);
        memcpy(material->emissive_factor_, material_in->emissive_factor, sizeof(float[3]));

        // extensions

        if (material_in->has_emissive_strength)
        {
            material->emissive_factor_[3] = material_in->emissive_strength.emissive_strength;
        }

        if (material_in->has_anisotropy)
        {
            material->anisotropy_rotation_ = material_in->anisotropy.anisotropy_rotation;
            material->anisotropy_strength_ = material_in->anisotropy.anisotropy_strength;
            material->anisotropy_ = GET_IDX(material_in->anisotropy.anisotropy_texture.texture, first_tex);
        }

        if (material_in->has_specular)
        {
            material->specular_ = GET_IDX(material_in->specular.specular_texture.texture, first_tex);
            material->spec_color_ = GET_IDX(material_in->specular.specular_color_texture.texture, first_tex);
            material->specular_color_factor_[3] = material_in->specular.specular_factor;
            memcpy(material->specular_color_factor_, material_in->specular.specular_color_factor, sizeof(float[3]));
        }

        if (material_in->has_sheen)
        {
            material->sheen_color_ = GET_IDX(material_in->sheen.sheen_color_texture.texture, first_tex);
            material->sheen_roughness_ = GET_IDX(material_in->sheen.sheen_roughness_texture.texture, first_tex);
            material->sheen_color_factor_[3] = material_in->sheen.sheen_roughness_factor;
            memcpy(material->sheen_color_factor_, material_in->sheen.sheen_color_factor, sizeof(float[3]));
        }
    }

    this->sampler_count_ = this->data_->samplers_count;
    this->sampler_cinfos_ = alloc(VkSamplerCreateInfo, this->sampler_count_);
    for (size_t s = 0; s < this->sampler_count_; s++)
    {
        cgltf_sampler* sampler = this->data_->samplers + s;
        VkSamplerCreateInfo* cinfo = this->sampler_cinfos_ + s;
        cinfo->sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        cinfo->addressModeU = get_wrapping(sampler->wrap_s);
        cinfo->addressModeV = get_wrapping(sampler->wrap_t);
        cinfo->magFilter = get_filter(sampler->mag_filter);
        cinfo->minFilter = get_filter(sampler->min_filter);
        cinfo->anisotropyEnable = true;
        cinfo->maxAnisotropy = 4.0f;
        cinfo->maxLod = get_mip_map(sampler->min_filter, &cinfo->mipmapMode) ? VK_LOD_CLAMP_NONE : 0;
    }

    this->tex_count_ = this->data_->textures_count;
    this->texs_ = alloc(gltf_tex, this->tex_count_);
    for (size_t t = 0; t < this->tex_count_; t++)
    {
        cgltf_texture* tex = this->data_->textures + t;
        cgltf_buffer_view* view = tex->image->buffer_view;
        this->texs_[t].data_ = stbi_load_from_memory(cgltf_buffer_view_data(view), view->size, //
                                                     &this->texs_[t].width_,                   //
                                                     &this->texs_[t].height_,                  //
                                                     &this->texs_[t].channel_, STBI_rgb_alpha);
        this->texs_[t].sampler_idx_ = GET_IDX(tex->sampler, this->data_->samplers);
        sprintf_s(this->texs_[t].name_, sizeof(this->texs_[t].name_), "%s__[i_%s]", tex->name, tex->image->name);
    }

    size_t prim_count = 0;
    for (size_t m = 0; m < this->data_->meshes_count; m++)
    {
        cgltf_mesh* mesh = this->data_->meshes + m;
        prim_count += mesh->primitives_count;
    }

    this->prims_ = alloc(gltf_prim, prim_count);
    size_t p = 0;
    for (size_t m = 0; m < this->data_->meshes_count; m++)
    {
        cgltf_mesh* mesh_in = this->data_->meshes + m;
        for (size_t pp = 0; pp < mesh_in->primitives_count; pp++)
        {
            cgltf_primitive* prim_in = mesh_in->primitives + pp;
            gltf_prim* prim = this->prims_ + p;
            sprintf_s(prim->name_, sizeof(prim->name_), "%s__[%zu]", mesh_in->name, pp);

            for (size_t a = 0; a < prim_in->attributes_count; a++)
            {
                cgltf_attribute* attrib = prim_in->attributes + a;
                cgltf_accessor* acc = attrib->data;
                switch (attrib->type)
                {
                    case cgltf_attribute_type_position:
                        prim->vtx_count_ = acc->count;
                        prim->position = malloc(acc->count * sizeof(*prim->position));
                        cgltf_accessor_unpack_floats(acc, *prim->position, acc->count * 3);
                        break;
                    case cgltf_attribute_type_normal:
                        prim->normal_ = malloc(acc->count * sizeof(*prim->normal_));
                        cgltf_accessor_unpack_floats(acc, *prim->normal_, acc->count * 3);
                        break;
                    case cgltf_attribute_type_tangent:
                        prim->tangent_ = malloc(acc->count * sizeof(*prim->tangent_));
                        cgltf_accessor_unpack_floats(acc, *prim->tangent_, acc->count * 4);
                        break;
                    case cgltf_attribute_type_texcoord:
                        prim->texcoord_ = malloc(acc->count * sizeof(*prim->texcoord_));
                        cgltf_accessor_unpack_floats(acc, *prim->texcoord_, acc->count * 2);
                        break;
                    case cgltf_attribute_type_color:
                        prim->color_ = malloc(acc->count * sizeof(*prim->color_));
                        cgltf_accessor_unpack_floats(acc, *prim->color_, acc->count * 4);
                        break;
                    case cgltf_attribute_type_joints:
                        prim->joint_ = malloc(acc->count * sizeof(*prim->joint_));
                        cgltf_accessor_unpack_indices(acc, *prim->joint_, 4, acc->count);
                        break;
                    case cgltf_attribute_type_weights:
                        prim->weight_ = malloc(acc->count * sizeof(*prim->weight_));
                        cgltf_accessor_unpack_floats(acc, *prim->weight_, acc->count * 4);
                        break;
                    case cgltf_attribute_type_custom:
                    case cgltf_attribute_type_invalid:
                    case cgltf_attribute_type_max_enum:
                        break;
                }
            }

            cgltf_accessor* idx_acc = prim_in->indices;
            if (idx_acc)
            {
                prim->idx_count_ = idx_acc->count;
                prim->idx_ = malloc(prim->idx_count_ * sizeof(uint32_t));
                cgltf_accessor_unpack_indices(idx_acc, prim->idx_, 4, prim->idx_count_);
            }
            else
            {
                prim->idx_count_ = prim->vtx_count_;
                prim->idx_ = malloc(prim->idx_count_ * sizeof(uint32_t));
                for (size_t i = 0; i < prim->idx_count_; i++)
                {
                    prim->idx_[i] = i;
                }
            }

            p++;
        }
    }

    return this;
}

IMPL_OBJ_DELETE(gltf_file)
{
    if (this->data_)
    {
        cgltf_free(this->data_);
    }

    for (size_t p = 0; p > this->prim_count_; p++)
    {
        gltf_prim* prim = this->prims_ + p;
        ffree(prim->position);
        ffree(prim->normal_);
        ffree(prim->tangent_);
        ffree(prim->texcoord_);
        ffree(prim->color_);
        ffree(prim->joint_);
        ffree(prim->weight_);
        ffree(prim->idx_);
    }
    ffree(this->prims_);

    ffree(this->materials_);
    ffree(this->transforms_);
    ffree(this->sampler_cinfos_);

    for (size_t t = 0; t < this->tex_count_; t++)
    {
        stbi_image_free(this->texs_[t].data_);
    }
    ffree(this->texs_);
}