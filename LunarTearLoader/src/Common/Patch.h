#include <vector>

class Patch {
public:
    void* address;
    std::vector<uint8_t> patchBytes;
    std::vector<uint8_t> originalBytes;

    Patch(uintptr_t addr, std::vector<uint8_t> patch);

    void Apply();
    void Revert();
};
