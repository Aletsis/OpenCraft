#include <cassert>
#include <iostream>
#include "item/Item.hpp"
#include "sim/WorldSession.hpp"
#include "world/BrewingStandBlockEntity.hpp"

using namespace OpenCraft;

using IC = InventoryContainerType;
using IA = InventoryAction;

static ItemId item(const char* id) {
    ItemId r = ItemRegistry::global().getId(id);
    assert(r != EMPTY_ITEM_ID);
    return r;
}

static BlockStateId block(BlockID id) { return BlockRegistry::global().fromLegacyId(id); }

static CommandResult tx(WorldSession& s, IA a, IC from, size_t fs, IC to, size_t ts, uint16_t amount = 0) {
    InventoryTransactionCommand c;
    c.action = a;
    c.fromContainer = from;
    c.fromSlot = fs;
    c.toContainer = to;
    c.toSlot = ts;
    c.amount = amount;
    return s.executeCommand(c);
}

// Plataforma de piedra en y = 99 con aire encima; el jugador queda en el centro
static void buildFloor(WorldSession& s) {
    auto& w = s.getWorld();
    for (int x = -8; x <= 8; ++x)
        for (int z = -8; z <= 8; ++z) {
            w.setBlockState(x, 99, z, block(BlockID::Stone));
            for (int y = 100; y < 106; ++y) w.setBlockState(x, y, z, AIR_BLOCK_STATE);
        }
    s.getPlayer().position = Math::Vec3(0.5f, 100.0f, 0.5f);
    s.getPlayer().prevPosition = s.getPlayer().position;
}

static void place(WorldSession& s, int x, int y, int z, BlockID id) {
    PlaceBlockCommand c;
    c.x = x;
    c.y = y;
    c.z = z;
    c.blockState = block(id);
    assert(s.executeCommand(c) == CommandResult::Success);
}

static CommandResult interact(WorldSession& s, int x, int y, int z) {
    InteractBlockCommand c;
    c.x = x;
    c.y = y;
    c.z = z;
    return s.executeCommand(c);
}

static void testCraftingGrid(WorldSession& s) {
    auto& p = s.getPlayer();
    p.inventory.clear();
    ItemStack planks(item("opencraft:planks"), 10);
    p.inventory.addItem(planks);
    size_t planksSlot = 0;
    for (size_t i = 0; i < p.inventory.getSize(); ++i)
        if (!p.inventory.getSlot(i).isEmpty()) {
            planksSlot = i;
            break;
        }

    // Inventario → cursor → rejilla (una unidad a cada casilla 2x2 con Split desde el cursor)
    assert(tx(s, IA::Move, IC::PlayerInventory, planksSlot, IC::Cursor, 0) == CommandResult::Success);
    assert(p.cursorItem.count == 10);
    for (size_t g = 0; g < 2; ++g)
        assert(tx(s, IA::Move, IC::Cursor, 0, IC::CraftingGrid, g, 1) == CommandResult::Success);
    assert(s.getCraftingGrid()[0].count == 1 && s.getCraftingGrid()[1].count == 1);
    assert(tx(s, IA::Move, IC::Cursor, 0, IC::CraftingGrid, 99) == CommandResult::InvalidSlot);

    // Dos tablones en vertical = palos; recolocar: rejilla → cursor y de nuevo a la columna
    assert(tx(s, IA::Move, IC::CraftingGrid, 1, IC::Cursor, 0) == CommandResult::Success);
    assert(tx(s, IA::Move, IC::Cursor, 0, IC::CraftingGrid, 2, 1) == CommandResult::Success);
    assert(!s.getCraftingOutput().isEmpty() && "Dos tablones en columna producen palos");
    assert(s.getCraftingOutput().itemId == item("opencraft:stick"));

    CraftCommand craft;
    assert(s.executeCommand(craft) == CommandResult::Success);
    assert(p.inventory.hasItem(item("opencraft:stick"), 4));
    assert(s.getCraftingOutput().isEmpty() && "Se consumieron los ingredientes");
    assert(s.executeCommand(craft) == CommandResult::Rejected);

    // Craftear todo: varias unidades en la rejilla
    for (int r = 0; r < 3; ++r) {
        assert(tx(s, IA::Move, IC::Cursor, 0, IC::CraftingGrid, 0, 1) == CommandResult::Success);
        assert(tx(s, IA::Move, IC::Cursor, 0, IC::CraftingGrid, 2, 1) == CommandResult::Success);
    }
    craft.craftAll = true;
    assert(s.executeCommand(craft) == CommandResult::Success);
    assert(p.inventory.hasItem(item("opencraft:stick"), 16));

    // Transferencia rápida de la rejilla y de la salida, y cursor de vuelta al inventario
    assert(tx(s, IA::Move, IC::Cursor, 0, IC::CraftingGrid, 3, 1) == CommandResult::Success);
    assert(tx(s, IA::QuickTransfer, IC::CraftingGrid, 3, IC::PlayerInventory, 0) == CommandResult::Success);
    assert(s.getCraftingGrid()[3].isEmpty());
    (void)tx(s, IA::QuickTransfer, IC::CraftingGrid, 3, IC::PlayerInventory, 0); // casilla vacía
    assert(tx(s, IA::QuickTransfer, IC::CraftingOutput, 0, IC::PlayerInventory, 0) == CommandResult::Success);
    (void)tx(s, IA::Move, IC::Cursor, 0, IC::PlayerInventory, 30);
    (void)tx(s, IA::Swap, IC::PlayerInventory, 30, IC::PlayerInventory, 31);
    (void)tx(s, IA::Split, IC::PlayerInventory, 31, IC::Cursor, 0);
    (void)tx(s, IA::Move, IC::Cursor, 0, IC::PlayerInventory, 31);
    (void)tx(s, IA::QuickTransfer, IC::PlayerInventory, 0, IC::PlayerInventory, 0);
    (void)tx(s, IA::QuickTransfer, IC::PlayerInventory, 20, IC::PlayerInventory, 0);
    assert(tx(s, IA::Move, IC::PlayerInventory, 999, IC::Cursor, 0) == CommandResult::InvalidSlot);
    std::cout << "[OK] Rejilla de fabricación, salida y transferencias" << std::endl;
}

