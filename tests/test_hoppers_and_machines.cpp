#include <cassert>
#include <iostream>
#include "item/Item.hpp"
#include "sim/ItemEntity.hpp"
#include "sim/ProjectileEntity.hpp"
#include "world/AutomationBlockEntities.hpp"
#include "world/ChestBlockEntity.hpp"
#include "world/CrafterBlockEntity.hpp"
#include "world/DispenserBlockEntity.hpp"
#include "world/DropperBlockEntity.hpp"
#include "world/FurnaceBlockEntity.hpp"
#include "world/HopperBlockEntity.hpp"
#include "storage/WorldFormat.hpp"
#include "world/TrialSpawnerBlockEntity.hpp"
#include "world/VaultBlockEntity.hpp"
#include "world/World.hpp"

using namespace OpenCraft;

static ItemId item(const char* id) {
    ItemId r = ItemRegistry::global().getId(id);
    assert(r != EMPTY_ITEM_ID);
    return r;
}

template <class T>
static std::shared_ptr<T> put(World& w, BlockPos p, std::shared_ptr<T> e) {
    w.setBlockEntity(p.x, p.y, p.z, e);
    return e;
}

static void runTicks(HopperBlockEntity& h, World& w, std::vector<ItemEntity>& items, int n) {
    for (int i = 0; i < n; ++i) h.stepTransfer(w, items);
}

