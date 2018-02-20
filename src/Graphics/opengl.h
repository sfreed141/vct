#ifdef _WIN32
#define APIENTRY __stdcall
#endif

#include <glad/glad.h>

// put these into a 'layout class' and define one per  render pass? could pass to scene and loader
// enum class GLAttribute {
//     Position, Normal, Texcoord,
//     Tangent, Bitangent
// };

// enum class GLShaderTypes {}
// enum class GLTextureBindings {}