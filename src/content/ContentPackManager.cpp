#include "content/ContentPackManager.hpp"
#include "sim/WorldSession.hpp"
#include "item/Item.hpp"
#include "world/BlockRegistry.hpp"
#include "storage/WorldFormat.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace OpenCraft {

bool ContentPackManager::validatePack(ContentPack& pack) const {
    if (pack.manifest.id.empty()) {
        pack.isValid = false;
        pack.validationError = "El paquete no tiene un identificador 'id' válido.";
        return false;
    }

    // 1. Validar dependencias
    for (const auto& dep : pack.manifest.dependencies) {
        auto it = m_packs.find(dep);
        if (it == m_packs.end() || !it->second.isValid) {
            pack.isValid = false;
            pack.validationError =
                "Dependencia insatisfecha: '" + dep + "' requerida por el paquete '" + pack.manifest.id + "'.";
            return false;
        }
    }

    // 2. Validar referencias a ítems en recetas
    auto& itemReg = ItemRegistry::global();
    for (const auto& recipe : pack.recipes) {
        if (itemReg.getId(recipe.resultItem) == EMPTY_ITEM_ID) {
            pack.isValid = false;
            pack.validationError = "Referencia rota a ítem resultado desconocido '" + recipe.resultItem +
                                   "' en receta de paquete '" + pack.manifest.id + "'.";
            return false;
        }
        for (const auto& ing : recipe.ingredients) {
            if (!ing.empty() && itemReg.getId(ing) == EMPTY_ITEM_ID) {
                pack.isValid = false;
                pack.validationError = "Referencia rota a ítem ingrediente desconocido '" + ing + "' en paquete '" +
                                       pack.manifest.id + "'.";
                return false;
            }
        }
    }

    pack.isValid = true;
    pack.validationError.clear();
    return true;
}

bool ContentPackManager::registerPack(ContentPack pack) {
    std::string id = pack.manifest.id;
    validatePack(pack);
    bool valid = pack.isValid;
    m_packs[id] = std::move(pack);
    return valid;
}

bool ContentPackManager::enablePack(const std::string& id) {
    auto it = m_packs.find(id);
    if (it == m_packs.end() || !it->second.isValid) {
        return false;
    }

    // Verificar si ya está habilitado
    if (std::find(m_enabledOrder.begin(), m_enabledOrder.end(), id) == m_enabledOrder.end()) {
        m_enabledOrder.push_back(id);
    }
    return true;
}

bool ContentPackManager::disablePack(const std::string& id) {
    auto it = std::find(m_enabledOrder.begin(), m_enabledOrder.end(), id);
    if (it != m_enabledOrder.end()) {
        m_enabledOrder.erase(it);
        return true;
    }
    return false;
}

bool ContentPackManager::isPackEnabled(const std::string& id) const {
    return std::find(m_enabledOrder.begin(), m_enabledOrder.end(), id) != m_enabledOrder.end();
}

std::string ContentPackManager::computeNetworkHash() const {
    if (m_enabledOrder.empty()) {
        return "OC_VANILLA_1.21_NO_PACKS";
    }

    std::ostringstream ss;
    for (const auto& id : m_enabledOrder) {
        auto it = m_packs.find(id);
        if (it != m_packs.end()) {
            // Serialización canónica de todo el contenido que altera las reglas: dos paquetes con el
            // mismo identificador pero recetas o funciones distintas producen hashes distintos (A12)
            const auto& pack = it->second;
            ss << "pack{" << pack.manifest.id.size() << ':' << pack.manifest.id << ':' << pack.manifest.version << '}';
            for (const auto& r : pack.recipes) {
                ss << "recipe{" << r.resultItem << 'x' << r.resultCount << (r.isShaped ? "|shaped|" : "|shapeless|")
                   << static_cast<int>(r.width) << 'x' << static_cast<int>(r.height) << '[';
                for (const auto& ing : r.ingredients) ss << ing.size() << ':' << ing << ',';
                ss << "]}";
            }
            for (const auto& adv : pack.advancements) {
                ss << "adv{" << adv.size() << ':' << adv << '}';
            }
            // Las funciones se recorren en orden de nombre: unordered_map no garantiza un orden estable
            std::vector<const std::pair<const std::string, std::vector<std::string>>*> fns;
            for (const auto& fn : pack.functions) fns.push_back(&fn);
            std::sort(fns.begin(), fns.end(), [](auto* a, auto* b) { return a->first < b->first; });
            for (const auto* fn : fns) {
                ss << "fn{" << fn->first << '[';
                for (const auto& line : fn->second) ss << line.size() << ':' << line << ';';
                ss << "]}";
            }
        }
    }

    std::string payload = ss.str();
    uint32_t crc =
        computeCRC32(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(payload.data()), payload.size()));

    std::ostringstream hexOut;
    hexOut << "OCPACK_" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << crc;
    return hexOut.str();
}

bool ContentPackManager::verifyNetworkHash(const std::string& remoteHash) const {
    return computeNetworkHash() == remoteHash;
}

void ContentPackManager::applyToSession(WorldSession& session) {
    for (const auto& id : m_enabledOrder) {
        auto it = m_packs.find(id);
        if (it != m_packs.end()) {
            // Cargar funciones en FunctionRunner
            for (const auto& fn : it->second.functions) {
                session.getFunctionRunner().registerFunction(fn.first, fn.second);
            }
            // Inyectar recetas
            auto& itemReg = ItemRegistry::global();
            auto& recipeReg = session.getRecipeRegistry();
            for (const auto& r : it->second.recipes) {
                ItemId resId = itemReg.getId(r.resultItem);
                if (resId != EMPTY_ITEM_ID) {
                    if (r.isShaped) {
                        std::vector<ItemId> grid;
                        for (const auto& ing : r.ingredients) {
                            grid.push_back(itemReg.getId(ing));
                        }
                        ShapedRecipe sr{.id = it->first + ":" + r.resultItem,
                                        .width = r.width,
                                        .height = r.height,
                                        .pattern = std::move(grid),
                                        .result = ItemStack(resId, r.resultCount)};
                        recipeReg.registerShaped(std::move(sr));
                    } else {
                        std::vector<ItemId> inputs;
                        for (const auto& ing : r.ingredients) {
                            inputs.push_back(itemReg.getId(ing));
                        }
                        ShapelessRecipe slr{.id = it->first + ":" + r.resultItem,
                                            .ingredients = std::move(inputs),
                                            .result = ItemStack(resId, r.resultCount)};
                        recipeReg.registerShapeless(std::move(slr));
                    }
                }
            }
        }
    }
}

} // namespace OpenCraft
