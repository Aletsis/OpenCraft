#include <cassert>
#include <filesystem>
#include <iostream>
#include "sim/WorldSession.hpp"
#include "storage/WorldStorage.hpp"

using namespace OpenCraft;

// El guardado de la sesión solo escribe chunks modificados o con criaturas, sin perder ni duplicar
// criaturas cuando cambian de chunk entre guardados
static size_t mobsAfterReload(const std::filesystem::path& dir) {
    WorldStorage storage(dir);
    WorldSession s;
    s.init(2, 5);
    StorageError err = s.loadFromStorage(storage);
    assert(err == StorageError::None);
    (void)err;
    return s.getMobs().size();
}

int main() {
    std::cout << "--- Guardado incremental y criaturas por chunk ---" << std::endl;
    std::filesystem::path dir = std::filesystem::temp_directory_path() / "opencraft_test_incremental_save";
    std::filesystem::remove_all(dir);
    WorldStorage storage(dir);
    assert(storage.initializeWorld("Incremental", 5) == StorageError::None);

    {
        WorldSession s;
        s.init(2, 5);
        s.attachStorage(&storage);
        s.getMobs().emplace_back(5000, MobType::Cow, Math::Vec3(4.0f, 80.0f, 4.0f));
        s.getMobs().emplace_back(5001, MobType::Pig, Math::Vec3(5.0f, 80.0f, 5.0f));
        assert(s.saveToStorage(storage) == StorageError::None);

        // Un segundo guardado sin cambios no reescribe los chunks intactos
        auto before = std::filesystem::last_write_time(storage.getChunkPath(1, 1));
        assert(s.saveToStorage(storage) == StorageError::None);
        assert(std::filesystem::last_write_time(storage.getChunkPath(1, 1)) == before &&
               "Un chunk sin cambios ni criaturas no debe reescribirse en cada autoguardado");
    }
    assert(mobsAfterReload(dir) == 2);

    {
        WorldStorage storage2(dir);
        WorldSession s;
        s.init(2, 5);
        assert(s.loadFromStorage(storage2) == StorageError::None);
        for (auto& m : s.getMobs()) m.position = Math::Vec3(20.0f, 80.0f, 20.0f); // se mudan al chunk (1,1)
        assert(s.saveToStorage(storage2) == StorageError::None);
    }
    assert(mobsAfterReload(dir) == 2 && "Las criaturas que cambian de chunk no se pierden ni se duplican");

    std::filesystem::remove_all(dir);
    std::cout << "EXITO: guardado incremental sin pérdida ni duplicación de criaturas." << std::endl;
    return 0;
}
