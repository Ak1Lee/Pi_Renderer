#pragma once

#include "Core/Mesh.h" // Inherit from base Mesh class

namespace Core {

class CubeMesh : public Mesh {
public:
    CubeMesh(); // Constructor will define cube's specific data and call Mesh::setupData
};

} // namespace Core
