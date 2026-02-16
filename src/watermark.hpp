#include <string>

#include "grayscale.hpp"

namespace macflim
{

void watermark(grayscale &img, const std::string &s);
void burn_subtitle(grayscale &img, const std::string &sub);

} // namespace macflim
