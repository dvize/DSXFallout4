#include "EventHandler.h"
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <F4SE/F4SE.h>
#include "RE/Fallout.h"

#include "EventHandler.h"
#include <spdlog/spdlog.h>
#include <unordered_map>

extern std::vector<DSX::TriggerSetting> userTriggers;
extern std::vector<DSX::Packet> triggerPackets;
extern DSX::NetworkManager networkManager;
extern DSX::Packet lastLeftPacket;
extern DSX::Packet lastRightPacket;
extern std::string actionLeft;
extern std::string actionRight;

namespace
{
	std::unordered_map<uint32_t, RE::TESObjectWEAP::InstanceData> weaponInstanceCache;
	DSX::Packet prevLeftPacket;
	DSX::Packet prevRightPacket;
	bool isPipboyActive = false;
	bool isPauseMenuActive = false;

	bool HasWeaponFlag(const REX::EnumSet<RE::WEAPON_FLAGS, std::uint32_t>& flags, RE::WEAPON_FLAGS flag)
	{
		return flags.any(flag);
	}

	bool IsFormIDMatch(RE::TESForm* item, const std::string& formIDStr)
	{
		if (formIDStr.empty())
			return false;
		try {
			uint32_t inDec = std::stoul(formIDStr, nullptr, 16);
			return inDec == item->formID;
		} catch (const std::exception& e) {
			logger::error("Error converting FormID: {}", e.what());
			return false;
		}
	}

	struct WeaponProperties
	{
		bool isAutomatic = false;
		bool isChargingAttack = false;
		bool isChargingReload = false;
		bool isBoltAction = false;
		bool isHoldInputToPower = false;
		bool isRepeatableSingleFire = false;
		bool isSecondaryWeapon = false;
		bool hasScope = false;
	};

	WeaponProperties GetWeaponProperties(const REX::EnumSet<RE::WEAPON_FLAGS, std::uint32_t>& flags)
	{
		WeaponProperties props;
		props.isAutomatic = HasWeaponFlag(flags, RE::WEAPON_FLAGS::kAutomatic);
		props.isChargingAttack = HasWeaponFlag(flags, RE::WEAPON_FLAGS::kChargingAttack);
		props.isChargingReload = HasWeaponFlag(flags, RE::WEAPON_FLAGS::kChargingReload);
		props.isBoltAction = HasWeaponFlag(flags, RE::WEAPON_FLAGS::kBoltAction);
		props.isHoldInputToPower = HasWeaponFlag(flags, RE::WEAPON_FLAGS::kHoldInputToPower);
		props.isRepeatableSingleFire = HasWeaponFlag(flags, RE::WEAPON_FLAGS::kRepeatableSingleFire);
		props.isSecondaryWeapon = HasWeaponFlag(flags, RE::WEAPON_FLAGS::kSecondaryWeapon);
		props.hasScope = HasWeaponFlag(flags, RE::WEAPON_FLAGS::kHasScope);
		return props;
	}

	DSX::Packet CreateDefaultPacket(int controllerIndex, DSX::Trigger trigger)
	{
		DSX::Packet packet;
		packet.AddAdaptiveTrigger(controllerIndex, trigger, DSX::TriggerMode::Normal, { 0, 0, 0, 0 });
		packet.AddTriggerThreshold(controllerIndex, trigger, 0);  // No threshold
		return packet;
	}

