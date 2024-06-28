#ifndef INCLUDE_GLMS_HPP
#define INCLUDE_GLMS_HPP

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_INLINE
#define GLM_FORCE_XYZW_ONLY
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
    void assign_value(G& glm_types, const T& arr)
    {
        for (int i = 0; i < glm_types.length(); i++)
        {
            glm_types[i] = arr[i];
        }
    }
} // namespace glms

#endif // INCLUDE_GLMS_HPP
