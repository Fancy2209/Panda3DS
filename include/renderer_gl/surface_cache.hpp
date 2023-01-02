#pragma once
#include <functional>
#include <optional>
#include "surfaces.hpp"

// Surface cache class that can fit "capacity" instances of the "SurfaceType" class of surfaces
// SurfaceType *must* have all of the following
// - An "allocate" function that allocates GL resources for the surfaces
// - A "free" function that frees up all resources the surface is taking up
// - A "matches" function that, when provided with a SurfaceType object reference
// Will tell us if the 2 surfaces match (Only as far as location in VRAM, format, dimensions, etc)
// Are concerned. We could overload the == operator, but that implies full equality
// Including equality of the allocated OpenGL resources, which we don't want
// - A "valid" member that tells us whether the function is still valid or not
template <typename SurfaceType, size_t capacity>
class SurfaceCache {
    // Vanilla std::optional can't hold actual references
    using OptionalRef = std::optional<std::reference_wrapper<SurfaceType>>;
    static_assert(std::is_same<SurfaceType, ColourBuffer>() || std::is_same<SurfaceType, DepthBuffer>(),
        "Invalid surface type");

    size_t size;
    std::array<SurfaceType, capacity> buffer;

public:
    void reset() {
        size = 0;
        for (auto& e : buffer) { // Free the VRAM of all surfaces
            e.free();
        }
    }

    OptionalRef find(SurfaceType& other) {
        for (auto& e : buffer) {
            if (e.matches(other) && e.valid)
                return e;
        }

        return std::nullopt;
    }

    // Adds a surface object to the cache and returns it
    SurfaceType add(SurfaceType& surface) {
        if (size >= capacity) {
            Helpers::panic("Surface cache full! Add emptying!");
        }
        size++;

        // Find an invalid entry in the cache and overwrite it with the new surface
        for (auto& e : buffer) {
            if (!e.valid) {
                e = surface;
                e.allocate();
                return e;
            }
        }

        // This should be unreachable but helps to panic anyways
        Helpers::panic("Couldn't add surface to cache\n");
    }

    SurfaceType& operator[](size_t i) {
        return buffer[i];
    }
};
