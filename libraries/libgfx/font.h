#pragma once

#include <std/string_view.h>

namespace gfx {

class Font {
public:
    virtual ~Font() = default;

    virtual size_t width() const = 0;
    virtual size_t height() const = 0;

    virtual StringView name() const = 0;
};

}