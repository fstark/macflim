#pragma once

#include "grayscale.hpp"

#include <string>

namespace macflim
{

void watermark(grayscale &img, const std::string &s);
void burn_subtitle(grayscale &img, const std::string &sub);

} // namespace macflim
