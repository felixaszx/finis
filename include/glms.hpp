/**
 * @file glms.hpp
 * @author Felix Xing (felixaszx@outlook.com)
 * @brief 
 * @version 0.1
 * @date 2024-08-15
 * 
 * @copyright MIT License Copyright (c) 2024 Felixaszx (Felix Xing)
 * 
 */
#ifndef INCLUDE_GLMS_HPP
#define INCLUDE_GLMS_HPP

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_INLINE
#define GLM_FORCE_XYZW_ONLY
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/fwd.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace glms
{
    template <typename T>
    GLM_FUNC_QUALIFIER glm::mat<4, 4, T, glm::defaultp> perspective(T fovy, T aspect, T zNear, T zFar)
    {
        glm::mat<4, 4, T, glm::defaultp> tmp = glm::perspective(fovy, aspect, zNear, zFar);
        tmp[1][1] *= -1;
        return tmp;
    }

    template <typename G, typename T>
    void assign_value(G& glm_types, const T& arr, size_t limit = ~0)
    {
        for (int i = 0; i < glm_types.length() && i < limit; i++)
        {
            glm_types[i] = arr[i];
        }
    }

    namespace literal
    {
        inline float operator""_dg(long double degree)
        {
            return glm::radians(degree);
        }
        inline float operator""_dg(unsigned long long degree)
        {
            return glm::radians(static_cast<float>(degree));
        }
    }; // namespace literal
} // namespace glms

#endif // INCLUDE_GLMS_HPP