static void testContainers(WorldSession& s) {
    auto& p = s.getPlayer();
    p.inventory.clear();
    p.cursorItem.clear();

    // Cofre: abrir, mover con Mayús+clic en ambos sentidos, cerrar
    place(s, 2, 100, 0, BlockID::Chest);
    assert(interact(s, 2, 100, 0) == CommandResult::Success);
    assert(s.getOpenContainerKind() == OpenContainerKind::Chest);
    p.inventory.setSlot(0, ItemStack(item("opencraft:cobblestone"), 32));
    assert(tx(s, IA::QuickTransfer, IC::PlayerInventory, 0, IC::ChestContainer, 0) == CommandResult::Success);
    auto chest = s.getWorld().getChestBlockEntity(2, 100, 0);
    assert(chest && chest->getInventory().hasItem(item("opencraft:cobblestone"), 32));
    assert(p.inventory.getSlot(0).isEmpty());
    assert(tx(s, IA::Move, IC::ChestContainer, 0, IC::Cursor, 0) == CommandResult::Success);
    assert(p.cursorItem.count == 32);
    assert(tx(s, IA::Move, IC::Cursor, 0, IC::ChestContainer, 5, 16) == CommandResult::Success);
    assert(p.cursorItem.count == 16);
    assert(tx(s, IA::Move, IC::Cursor, 0, IC::ChestContainer, 5) == CommandResult::Success);
    assert(p.cursorItem.isEmpty() && chest->getInventory().getSlot(5).count == 32);
    assert(tx(s, IA::Move, IC::ChestContainer, 5, IC::Cursor, 0, 8) == CommandResult::Success);
    assert(tx(s, IA::Move, IC::ChestContainer, 5, IC::Cursor, 0, 2) == CommandResult::Success);
    assert(p.cursorItem.count == 10 && chest->getInventory().getSlot(5).count == 22);
    assert(tx(s, IA::Move, IC::Cursor, 0, IC::ChestContainer, 6) == CommandResult::Success);
    assert(tx(s, IA::Move, IC::ChestContainer, 999, IC::Cursor, 0) == CommandResult::InvalidSlot);
    assert(tx(s, IA::QuickTransfer, IC::ChestContainer, 5, IC::PlayerInventory, 0) == CommandResult::Success);
    assert(tx(s, IA::Swap, IC::ChestContainer, 6, IC::PlayerInventory, 3) == CommandResult::Success);
    assert(s.executeCommand(CloseContainerCommand{}) == CommandResult::Success);
    assert(s.getOpenContainerKind() == OpenContainerKind::None);

    // Horno, tolva, soltador y dispensador se abren con su tipo
    place(s, -2, 100, 0, BlockID::Furnace);
    assert(interact(s, -2, 100, 0) == CommandResult::Success);
    assert(s.getOpenContainerKind() == OpenContainerKind::Furnace);
    p.inventory.setSlot(1, ItemStack(item("opencraft:coal"), 4));
    assert(tx(s, IA::QuickTransfer, IC::PlayerInventory, 1, IC::FurnaceContainer, 0) == CommandResult::Success);
    (void)tx(s, IA::Move, IC::FurnaceContainer, 1, IC::Cursor, 0);
    (void)tx(s, IA::QuickTransfer, IC::FurnaceContainer, 1, IC::PlayerInventory, 0);
    s.executeCommand(CloseContainerCommand{});

    struct {
        BlockID id;
        OpenContainerKind kind;
        IC ic;
    } machines[] = {
        {BlockID::Hopper, OpenContainerKind::Hopper, IC::HopperContainer},
        {BlockID::Dropper, OpenContainerKind::Dropper, IC::DropperContainer},
        {BlockID::Dispenser, OpenContainerKind::Dispenser, IC::DispenserContainer},
    };
    int z = 2;
    for (auto& m : machines) {
        place(s, 0, 100, z, m.id);
        assert(interact(s, 0, 100, z) == CommandResult::Success);
        assert(s.getOpenContainerKind() == m.kind);
        p.inventory.setSlot(2, ItemStack(item("opencraft:cobblestone"), 3));
        assert(tx(s, IA::QuickTransfer, IC::PlayerInventory, 2, m.ic, 0) == CommandResult::Success);
        assert(tx(s, IA::QuickTransfer, m.ic, 0, IC::PlayerInventory, 0) == CommandResult::Success);
        s.executeCommand(CloseContainerCommand{});
        z = -z;
        if (z > 0) z += 1;
    }
    // Contenedor sin abrir ni cercano: rechazado
    s.getPlayer().position = Math::Vec3(7.5f, 100.0f, 7.5f);
    assert(tx(s, IA::QuickTransfer, IC::PlayerInventory, 0, IC::CrafterContainer, 0) != CommandResult::Success);
    s.getPlayer().position = Math::Vec3(0.5f, 100.0f, 0.5f);
    assert(interact(s, 0, 100, 40) == CommandResult::OutOfReach);
    std::cout << "[OK] Cofre, horno, tolva, soltador y dispensador" << std::endl;
}

