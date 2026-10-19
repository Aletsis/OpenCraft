#include <cassert>
#include <iostream>
#include "item/Item.hpp"
#include "sim/MobEntity.hpp"
#include "world/BlockRegistry.hpp"
#include "world/IdAliases.hpp"

using namespace OpenCraft;

// Los identificadores renombrados en 1.0.0-rc1 siguen resolviendo: mundos, comandos y paquetes
// anteriores cargan con los nombres nuevos
int main() {
    std::cout << "--- Alias de identificadores renombrados ---" << std::endl;
    auto& blocks = BlockRegistry::global();
    auto& items = ItemRegistry::global();
    size_t checked = 0;
    for (const auto& [oldId, newId] : IdAliases::table()) {
        if (blocks.hasBlock(newId)) {
            assert(blocks.hasBlock(oldId) && blocks.getStateId(oldId) == blocks.getStateId(newId));
            ++checked;
        }
        if (items.hasItem(newId)) {
            assert(items.hasItem(oldId) && items.getId(oldId) == items.getId(newId));
            ++checked;
        }
    }
    assert(checked >= 25 && "La mayoría de los alias deben corresponder a bloques u objetos registrados");
    assert(blocks.getStateId("opencraft:netherrack") == blocks.getStateId("opencraft:abyssal_rock"));
    assert(items.getId("opencraft:redstone") == items.getId("opencraft:cinnabar"));
    assert(mobTypeFromString("opencraft:creeper") == MobType::Lurker);
    assert(mobTypeFromString("opencraft:breeze") == MobType::Whirl);
    assert(mobTypeFromString("opencraft:bogged") == MobType::SwampSkeleton);
    assert(mobTypeToString(MobType::Lurker) == "opencraft:lurker");
    std::cout << "EXITO: " << checked << " alias de bloques y objetos verificados." << std::endl;
    return 0;
}
