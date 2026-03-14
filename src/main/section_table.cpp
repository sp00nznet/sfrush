// Section table for SF Rush N64 recomp

#pragma warning(push)
#pragma warning(disable: 4005)
#define static
#include "../../RecompiledFuncs/recomp_overlays.inl"
#undef static
#pragma warning(pop)

extern const size_t num_sections_export = sizeof(section_table) / sizeof(section_table[0]);
