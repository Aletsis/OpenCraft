#pragma once

#include <cstdint>
#include <variant>
#include "math/Math.hpp"
#include "sim/Entity.hpp"

namespace OpenCraft {

enum class CommandResult : uint8_t {
    Success = 0,
    OutOfReach,
    Collision,
    InvalidBlock,
    Unbreakable,
    InvalidPosition,
    UnknownEntity,
    Rejected,
    RevisionConflict,
    InventoryFull,
    InvalidSlot,
    TradeOutOfStock,
    InsufficientPayment
};

struct MoveCommand {
    EntityId entityId = LOCAL_PLAYER_ENTITY_ID;
    Math::Vec3 moveInput{0.0f, 0.0f, 0.0f};
    float yaw = 0.0f;
    float pitch = 0.0f;
    bool jump = false;
    bool sprint = false;
    bool sneak = false;
};

struct BreakBlockCommand {
    EntityId entityId = LOCAL_PLAYER_ENTITY_ID;
    int x = 0;
    int y = 0;
    int z = 0;
};

struct PlaceBlockCommand {
    EntityId entityId = LOCAL_PLAYER_ENTITY_ID;
    int x = 0;
    int y = 0;
    int z = 0;
    uint16_t blockState = 0;
};

struct SelectSlotCommand {
    EntityId entityId = LOCAL_PLAYER_ENTITY_ID;
    size_t slot = 0;
};

enum class InventoryContainerType : uint8_t {
    PlayerInventory = 0,
    ChestContainer = 1,
    Cursor = 2,
    CraftingGrid = 3,
    CraftingOutput = 4,
    FurnaceContainer = 5,
    HopperContainer = 6,
    DropperContainer = 7,
    DispenserContainer = 8,
    CrafterContainer = 9
};

enum class InventoryAction : uint8_t {
    Move = 0,
    Swap = 1,
    Split = 2,
    QuickTransfer = 3
};

struct InventoryTransactionCommand {
    EntityId entityId = LOCAL_PLAYER_ENTITY_ID;
    InventoryAction action = InventoryAction::Move;
    InventoryContainerType fromContainer = InventoryContainerType::PlayerInventory;
    size_t fromSlot = 0;
    InventoryContainerType toContainer = InventoryContainerType::PlayerInventory;
    size_t toSlot = 0;
    uint16_t amount = 0;
    uint32_t expectedRevision = 0; // Para verificación optimista de concurrencia
};

struct DropItemCommand {
    EntityId entityId = LOCAL_PLAYER_ENTITY_ID;
    bool fromCursor = false;
    size_t slot = 0;
    uint16_t count = 0; // 0 significa todo el stack
};

struct GiveItemCommand {
    EntityId entityId = LOCAL_PLAYER_ENTITY_ID;
    uint16_t itemId = 0;
    uint16_t count = 1;
    uint16_t durability = 0;
};

struct InteractBlockCommand {
    EntityId entityId = LOCAL_PLAYER_ENTITY_ID;
    int x = 0;
    int y = 0;
    int z = 0;
};

struct CloseContainerCommand {
    EntityId entityId = LOCAL_PLAYER_ENTITY_ID;
};

struct CraftCommand {
    EntityId entityId = LOCAL_PLAYER_ENTITY_ID;
    bool craftAll = false;
};

enum class DigAction : uint8_t {
    Start = 0,
    Progress = 1,
    Cancel = 2,
    Complete = 3
};

struct DigBlockCommand {
    EntityId entityId = LOCAL_PLAYER_ENTITY_ID;
    int x = 0;
    int y = 0;
    int z = 0;
    DigAction action = DigAction::Start;
};

struct EatFoodCommand {
    EntityId entityId = LOCAL_PLAYER_ENTITY_ID;
};

struct RespawnCommand {
    EntityId entityId = LOCAL_PLAYER_ENTITY_ID;
};

struct SetGameModeCommand {
    EntityId entityId = LOCAL_PLAYER_ENTITY_ID;
    uint32_t gameMode = 0; // 0 = Survival, 1 = Creative
};

struct SleepInBedCommand {
    EntityId entityId = LOCAL_PLAYER_ENTITY_ID;
    int x = 0;
    int y = 0;
    int z = 0;
};

struct AttackEntityCommand {
    EntityId entityId = LOCAL_PLAYER_ENTITY_ID;
    EntityId targetEntityId = INVALID_ENTITY_ID;
};

struct UseShieldCommand {
    EntityId entityId = LOCAL_PLAYER_ENTITY_ID;
    bool isBlocking = true;
};

struct ShootBowCommand {
    EntityId entityId = LOCAL_PLAYER_ENTITY_ID;
    float charge = 1.0f; // 0.1f a 1.0f (fuerza de disparo)
};

struct TradeWithVillagerCommand {
    EntityId entityId = LOCAL_PLAYER_ENTITY_ID;
    EntityId villagerEntityId = INVALID_ENTITY_ID;
    size_t offerIndex = 0;
};

struct InteractMobCommand {
    EntityId entityId = LOCAL_PLAYER_ENTITY_ID;
    EntityId mobEntityId = INVALID_ENTITY_ID;
    std::string heldItem = "";
};

struct FishCommand {
    EntityId entityId = LOCAL_PLAYER_ENTITY_ID;
    int targetX = 0;
    int targetY = 0;
    int targetZ = 0;
};

struct ToggleCrafterSlotCommand {
    EntityId entityId = LOCAL_PLAYER_ENTITY_ID;
    int x = 0;
    int y = 0;
    int z = 0;
    size_t slot = 0;
};

// Selección de la paleta del modo creativo (count = 0 vacía el cursor)
struct CreativePickCommand {
    EntityId entityId = LOCAL_PLAYER_ENTITY_ID;
    uint16_t itemId = 0;
    uint16_t count = 0;
    bool toInventory = false;
};

// Doble pulsación de salto en creativo: alterna el vuelo
struct ToggleFlightCommand {
    EntityId entityId = LOCAL_PLAYER_ENTITY_ID;
};

using GameCommand =
    std::variant<MoveCommand, BreakBlockCommand, PlaceBlockCommand, SelectSlotCommand, InventoryTransactionCommand,
                 DropItemCommand, GiveItemCommand, InteractBlockCommand, CloseContainerCommand, CraftCommand,
                 DigBlockCommand, EatFoodCommand, RespawnCommand, SetGameModeCommand, SleepInBedCommand,
                 AttackEntityCommand, UseShieldCommand, ShootBowCommand, TradeWithVillagerCommand, InteractMobCommand,
                 FishCommand, ToggleCrafterSlotCommand, CreativePickCommand, ToggleFlightCommand>;

} // namespace OpenCraft
