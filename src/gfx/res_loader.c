#include "res_loader.h"
#include <stb/stb_ds.h>

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
        printf("Fail to parse gltf file %s", file_path);
        return this;
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
        this->texs_[t].channel_ = 4;
        this->texs_[t].sampler_idx_ = GET_IDX(tex->sampler, this->data_->samplers);
        if (this->sampler_cinfos_[this->texs_[t].sampler_idx_].maxLod)
        {
#define MAX(x, y) (x > y ? x : y)
            this->texs_[t].levels_ = floor(log2(MAX(this->texs_[t].width_, this->texs_[t].height_))) + 1;
        }
        snprintf(this->texs_[t].name_, sizeof(this->texs_[t].name_), "%s__[i_%s]", tex->name, tex->image->name);
    }

    for (size_t m = 0; m < this->data_->meshes_count; m++)
    {
        cgltf_mesh* mesh = this->data_->meshes + m;
        this->prim_count_ += mesh->primitives_count;
    }

    this->prims_ = alloc(gltf_prim, this->prim_count_);
    size_t p = 0;
    for (size_t m = 0; m < this->data_->meshes_count; m++)
    {
        cgltf_mesh* mesh_in = this->data_->meshes + m;
        for (size_t pp = 0; pp < mesh_in->primitives_count; pp++)
        {
            cgltf_primitive* prim_in = mesh_in->primitives + pp;
            gltf_prim* prim = this->prims_ + p;
            snprintf(prim->name_, sizeof(prim->name_), "%s__[%zu]", mesh_in->name, pp);

            for (size_t a = 0; a < prim_in->attributes_count; a++)
            {
                cgltf_attribute* attrib = prim_in->attributes + a;
                cgltf_accessor* acc = attrib->data;
                switch (attrib->type)
                {
                    case cgltf_attribute_type_position:
                        prim->vtx_count_ = acc->count;
                        prim->position = calloc(acc->count, sizeof(*prim->position));
                        cgltf_accessor_unpack_floats(acc, *prim->position, acc->count * 3);
                        break;
                    case cgltf_attribute_type_normal:
                        prim->normal_ = calloc(acc->count, sizeof(*prim->normal_));
                        cgltf_accessor_unpack_floats(acc, *prim->normal_, acc->count * 3);
                        break;
                    case cgltf_attribute_type_tangent:
                        prim->tangent_ = calloc(acc->count, sizeof(*prim->tangent_));
                        cgltf_accessor_unpack_floats(acc, *prim->tangent_, acc->count * 4);
                        break;
                    case cgltf_attribute_type_texcoord:
                        prim->texcoord_ = calloc(acc->count, sizeof(*prim->texcoord_));
                        cgltf_accessor_unpack_floats(acc, *prim->texcoord_, acc->count * 2);
                        break;
                    case cgltf_attribute_type_color:
                        prim->color_ = calloc(acc->count, sizeof(*prim->color_));
                        cgltf_accessor_unpack_floats(acc, *prim->color_, acc->count * 4);
                        break;
                    case cgltf_attribute_type_joints:
                        prim->joint_ = calloc(acc->count, sizeof(*prim->joint_));
                        cgltf_accessor_unpack_indices(acc, *prim->joint_, 4, acc->count);
                        break;
                    case cgltf_attribute_type_weights:
                        prim->weight_ = calloc(acc->count, sizeof(*prim->weight_));
                        cgltf_accessor_unpack_floats(acc, *prim->weight_, acc->count * 4);
                        break;
                    default:
                        break;
                }
            }

            for (size_t a = 0; a < prim_in->targets_count; a++)
            {
                cgltf_morph_target* target = prim_in->targets + a;
                for (size_t ta = 0; ta < target->attributes_count; ta++)
                {
                    cgltf_accessor* acc = target->attributes[ta].data;
                    switch (target->attributes->type)
                    {
                        case cgltf_attribute_type_position:
                            prim->morph_position = calloc(acc->count, sizeof(*prim->position));
                            cgltf_accessor_unpack_floats(acc, *prim->position, acc->count * 3);
                            break;
                        case cgltf_attribute_type_normal:
                            prim->morph_normal_ = calloc(acc->count, sizeof(*prim->normal_));
                            cgltf_accessor_unpack_floats(acc, *prim->normal_, acc->count * 3);
                            break;
                        case cgltf_attribute_type_tangent:
                            prim->morph_tangent_ = calloc(acc->count, sizeof(*prim->tangent_));
                            cgltf_accessor_unpack_floats(acc, *prim->tangent_, acc->count * 3);
                            break;
                        default:
                            break;
                    }
                }
            }

            cgltf_accessor* idx_acc = prim_in->indices;
            if (idx_acc)
            {
                prim->idx_count_ = idx_acc->count;
                prim->idx_ = calloc(prim->idx_count_, sizeof(uint32_t));
                cgltf_accessor_unpack_indices(idx_acc, prim->idx_, 4, prim->idx_count_);
            }
            else
            {
                prim->idx_count_ = prim->vtx_count_;
                prim->idx_ = calloc(prim->idx_count_, sizeof(uint32_t));
                for (size_t i = 0; i < prim->idx_count_; i++)
                {
                    prim->idx_[i] = i;
                }
            }

            prim->material_ = this->materials_ + GET_IDX(prim_in->material, this->data_->materials);
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
        free(prim->position);
        free(prim->normal_);
        free(prim->tangent_);
        free(prim->texcoord_);
        free(prim->color_);
        free(prim->joint_);
        free(prim->weight_);
        free(prim->idx_);
        free(prim->morph_position);
        free(prim->morph_normal_);
        free(prim->morph_tangent_);
    }
    ffree(this->prims_);

    ffree(this->materials_);
    ffree(this->sampler_cinfos_);

    for (size_t t = 0; t < this->tex_count_; t++)
    {
        stbi_image_free(this->texs_[t].data_);
    }
    ffree(this->texs_);
}

