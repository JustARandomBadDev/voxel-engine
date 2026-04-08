#ifndef VULKAN_CONFIG_H
#define VULKAN_CONFIG_H

#define DEBUG

#ifndef DEBUG
    inline constexpr bool enableValidationLayers = false;
#else
    inline constexpr bool enableValidationLayers = true;
#endif

inline constexpr int RENDER_DISTANCE = 16;
inline constexpr int CHUNK_SIZE      = 16;
inline constexpr int NB_FACE_CHUNK   = (CHUNK_SIZE / 2) * (CHUNK_SIZE / 2) * (CHUNK_SIZE / 2) * 6;
inline constexpr int MARGIN_BLOCKS   = 1;

#endif // VULKAN_CONFIG_H
