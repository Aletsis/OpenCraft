#pragma once

namespace OpenCraft {

class Chunk;

// Cálculo inicial de luz de un chunk aislado (sin vecinos): luz de cielo por columnas más
// propagación por inundación, y luz de bloques emisores. Se ejecuta al generar o cargar un chunk;
// los cambios posteriores los propaga LightEngine a través del mundo.
namespace ChunkLighting {

void computeChunkLight(Chunk& chunk);

} // namespace ChunkLighting

} // namespace OpenCraft