static void testDropCreativeAndModes(WorldSession& s) {
    auto& p = s.getPlayer();
    p.inventory.clear();
    p.cursorItem.clear();
    size_t before = s.getItemEntities().size();
    p.inventory.setSlot(0, ItemStack(item("opencraft:cobblestone"), 10));
    p.selectedSlot = 0;
    DropItemCommand d;
    d.slot = 0;
    d.count = 1;
    assert(s.executeCommand(d) == CommandResult::Success && p.inventory.getSlot(0).count == 9);
    d.count = 0;
    assert(s.executeCommand(d) == CommandResult::Success && p.inventory.getSlot(0).isEmpty());
    assert(s.executeCommand(d) == CommandResult::Rejected);
    p.cursorItem = ItemStack(item("opencraft:cobblestone"), 2);
    d.fromCursor = true;
    assert(s.executeCommand(d) == CommandResult::Success && p.cursorItem.isEmpty());
    assert(s.executeCommand(d) == CommandResult::Rejected);
    assert(s.getItemEntities().size() == before + 3);

    // Paleta y vuelo solo en creativo
    CreativePickCommand pick;
    pick.itemId = item("opencraft:diamond");
    pick.count = 200;
    assert(s.executeCommand(pick) == CommandResult::Rejected);
    assert(s.executeCommand(ToggleFlightCommand{}) == CommandResult::Rejected);
    SetGameModeCommand gm;
    gm.gameMode = 1;
    assert(s.executeCommand(gm) == CommandResult::Success && p.isCreative());
    assert(s.executeCommand(pick) == CommandResult::Success && p.cursorItem.count == 64);
    pick.toInventory = true;
    assert(s.executeCommand(pick) == CommandResult::Success && p.inventory.hasItem(pick.itemId, 64));
    pick.count = 0;
    assert(s.executeCommand(pick) == CommandResult::Success && p.cursorItem.isEmpty());
    pick.itemId = 60000;
    pick.count = 1;
    assert(s.executeCommand(pick) == CommandResult::Rejected);
    assert(s.executeCommand(ToggleFlightCommand{}) == CommandResult::Success && p.isFlying);
    assert(s.executeCommand(ToggleFlightCommand{}) == CommandResult::Success && !p.isFlying);
    gm.gameMode = 0;
    s.executeCommand(gm);

    // Comer: rechazado con la mano vacía o con algo que no es comida; con pan, recupera hambre
    p.inventory.clear();
    p.selectedSlot = 0;
    assert(s.executeCommand(EatFoodCommand{}) != CommandResult::Success);
    p.inventory.setSlot(0, ItemStack(item("opencraft:cobblestone"), 1));
    p.syncHotbar();
    assert(s.executeCommand(EatFoodCommand{}) == CommandResult::Rejected);
    p.inventory.setSlot(0, ItemStack(item("opencraft:bread"), 2));
    p.syncHotbar();
    p.hunger = 5.0f;
    assert(s.executeCommand(EatFoodCommand{}) == CommandResult::Success);
    assert(p.hunger > 5.0f && p.inventory.getSlot(0).count == 1);
    assert(s.executeCommand(RespawnCommand{}) == CommandResult::Success && "Vivo: reaparecer no hace nada");
    std::cout << "[OK] Soltar, paleta creativa, vuelo y comida" << std::endl;
}