	void ApplyWeaponTriggerSettings(const RE::TESObjectWEAP* weapon, bool isEquipping)
	{
		if (!weapon) {
			return;
		}

		RE::WEAPON_TYPE weaponType = static_cast<RE::WEAPON_TYPE>(weapon->weaponData.type.get());
		logger::info("{} of weapon type: {}",
			isEquipping ? "Equip detected" : "Unequip detected",
			static_cast<int>(weaponType));

		logger::info("Weapon FormID: {:x}", weapon->formID);

		if (!isEquipping) {
			actionLeft = "Default_Left";
			actionRight = "Default_Right";
			lastLeftPacket = triggerPackets[0];
			lastRightPacket = triggerPackets[1];
			networkManager.SendPacket(lastLeftPacket);
			networkManager.SendPacket(lastRightPacket);
			return;
		}

		// Check for custom FormID match first
		bool foundCustomWeapon = false;
		for (size_t i = 0; i < userTriggers.size(); i++) {
			if (!userTriggers[i].formID.empty() &&
				IsFormIDMatch(const_cast<RE::TESObjectWEAP*>(weapon), userTriggers[i].formID)) {
				logger::info("Found custom weapon FormID: {}", userTriggers[i].formID);
				foundCustomWeapon = true;
				if (userTriggers[i].triggerSide == static_cast<int>(DSX::Trigger::Left)) {
					actionLeft = "Custom_" + userTriggers[i].formID + "_Left";
					lastLeftPacket = triggerPackets[i];
					networkManager.SendPacket(lastLeftPacket);
				} else if (userTriggers[i].triggerSide == static_cast<int>(DSX::Trigger::Right)) {
					actionRight = "Custom_" + userTriggers[i].formID + "_Right";
					lastRightPacket = triggerPackets[i];
					networkManager.SendPacket(lastRightPacket);
				}
				break;
			}
		}

		if (!foundCustomWeapon) {
			auto player = RE::PlayerCharacter::GetSingleton();
			if (player && player->currentProcess && player->currentProcess->middleHigh) {
				auto& equipped = player->currentProcess->middleHigh->equippedItems;
				if (!equipped.empty() && equipped[0].item.object && equipped[0].item.instanceData) {
					auto instance = static_cast<RE::TESObjectWEAP::InstanceData*>(equipped[0].item.instanceData.get());
					auto it = weaponInstanceCache.find(weapon->formID);
					RE::TESObjectWEAP::InstanceData cachedInstance;
					if (it != weaponInstanceCache.end()) {
						cachedInstance = it->second;
					} else {
						cachedInstance = *instance;
						weaponInstanceCache[weapon->formID] = cachedInstance;
					}

					WeaponProperties props = GetWeaponProperties(cachedInstance.flags);
					logger::info(
						"Weapon properties - Auto: {}, Charging: {}, ChargingReload: {}, "
						"Bolt: {}, HoldPower: {}, RepeatingFire: {}, Secondary: {}, Scope: {}",
						props.isAutomatic, props.isChargingAttack, props.isChargingReload,
						props.isBoltAction, props.isHoldInputToPower, props.isRepeatableSingleFire,
						props.isSecondaryWeapon, props.hasScope);

					std::string category;
					switch (weaponType) {
					case RE::WEAPON_TYPE::kHandToHand:
						category = "HandToHand";
						logger::info("Applying hand-to-hand settings");
						actionLeft = "HandToHand_Left";
						actionRight = "HandToHand_Right";
						break;
					case RE::WEAPON_TYPE::kOneHandSword:
						category = "OneHandSword";
						logger::info("Applying one-handed sword settings");
						actionLeft = "OneHandSword_Left";
						actionRight = "OneHandSword_Right";
						break;
					case RE::WEAPON_TYPE::kOneHandDagger:
						category = "OneHandDagger";
						logger::info("Applying one-handed dagger settings");
						actionLeft = "OneHandDagger_Left";
						actionRight = "OneHandDagger_Right";
						break;
					case RE::WEAPON_TYPE::kOneHandAxe:
						category = "OneHandAxe";
						logger::info("Applying one-handed axe settings");
						actionLeft = "OneHandAxe_Left";
						actionRight = "OneHandAxe_Right";
						break;
					case RE::WEAPON_TYPE::kOneHandMace:
						category = "OneHandMace";
						logger::info("Applying one-handed mace settings");
						actionLeft = "OneHandMace_Left";
						actionRight = "OneHandMace_Right";
						break;
					case RE::WEAPON_TYPE::kTwoHandSword:
						category = "TwoHandSword";
						logger::info("Applying two-handed sword settings");
						actionLeft = "TwoHandSword_Left";
						actionRight = "TwoHandSword_Right";
						break;
					case RE::WEAPON_TYPE::kTwoHandAxe:
						category = "TwoHandAxe";
						logger::info("Applying two-handed axe settings");
						actionLeft = "TwoHandAxe_Left";
						actionRight = "TwoHandAxe_Right";
						break;
					case RE::WEAPON_TYPE::kBow:
						category = "Bow";
						logger::info("Applying bow settings");
						actionLeft = "Bow_Left";
						actionRight = "Bow_Right";
						break;
					case RE::WEAPON_TYPE::kStaff:
						category = "Staff";
						logger::info("Applying staff settings");
						actionLeft = "Staff_Left";
						actionRight = "Staff_Right";
						break;
					case RE::WEAPON_TYPE::kGun:
						if (props.isBoltAction) {
							category = "Gun_BoltAction";
							logger::info("Applying bolt action gun settings");
							actionLeft = "Gun_BoltAction_Left";
							actionRight = "Gun_BoltAction_Right";
						} else if (props.isChargingReload) {
							category = "Gun_ChargingReload";
							logger::info("Applying charging reload gun settings");
							actionLeft = "Gun_ChargingReload_Left";
							actionRight = "Gun_ChargingReload_Right";
						} else if (props.isChargingAttack) {
							category = "Gun_ChargingAttack";
							logger::info("Applying charging attack gun settings");
							actionLeft = "Gun_ChargingAttack_Left";
							actionRight = "Gun_ChargingAttack_Right";
						} else if (props.isHoldInputToPower) {
							category = "Gun_HoldInputToPower";
							logger::info("Applying hold-to-power gun settings");
							actionLeft = "Gun_HoldInputToPower_Left";
							actionRight = "Gun_HoldInputToPower_Right";
						} else if (props.isAutomatic) {
							category = "Gun_Automatic";
							logger::info("Applying automatic gun settings");
							actionLeft = "Gun_Automatic_Left";
							actionRight = "Gun_Automatic_Right";
						} else if (props.isRepeatableSingleFire) {
							category = "Gun_RepeatableSingleFire";
							logger::info("Applying repeatable single fire gun settings");
							actionLeft = "Gun_RepeatableSingleFire_Left";
							actionRight = "Gun_RepeatableSingleFire_Right";
						} else {
							category = "Gun_Regular";
							logger::info("Applying standard gun settings");
							actionLeft = "Gun_Regular_Left";
							actionRight = "Gun_Regular_Right";
						}
						break;
					case RE::WEAPON_TYPE::kGrenade:
						category = "Grenade";
						logger::info("Applying grenade settings");
						actionLeft = "Grenade_Left";
						actionRight = "Grenade_Right";
						break;
					case RE::WEAPON_TYPE::kMine:
						category = "Mine";
						logger::info("Applying mine settings");
						actionLeft = "Mine_Left";
						actionRight = "Mine_Right";
						break;
					default:
						logger::warn("Unhandled weapon type: {}", static_cast<int>(weaponType));
						return;
					}

					// Send packets for the determined category and update last sent packets
					for (size_t i = 0; i < userTriggers.size() && i < triggerPackets.size(); ++i) {
						const auto& setting = userTriggers[i];
						const auto& packet = triggerPackets[i];
						std::string categoryAction = setting.category + (setting.triggerSide == static_cast<int>(DSX::Trigger::Left) ? "_Left" : "_Right");
						if (setting.triggerSide == static_cast<int>(DSX::Trigger::Left) && categoryAction == actionLeft) {
							lastLeftPacket = packet;
							networkManager.SendPacket(lastLeftPacket);
						} else if (setting.triggerSide == static_cast<int>(DSX::Trigger::Right) && categoryAction == actionRight) {
							lastRightPacket = packet;
							networkManager.SendPacket(lastRightPacket);
						}
					}
				}
			}
		}
	}
}

