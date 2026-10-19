#include <iostream>
#include <cassert>
#include <random>
#include "item/Inventory.hpp"
#include "item/Item.hpp"

using namespace OpenCraft;

int main() {
    std::cout << "--- [F04 Prueba 1] Conservación estricta de recursos en inventario ---" << std::endl;

    // Crear inventario de 36 ranuras
    Inventory inv(36);

    auto& reg = ItemRegistry::global();
    ItemId diamondId = reg.getId("opencraft:diamond");
    inv.setSlot(0, ItemStack(diamondId, 64));

    // Agregar también un ítem con stack máximo 1
    // Espada de diamante (stack 1)
    ItemId swordId = reg.getId("opencraft:diamond_sword");
    inv.setSlot(1, ItemStack(swordId, 1, 100)); // durabilidad 100

    uint32_t expectedDiamondCount = 64;
    uint32_t expectedSwordCount = 1;

    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> slotDist(0, 35);
    std::uniform_int_distribution<int> actionDist(0, 3);
    std::uniform_int_distribution<uint16_t> amountDist(1, 64);

    const int iterations = 1500;
    std::cout << "Ejecutando " << iterations << " transacciones aleatorias (move, swap, split, etc.)..." << std::endl;

    for (int i = 0; i < iterations; ++i) {
        size_t slotA = slotDist(rng);
        size_t slotB = slotDist(rng);
        int action = actionDist(rng);
        uint16_t amount = amountDist(rng);

        switch (action) {
            case 0: inv.moveItem(slotA, slotB, amount); break;
            case 1: inv.swapSlots(slotA, slotB); break;
            case 2: inv.splitSlot(slotA, slotB, amount); break;
            case 3: inv.moveItem(slotA, slotB, 0); break;
        }

        // Invariante 1: Ningún slot puede exceder su maxStackSize
        for (size_t s = 0; s < inv.getSize(); ++s) {
            const auto& stack = inv.getSlot(s);
            if (!stack.isEmpty()) {
                if (stack.count > stack.getMaxStack()) {
                    std::cout << "DEBUG: violation at iteration " << i << ", action " << action << ", slot " << s
                              << ": itemId=" << stack.itemId << ", count=" << stack.count
                              << ", maxStack=" << stack.getMaxStack() << std::endl;
                }
                assert(stack.count <= stack.getMaxStack() && "Violación: stack excede tamaño máximo");
                assert(stack.count > 0 && "Violación: stack no vacío tiene cantidad 0");
            }
        }

        // Invariante 2: La suma total de diamantes debe ser exactamente 64
        assert(inv.countItem(diamondId) == expectedDiamondCount &&
               "Violación de conservación: diamantes perdidos o duplicados");
        assert(inv.countItem(swordId) == expectedSwordCount &&
               "Violación de conservación: espadas perdidas o duplicadas");
    }

    std::cout << "Verificación exitosa: 1500 transacciones completadas sin pérdida ni duplicación." << std::endl;
    std::cout << "Diamantes finales: " << inv.countItem(diamondId) << " (esperado 64)" << std::endl;
    std::cout << "Espadas finales: " << inv.countItem(swordId) << " (esperado 1)" << std::endl;
    std::cout << "[PASS] F04 Prueba 1 - Conservación de recursos completada con éxito." << std::endl;

    return 0;
}
