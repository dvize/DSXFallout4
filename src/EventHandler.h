#pragma once

#include "DSXController.hpp"
#include <F4SE/F4SE.h>
#include <RE/Bethesda/UI.h>
#include <RE/Bethesda/PipboyManager.h>

namespace DSX
{
	// Forward declaration if needed, but RE namespace is typically available via CommonLibF4

	void RegisterEventHandlers();
	void CheckWeaponOnGameLoad();
	void HandlePipboyStateChange(bool isAnyMenuActive, bool wasAnyMenuActive);

	class EquipEventHandler : public RE::BSTEventSink<RE::ActorEquipManagerEvent::Event>
	{
	public:
		static EquipEventHandler* GetSingleton();

		RE::BSEventNotifyControl ProcessEvent(const RE::ActorEquipManagerEvent::Event& evn,
			RE::BSTEventSource<RE::ActorEquipManagerEvent::Event>* eventSource) override;

	private:
		EquipEventHandler() = default;
		EquipEventHandler(const EquipEventHandler&) = delete;
		EquipEventHandler(EquipEventHandler&&) = delete;
		EquipEventHandler& operator=(const EquipEventHandler&) = delete;
		EquipEventHandler& operator=(EquipEventHandler&&) = delete;
	};

	class MenuEventHandler : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
	{
	public:
		static MenuEventHandler* GetSingleton();
		RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent& a_event,
			RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_source) override;

	private:
		MenuEventHandler() = default;
		MenuEventHandler(const MenuEventHandler&) = delete;
		MenuEventHandler(MenuEventHandler&&) = delete;
		MenuEventHandler& operator=(const MenuEventHandler&) = delete;
		MenuEventHandler& operator=(MenuEventHandler&&) = delete;
	};

}  // namespace DSX