int main() {
    std::cout << "--- Tolvas y máquinas ---" << std::endl;
    World w;
    w.setChunk(0, 0, std::make_unique<Chunk>(0, 0));
    std::vector<ItemEntity> items;
    const ItemId cobble = item("opencraft:cobblestone");

    // 1. Cofre arriba → tolva → cofre abajo, de uno en uno con enfriamiento
    auto top = put(w, {2, 72, 2}, std::make_shared<ChestBlockEntity>(BlockPos{2, 72, 2}));
    auto hopper = put(w, {2, 71, 2}, std::make_shared<HopperBlockEntity>(BlockPos{2, 71, 2}));
    auto bottom = put(w, {2, 70, 2}, std::make_shared<ChestBlockEntity>(BlockPos{2, 70, 2}));
    top->getInventory().setSlot(0, ItemStack(cobble, 3));
    assert(hopper->stepTransfer(w, items) && hopper->getCooldown() == HopperBlockEntity::TRANSFER_COOLDOWN_TICKS);
    assert(!hopper->stepTransfer(w, items) && "En enfriamiento");
    runTicks(*hopper, w, items, 200);
    assert(top->getInventory().getSlot(0).isEmpty());
    assert(bottom->getInventory().hasItem(cobble, 3) && "Todo acaba en el cofre inferior");
    hopper->setLocked(true);
    top->getInventory().setSlot(0, ItemStack(cobble, 1));
    assert(!hopper->stepTransfer(w, items) && hopper->isLocked());
    hopper->setLocked(false);

    // 2. Succión de objetos en el suelo encima de la tolva
    top->getInventory().clear();
    ItemEntity ie;
    ie.id = 77;
    ie.stack = ItemStack(cobble, 2);
    ie.position = Math::Vec3(2.5f, 72.0f, 2.5f);
    items.push_back(ie);
    w.removeBlockEntity(2, 72, 2);
    hopper->setCooldown(0);
    assert(hopper->stepTransfer(w, items) && items[0].isDead);

    // 3. Tolvas laterales hacia horno, soltador, dispensador, otra tolva y fabricador
    struct {
        HopperFacing f;
        BlockPos target;
    } sides[] = {
        {HopperFacing::North, {6, 71, 5}},
        {HopperFacing::South, {6, 71, 7}},
        {HopperFacing::West, {5, 71, 6}},
        {HopperFacing::East, {7, 71, 6}},
    };
    put(w, sides[0].target, std::make_shared<FurnaceBlockEntity>(sides[0].target));
    auto dropper =
        put(w, sides[1].target, std::make_shared<DropperBlockEntity>(sides[1].target, BlockDirection::South));
    auto dispenser = put(w, sides[2].target, std::make_shared<DispenserBlockEntity>(sides[2].target));
    auto crafter = put(w, sides[3].target, std::make_shared<CrafterBlockEntity>(sides[3].target));
    crafter->toggleSlotDisabled(0);
    for (auto& sd : sides) {
        w.removeBlockEntity(6, 71, 6);
        auto h = put(w, {6, 71, 6}, std::make_shared<HopperBlockEntity>(BlockPos{6, 71, 6}, sd.f));
        h->getInventory().setSlot(0, ItemStack(cobble, 2));
        runTicks(*h, w, items, 40);
        assert(h->getInventory().getSlot(0).isEmpty() && "La tolva lateral vacía su contenido en el destino");
    }
    assert(crafter->getInventory().getSlot(0).isEmpty() && "La ranura deshabilitada no recibe objetos");
    assert(!crafter->getInventory().getSlot(1).isEmpty());
    auto chained = put(w, {6, 70, 6}, std::make_shared<HopperBlockEntity>(BlockPos{6, 70, 6}));
    w.removeBlockEntity(6, 71, 6);
    auto h2 = put(w, {6, 71, 6}, std::make_shared<HopperBlockEntity>(BlockPos{6, 71, 6}));
    h2->getInventory().setSlot(0, ItemStack(cobble, 1));
    runTicks(*h2, w, items, 20);
    assert(chained->getInventory().hasItem(cobble, 1) && "Tolva hacia tolva");

    // 4. Soltador y dispensador disparan al frente; vacíos no hacen nada
    std::vector<ItemEntity> out;
    std::vector<ProjectileEntity> proj;
    assert(AutomationActions::getFacingOffset(BlockDirection::Up).y == 1);
    assert(AutomationActions::getFacingOffset(BlockDirection::East).x == 1);
    assert(AutomationActions::getFacingOffset(BlockDirection::North).z == -1);
    dropper->getInventory().clear();
    assert(!AutomationActions::triggerDropper(w, *dropper, out));
    dropper->getInventory().setSlot(3, ItemStack(cobble, 2));
    assert(AutomationActions::triggerDropper(w, *dropper, out) && out.size() == 1);
    assert(dropper->getInventory().getSlot(3).count == 1);
    // Soltador apuntando a un cofre: inserta en lugar de soltar
    auto front = put(w, {6, 71, 8}, std::make_shared<ChestBlockEntity>(BlockPos{6, 71, 8}));
    assert(AutomationActions::triggerDropper(w, *dropper, out) && front->getInventory().hasItem(cobble, 1));

    dispenser->getInventory().clear();
    assert(!AutomationActions::triggerDispenser(w, *dispenser, out, proj));
    dispenser->getInventory().setSlot(0, ItemStack(item("opencraft:arrow"), 3));
    assert(AutomationActions::triggerDispenser(w, *dispenser, out, proj) && proj.size() == 1 &&
           "Las flechas se disparan");
    dispenser->getInventory().setSlot(1, ItemStack(cobble, 1));
    dispenser->getInventory().setSlot(0, ItemStack());
    size_t before = out.size();
    assert(AutomationActions::triggerDispenser(w, *dispenser, out, proj) && out.size() == before + 1);

    // 5. Fabricador: palos a partir de dos tablones en columna
    auto cr = std::make_shared<CrafterBlockEntity>(BlockPos{10, 71, 10});
    put(w, {10, 71, 10}, cr);
    assert(!AutomationActions::triggerCrafter(w, *cr, out) && "Sin receta no fabrica");
    cr->getInventory().setSlot(0, ItemStack(item("opencraft:planks"), 1));
    cr->getInventory().setSlot(3, ItemStack(item("opencraft:planks"), 1));
    before = out.size();
    assert(AutomationActions::triggerCrafter(w, *cr, out));
    assert(out.size() == before + 1 && out.back().stack.itemId == item("opencraft:stick"));
    assert(cr->getInventory().getSlot(0).isEmpty() && cr->getInventory().getSlot(3).isEmpty());

    // 6. Guardado: todas las máquinas sobreviven a extraer → serializar → leer → aplicar
    auto spawner = put(w, {12, 71, 12}, std::make_shared<TrialSpawnerBlockEntity>(BlockPos{12, 71, 12}));
    spawner->setOminous(true);
    auto vault = put(w, {13, 71, 12}, std::make_shared<VaultBlockEntity>(BlockPos{13, 71, 12}));
    vault->markPlayerUnlocked(42);
    cr->getInventory().setSlot(4, ItemStack(item("opencraft:planks"), 5));
    cr->toggleSlotDisabled(8);
    dispenser->getInventory().setSlot(2, ItemStack(item("opencraft:arrow"), 7));
    hopper->getInventory().setSlot(1, ItemStack(cobble, 9));
    ChunkSaveData saved;
    WorldFormat::extractChunkData(*w.getChunk(0, 0), saved);
    std::vector<uint8_t> buf;
    assert(WorldFormat::serializeChunkData(saved, buf) == SerializationResult::Success);
    ChunkSaveData loaded;
    assert(WorldFormat::deserializeChunk(buf, loaded) == SerializationResult::Success);
    assert(loaded.hoppers.size() == saved.hoppers.size() && loaded.droppers.size() == saved.droppers.size());
    assert(loaded.dispensers.size() == 1 && loaded.crafters.size() == saved.crafters.size());
    assert(loaded.trialSpawners.size() == 1 && loaded.vaults.size() == 1);
    World w2;
    w2.setChunk(0, 0, std::make_unique<Chunk>(0, 0));
    WorldFormat::applyChunkData(loaded, *w2.getChunk(0, 0));
    auto d2 = std::dynamic_pointer_cast<DispenserBlockEntity>(w2.getBlockEntity(5, 71, 6));
    assert(d2 && d2->getInventory().hasItem(item("opencraft:arrow"), 7));
    auto h3 = std::dynamic_pointer_cast<HopperBlockEntity>(w2.getBlockEntity(2, 71, 2));
    assert(h3 && h3->getInventory().hasItem(cobble, 9));
    auto c2 = std::dynamic_pointer_cast<CrafterBlockEntity>(w2.getBlockEntity(10, 71, 10));
    assert(c2 && c2->isSlotDisabled(8) && c2->getInventory().getSlot(4).count == 5);
    auto dr2 = std::dynamic_pointer_cast<DropperBlockEntity>(w2.getBlockEntity(6, 71, 7));
    assert(dr2 && dr2->getFacing() == BlockDirection::South);
    auto ts2 = std::dynamic_pointer_cast<TrialSpawnerBlockEntity>(w2.getBlockEntity(12, 71, 12));
    assert(ts2 && ts2->isOminous());
    auto v2 = std::dynamic_pointer_cast<VaultBlockEntity>(w2.getBlockEntity(13, 71, 12));
    assert(v2 && v2->hasPlayerUnlocked(42));
    // Búfer truncado: error, nunca un fallo de memoria
    for (size_t cut : {size_t(0), size_t(3), buf.size() / 2, buf.size() - 1}) {
        ChunkSaveData broken;
        assert(WorldFormat::deserializeChunk(std::span<const uint8_t>(buf.data(), cut), broken) !=
               SerializationResult::Success);
    }
    for (int r = 0; r <= 12; ++r) assert(serializationResultToString(static_cast<SerializationResult>(r)) != nullptr);
    std::cout << "[OK] Guardado de máquinas, generador de desafío y bóveda" << std::endl;

    std::cout << "EXITO: tolvas, soltadores, dispensadores y fabricadores verificados." << std::endl;
    return 0;
}