namespace DSX
{
	EquipEventHandler* EquipEventHandler::GetSingleton()
	{
		static EquipEventHandler singleton;
		return &singleton;
	}

	RE::BSEventNotifyControl EquipEventHandler::ProcessEvent(const RE::ActorEquipManagerEvent::Event& evn,
		RE::BSTEventSource<RE::ActorEquipManagerEvent::Event>*)
	{
		// Check if the actor is the player
		if (!evn.actorAffected || !evn.actorAffected->IsPlayerRef()) {
			return RE::BSEventNotifyControl::kContinue;
		}

		// Get the item and ensure it’s a weapon
		const auto* item = evn.itemAffected;
		if (!item || !item->object || item->object->formType != RE::ENUM_FORM_ID::kWEAP) {
			return RE::BSEventNotifyControl::kContinue;
		}

		// Determine if equipping or unequipping
		bool isEquipping = evn.changeType == RE::ActorEquipManagerEvent::Type::Equip;
		auto weapon = static_cast<const RE::TESObjectWEAP*>(item->object);

		// Apply trigger settings
		ApplyWeaponTriggerSettings(weapon, isEquipping);

		return RE::BSEventNotifyControl::kContinue;
	}

	MenuEventHandler* MenuEventHandler::GetSingleton()
	{
		static MenuEventHandler singleton;
		logger::info("MenuEventHandler singleton created");
		return &singleton;
	}