void build_layer(cgltf_node**** layers, cgltf_node* curr, uint32_t depth)
{
    size_t layer_count = stbds_arrlen((*layers));
    if (layer_count <= depth)
    {
        stbds_arrsetlen((*layers), depth + 1);
        for (size_t i = layer_count; i < depth + 1; i++)
        {
            (*layers)[i] = nullptr;
        }
    }

    stbds_arrput((*layers)[depth], curr);

    for (size_t i = 0; i < curr->children_count; i++)
    {
        build_layer(layers, curr->children[i], depth + 1);
    }
}

IMPL_OBJ_NEW(gltf_desc, gltf_file* file)
{
    cgltf_mesh* meshes = file->data_->meshes;
    this->mesh_count_ = file->data_->meshes_count;
    this->prim_offset_ = alloc(uint32_t, this->mesh_count_);
    this->transform_ = alloc(vk_prim_transform, this->mesh_count_);
    this->node_count_ = file->data_->nodes_count;
    this->nodes_ = alloc(vk_mesh_node, this->node_count_);
    this->mapping_ = alloc(uint32_t, this->node_count_);

    size_t prim_count = 0;
    for (size_t m = 0; m < this->mesh_count_; m++)
    {
        construct_vk_prim_transform(this->transform_ + m);
        this->prim_offset_[m] = prim_count;
        prim_count += meshes[m].primitives_count;
    }
    this->prim_transform_ = alloc(vk_prim_transform*, file->prim_count_);

    cgltf_node*** layers = nullptr;
    for (size_t i = 0; i < file->data_->scene[0].nodes_count; i++)
    {
        build_layer(&layers, file->data_->scene[0].nodes[i], 0);
    }

    size_t node_idx = 0;
    for (size_t i = 0; i < stbds_arrlen(layers); i++)
    {
        for (size_t j = 0; j < stbds_arrlen(layers[i]); j++)
        {
            this->mapping_[GET_IDX(layers[i][j], file->data_->nodes)] = node_idx;
            cgltf_node* node_in = layers[i][j];
            vk_mesh_node* node = this->nodes_ + node_idx;
            construct_vk_mesh_node(node);

            if (node_in->has_translation)
            {
                memcpy(node->translation_, node_in->translation, sizeof(node->translation_));
            }

            if (node_in->has_rotation)
            {
                memcpy(node->rotation, node_in->rotation, sizeof(node->rotation));
            }

            if (node_in->has_scale)
            {
                memcpy(node->scale_, node_in->scale, sizeof(node->scale_));
            }

            if (node_in->has_matrix)
            {
                memcpy(node->preset_, node_in->matrix, sizeof(node->preset_));
            }

            if (node_in->parent)
            {
                node->parent_idx_ = this->mapping_[GET_IDX(node_in->parent, file->data_->nodes)];
            }

            if (node_in->mesh)
            {
                vk_prim_transform* transform = this->transform_ + GET_IDX(node_in->mesh, file->data_->meshes);
                transform->node_idx_ = node_idx;
                transform->first_joint_ = -1;

                if (node_in->weights)
                {
                    transform->target_count_ = node_in->weights_count;
                }
                else if (node_in->mesh->weights)
                {
                    transform->target_count_ = node_in->mesh->weights_count;
                }

                size_t p = this->prim_offset_[GET_IDX(node_in->mesh, file->data_->meshes)];
                for (size_t pp = 0; pp < node_in->mesh->primitives_count; pp++)
                {
                    this->prim_transform_[p + pp] = transform;
                }
            }

            node_idx++;
        }
    }

    for (size_t i = 0; i < stbds_arrlen(layers); i++)
    {
        stbds_arrfree(layers[i]);
    }
    stbds_arrfree(layers);
    return this;
}