static void testDigging(WorldSession& s) {
    auto& p = s.getPlayer();
    p.inventory.clear();
    s.getWorld().setBlockState(1, 100, 0, block(BlockID::Dirt));
    DigBlockCommand dig;
    dig.x = 1;
    dig.y = 100;
    dig.z = 0;
    assert(s.executeCommand(dig) == CommandResult::Success);
    dig.action = DigAction::Progress;
    (void)s.executeCommand(dig);
    dig.action = DigAction::Cancel;
    assert(s.executeCommand(dig) == CommandResult::Success);
    dig.action = DigAction::Start;
    assert(s.executeCommand(dig) == CommandResult::Success);
    bool broken = false;
    for (int t = 0; t < 200 && !broken; ++t) {
        dig.action = DigAction::Progress;
        s.executeCommand(dig);
        s.tick();
        broken = s.getWorld().getBlockState(1, 100, 0) == AIR_BLOCK_STATE;
        if (!broken && t % 5 == 4) {
            dig.action = DigAction::Complete;
            s.executeCommand(dig);
            broken = s.getWorld().getBlockState(1, 100, 0) == AIR_BLOCK_STATE;
        }
    }
    assert(broken && "La tierra se rompe a mano en supervivencia");
    dig.action = DigAction::Start;
    assert(s.executeCommand(dig) == CommandResult::InvalidBlock && "No se excava el aire");
    s.getWorld().setBlockState(1, 100, 0, block(BlockID::Bedrock));
    assert(s.executeCommand(dig) == CommandResult::Unbreakable);
    dig.y = 100000;
    assert(s.executeCommand(dig) == CommandResult::InvalidPosition);
    std::cout << "[OK] Excavación progresiva" << std::endl;
}

static void testBrewing() {
    World w;
    w.setChunk(0, 0, std::make_unique<Chunk>(0, 0));
    BrewingStandBlockEntity stand(BlockPos{1, 70, 1});
    assert(!stand.canBrew() && !stand.stepBrewing(w));
    auto& inv = stand.getInventory();
    inv.setSlot(BrewingStandBlockEntity::SLOT_INGREDIENT, ItemStack(item("opencraft:crimson_fungus"), 2));
    inv.setSlot(BrewingStandBlockEntity::SLOT_POTION_1, ItemStack(item("opencraft:potion"), 1)); // frasco de agua
    assert(stand.canBrew());
    assert(!stand.stepBrewing(w) && "Sin combustible no destila");
    inv.setSlot(BrewingStandBlockEntity::SLOT_FUEL, ItemStack(item("opencraft:ember_powder"), 1));
    for (uint32_t t = 0; t < BrewingStandBlockEntity::BREW_TIME_TOTAL; ++t) {
        stand.stepBrewing(w);
        if (t == 0) assert(stand.isBrewing() && stand.getFuel() > 0);
    }
    assert(!stand.isBrewing());
    const auto& out = inv.getSlot(BrewingStandBlockEntity::SLOT_POTION_1);
    assert(out.itemId == item("opencraft:potion") && out.components.size() >= 9 && "Poción rara");
    assert(inv.getSlot(BrewingStandBlockEntity::SLOT_INGREDIENT).count == 1);
    // La poción rara con cinabrio no es una receta válida
    inv.setSlot(BrewingStandBlockEntity::SLOT_INGREDIENT, ItemStack(item("opencraft:cinnabar"), 1));
    (void)stand.canBrew();
    stand.setBrewTime(5);
    (void)stand.stepBrewing(w);
    stand.incrementRevision();
    std::cout << "[OK] Soporte de pociones" << std::endl;
}

int main() {
    std::cout << "--- Sesión: inventario, fabricación, contenedores y excavación ---" << std::endl;
    WorldSession s;
    s.init(2, 1);
    s.getClock().setTickCount(1000);
    buildFloor(s);
    s.getPlayer().gameMode = 0;
    testCraftingGrid(s);
    testContainers(s);
    testDropCreativeAndModes(s);
    testDigging(s);
    testBrewing();
    std::cout << "EXITO: inventario, contenedores y excavación verificados." << std::endl;
    return 0;
}