	RE::BSEventNotifyControl MenuEventHandler::ProcessEvent(const RE::MenuOpenCloseEvent& a_event,
		RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
	{
		logger::info("Menu event: {} - {}", a_event.opening ? "Opening" : "Closing", a_event.menuName.c_str());

		bool prevAnyMenuActive = isPipboyActive || isPauseMenuActive;  // State before this event

		if (a_event.menuName == RE::PipboyMenu::MENU_NAME) {
			isPipboyActive = a_event.opening;
		} else if (a_event.menuName == "PauseMenu") {
			isPauseMenuActive = a_event.opening;
		}

		bool isAnyMenuActive = isPipboyActive || isPauseMenuActive;  // State after this event
		HandlePipboyStateChange(isAnyMenuActive, prevAnyMenuActive);

		return RE::BSEventNotifyControl::kContinue;
	}

	void HandlePipboyStateChange(bool isAnyMenuActive, bool wasAnyMenuActive)
	{
		logger::info("HandlePipboyStateChange called: isAnyMenuActive={}, wasAnyMenuActive={}",
			isAnyMenuActive, wasAnyMenuActive);

		if (isAnyMenuActive != wasAnyMenuActive) {
			if (isAnyMenuActive) {
				logger::info("Saving previous packets: Left={}, Right={}",
					lastLeftPacket.instructions.empty() ? "empty" : "set",
					lastRightPacket.instructions.empty() ? "empty" : "set");
				prevLeftPacket = lastLeftPacket;
				prevRightPacket = lastRightPacket;

				if (!triggerPackets.empty() && triggerPackets.size() >= 2) {
					lastLeftPacket = triggerPackets[0];   // Default Left
					lastRightPacket = triggerPackets[1];  // Default Right
					networkManager.SendPacket(lastLeftPacket);
					networkManager.SendPacket(lastRightPacket);
					actionLeft = "Default_Left";
					actionRight = "Default_Right";
					logger::info("Applied default triggers for {} menu",
						isPipboyActive ? "Pipboy" : "Pause");
				} else {
					logger::error("Trigger packets not initialized or insufficient size");
				}
			} else {
				logger::info("Restoring previous packets: Left={}, Right={}",
					prevLeftPacket.instructions.empty() ? "empty" : "set",
					prevRightPacket.instructions.empty() ? "empty" : "set");
				lastLeftPacket = prevRightPacket;
				lastRightPacket = prevRightPacket;
				networkManager.SendPacket(lastLeftPacket);
				networkManager.SendPacket(lastRightPacket);
				logger::info("Restored previous trigger settings after all menus closed");

				auto player = RE::PlayerCharacter::GetSingleton();
				if (player && player->currentProcess && player->currentProcess->middleHigh) {
					auto& equipped = player->currentProcess->middleHigh->equippedItems;
					if (!equipped.empty() && equipped[0].item.object &&
						equipped[0].item.object->formType == RE::ENUM_FORM_ID::kWEAP) {
						auto weapon = static_cast<const RE::TESObjectWEAP*>(equipped[0].item.object);
						ApplyWeaponTriggerSettings(weapon, true);
					}
				}
			}
		} else {
			logger::info("No state change detected");
		}
	}

	void RegisterEventHandlers()
	{
		logger::info("Registering event handlers");

		auto* equipManager = RE::ActorEquipManager::GetSingleton();
		if (equipManager) {
			equipManager->RegisterSink(EquipEventHandler::GetSingleton());
			logger::info("Registered ActorEquipManagerEvent handler");
		} else {
			logger::error("Failed to get ActorEquipManager singleton");
		}
	}

	void CheckWeaponOnGameLoad()
	{
		// Check equipped weapon on load
		logger::info("Game loaded, checking equipped weapon");
		auto player = RE::PlayerCharacter::GetSingleton();
		if (player && player->currentProcess && player->currentProcess->middleHigh) {
			auto& equipped = player->currentProcess->middleHigh->equippedItems;
			if (!equipped.empty() && equipped[0].item.object && equipped[0].item.object->formType == RE::ENUM_FORM_ID::kWEAP) {
				auto weapon = static_cast<const RE::TESObjectWEAP*>(equipped[0].item.object);
				ApplyWeaponTriggerSettings(weapon, true);  // Apply settings for equipped weapon
			} else {
				// No weapon equipped, set defaults
				lastLeftPacket = CreateDefaultPacket(0, DSX::Trigger::Left);
				lastRightPacket = CreateDefaultPacket(0, DSX::Trigger::Right);
				networkManager.SendPacket(lastLeftPacket);
				networkManager.SendPacket(lastRightPacket);
				actionLeft = "Default_Left";
				actionRight = "Default_Right";
			}
		}

	}
}