IMPL_OBJ_DELETE(gltf_desc)
{
    ffree(this->nodes_);
    ffree(this->mapping_);
    ffree(this->transform_);
    ffree(this->prim_transform_);
    ffree(this->prim_offset_);
}

size_t gltf_tex_size(gltf_tex* this)
{
    return this->width_ * this->height_ * this->channel_;
}

VkExtent3D gltf_tex_extent(gltf_tex* this)
{
    return (VkExtent3D){this->width_, this->height_, 1};
}

int cmp_gltf_ms(const void* x, const void* y)
{
    if (*(const gltf_ms*)x < *(const gltf_ms*)y)
    {
        return -1;
    }
    else if (*(const gltf_ms*)x > *(const gltf_ms*)y)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void gltf_frame_sample(gltf_key_frame* this, gltf_key_frame_channel channel, gltf_ms time_pt, T* dst)
{
    if (!this->time_stamps_[channel])
    {
        return;
    }

    gltf_ms clamped = this->time_stamps_[channel][this->stamp_count_[channel] - 1] - this->time_stamps_[channel][0];
    clamped = this->time_stamps_[channel][0] + time_pt % clamped;
    const gltf_ms* time_left = bsearch(&clamped, this->time_stamps_[channel], this->stamp_count_[channel], //
                                       sizeof(gltf_ms), cmp_gltf_ms);
    const gltf_ms* time_right = time_left + 1;
    float t = (float)(clamped - *time_left) / (float)(time_right - time_left);
    size_t left_idx = time_left - this->time_stamps_[clamped];

    switch (channel)
    {
        case GLTF_KEY_FRAME_CHANNEL_S:
        case GLTF_KEY_FRAME_CHANNEL_T:
        {
            vec3* data_left = ((vec3*)this->data_[channel]) + left_idx;
            glm_vec3_lerp(*data_left, *(data_left + 1), t, dst);
            return;
        }
        case GLTF_KEY_FRAME_CHANNEL_R:
        {
            quat* data_left = ((quat*)this->data_[channel]) + left_idx;
            glm_quat_lerpc(*data_left, *(data_left + 1), t, dst);
            return;
        }
        case GLTF_KEY_FRAME_CHANNEL_W:
        {
            float* data_left = ((float*)this->data_[channel]) + (left_idx * this->w_per_morph_);
            for (size_t i = 0; i < this->w_per_morph_; i++)
            {
                ((float*)dst)[i] = glm_lerp(data_left[i], data_left[i + this->w_per_morph_], t);
            }
            return;
        }
    }
}

IMPL_OBJ_NEW(gltf_anim, gltf_file* file, uint32_t anim_idx)
{
    this->node_count_ = file->data_->nodes_count;
    this->mapping_ = alloc(gltf_key_frame*, this->node_count_);
    this->key_frames_ = alloc(gltf_key_frame, this->node_count_);
    const cgltf_animation* anim = file->data_->animations + anim_idx;

    uint32_t key_frame_count = 0;
    for (size_t i = 0; i < anim->channels_count; i++)
    {
        cgltf_animation_channel* chan = anim->channels + i;
        cgltf_animation_sampler* sampler = chan->sampler;
        uint32_t node_idx = GET_IDX(chan->target_node, file->data_->nodes);

        if (this->mapping_[node_idx] == nullptr)
        {
            this->mapping_[node_idx] = this->key_frames_ + key_frame_count;
            key_frame_count++;
        }

        gltf_key_frame* key_frame = this->mapping_[node_idx];
        key_frame->stamp_count_[chan->target_path - 1] = sampler->input->count;
        key_frame->time_stamps_[chan->target_path - 1] = calloc(sampler->input->count, sizeof(gltf_ms));
        float* time_stamp_buffer = calloc(sampler->input->count, sizeof(float));
        cgltf_accessor_unpack_floats(sampler->input, time_stamp_buffer, sampler->input->count);
        for (size_t k = 0; k < sampler->input->count; k++)
        {
            key_frame->time_stamps_[chan->target_path - 1][k] = 1000 * time_stamp_buffer[k];
        }
        free(time_stamp_buffer);

        switch (chan->target_path)
        {
            case cgltf_animation_path_type_translation:
                key_frame->data_[GLTF_KEY_FRAME_CHANNEL_T] = calloc(sampler->output->count, sizeof(vec3));
                cgltf_accessor_unpack_floats(sampler->output, (float*)key_frame->data_[GLTF_KEY_FRAME_CHANNEL_T],
                                             sampler->output->count * 3);
                break;
            case cgltf_animation_path_type_rotation:
                key_frame->data_[GLTF_KEY_FRAME_CHANNEL_R] = calloc(sampler->output->count, sizeof(quat));
                cgltf_accessor_unpack_floats(sampler->output, (float*)key_frame->data_[GLTF_KEY_FRAME_CHANNEL_R],
                                             sampler->output->count * 4);
                break;
            case cgltf_animation_path_type_scale:
                key_frame->data_[GLTF_KEY_FRAME_CHANNEL_S] = calloc(sampler->output->count, sizeof(vec3));
                cgltf_accessor_unpack_floats(sampler->output, (float*)key_frame->data_[GLTF_KEY_FRAME_CHANNEL_S],
                                             sampler->output->count * 3);
                break;
            case cgltf_animation_path_type_weights:
                key_frame->w_per_morph_ = chan->target_node->weights_count //
                                              ? chan->target_node->weights_count
                                              : chan->target_node->mesh->weights_count;
                key_frame->data_[GLTF_KEY_FRAME_CHANNEL_W] = calloc(sampler->output->count, sizeof(float));
                cgltf_accessor_unpack_floats(sampler->output, (float*)key_frame->data_[GLTF_KEY_FRAME_CHANNEL_W],
                                             sampler->output->count * key_frame->w_per_morph_);
                break;
            default:
                break;
        }
    }

    return this;
}

IMPL_OBJ_DELETE(gltf_anim)
{
    for (uint32_t i = 0; i < this->node_count_; i++)
    {
        for (uint32_t c = 0; c < GLTF_FRAME_CHANNEL_COUNT; c++)
        {
            free(this->key_frames_[i].time_stamps_[c]);
            free(this->key_frames_[i].data_[c]);
        }
    }
    ffree(this->mapping_);
    ffree(this->key_frames_);
}

IMPL_OBJ_NEW(gltf_skin, gltf_file* file, gltf_desc* desc)
{
    this->skin_count_ = file->data_->skins_count;
    this->skin_offsets_ = alloc(uint32_t, this->skin_count_);

    for (size_t s = 0; s < file->data_->skins_count; s++)
    {
        this->skin_offsets_[s] = this->joint_count_;
        this->joint_count_ += file->data_->skins[s].joints_count;
    }

    this->joints_ = alloc(vk_mesh_joint, this->joint_count_);
    for (size_t s = 0; s < file->data_->skins_count; s++)
    {
        size_t joint_offset = this->skin_offsets_[s];
        cgltf_skin* skin = file->data_->skins + s;

        float* matrices = calloc(skin->inverse_bind_matrices->count, sizeof(float[16]));
        cgltf_accessor_unpack_floats(skin->inverse_bind_matrices, matrices, 16 * skin->inverse_bind_matrices->count);

        for (size_t i = 0; i < skin->joints_count; i++)
        {
            this->joints_[joint_offset + i].joint_ =
                desc->mapping_[GET_IDX(skin->joints[i], file->data_->nodes)]; // using linearized node
            memcpy(this->joints_[joint_offset + i].inv_binding_, matrices + 16 * i, 16 * sizeof(matrices[0]));
        }

        free(matrices);
    }

    for (size_t i = 0; i < desc->node_count_; i++)
    {
        uint32_t skin_idx = GET_IDX(file->data_->nodes[i].skin, file->data_->skins);
        if (skin_idx != -1)
        {
            if (file->data_->nodes[i].mesh)
            {
                vk_prim_transform* transform = desc->transform_ //
                                               + GET_IDX(file->data_->nodes[i].mesh, file->data_->meshes);
                transform->first_joint_ = this->skin_offsets_[skin_idx];
            }
        }
    }

    return this;
}

IMPL_OBJ_DELETE(gltf_skin)
{
    ffree(this->skin_offsets_);
    ffree(this->joints_);
}
