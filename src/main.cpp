#include <f4se/Version.h>
#include <f4se/API.h>
#include <spdlog/spdlog.h>
#include <thread>
#include <mutex>
#include <chrono>
#include <fstream>
#include <algorithm>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>

#include "PCH.h"
#include "RE/Fallout.h"
#include "DSXController.hpp"
#include "EventHandler.h"

std::vector<DSX::TriggerSetting> userTriggers;
std::vector<DSX::Packet> triggerPackets;
DSX::NetworkManager networkManager;
DSX::Packet lastLeftPacket;
DSX::Packet lastRightPacket;
std::string actionLeft;
std::string actionRight;

namespace {
    
    // Forward declarations for F4SE plugin
    void PostInit();
    void F4SEAPI MessageHandler(F4SE::MessagingInterface::Message* a_message);
    
    // DualSense controller specific functions
    void InitializeDSX();
    void BackgroundThread(std::chrono::milliseconds interval);
    bool LoadTriggerSettings();
    void GeneratePackets();
    

    void PostInit() {
        const auto task = F4SE::GetTaskInterface();
        task->AddTask([]() {
            logger::info("DSXFallout4 post-initialization");
			InitializeDSX();
        });
    }

    void F4SEAPI MessageHandler(F4SE::MessagingInterface::Message* a_message)
	{
		switch (a_message->type) {
		case F4SE::MessagingInterface::kGameLoaded:
			{
				static std::once_flag guard;
				std::call_once(guard, PostInit);
				break;
			}
		case F4SE::MessagingInterface::kPostLoadGame:
			{
				logger::info("PostLoadGame: Registering handlers and checking weapons");
				DSX::RegisterEventHandlers();
				DSX::CheckWeaponOnGameLoad();
				break;
			}

		case F4SE::MessagingInterface::kGameDataReady:
			logger::info("GameDataReady: Registering UI sink");
			auto* ui = RE::UI::GetSingleton();
			if (ui) {
				ui->RegisterSink(DSX::MenuEventHandler::GetSingleton());
				logger::info("Registered MenuOpenCloseEvent handler");
			} else {
				logger::error("Failed to get UI singleton in kGameDataReady");
			}
			break;
		}
	}

    void InitializeDSX() {
        logger::info("Initializing DSXFallout4");
        
        // Initialize network connection
        if (!networkManager.Initialize()) {
            logger::error("Failed to initialize network connection");
            return;
        }
        
        // Load trigger settings from config
        if (!LoadTriggerSettings()) {
            logger::error("Failed to load trigger settings");
            return;
        }
        
        // Generate packets from settings
        GeneratePackets();
        
        // Start background thread for sending controller updates
        auto interval = std::chrono::milliseconds(10000); // 10 second update interval
        std::thread backgroundThread(BackgroundThread, interval);
        backgroundThread.detach();
        
        logger::info("DSXFallout4 initialized successfully");
    }

    bool LoadTriggerSettings() {
        try {
            std::ifstream stream("./Data/F4SE/Plugins/DSXFallout4Config.json");
            if (!stream.is_open()) {
                logger::error("Could not open config file");
                return false;
            }

            nlohmann::json j;
            stream >> j;
            userTriggers.clear();

            for (const auto& item : j) {
                DSX::TriggerSetting setting;
                item.at("Name").get_to(setting.name);
                item.at("Category").get_to(setting.category);
                item.at("CustomFormID").get_to(setting.formID);
                item.at("TriggerSide").get_to(setting.triggerSide);
                item.at("TriggerType").get_to(setting.triggerType);
                item.at("customTriggerMode").get_to(setting.customTriggerMode);
                item.at("playerLEDNewRev").get_to(setting.playerLEDNewRev);
                item.at("MicLEDMode").get_to(setting.micLEDMode);
                item.at("TriggerThreshold").get_to(setting.triggerThresh);
                item.at("ControllerIndex").get_to(setting.controllerIndex);
                item.at("TriggerParams").get_to(setting.triggerParams);
                item.at("RGBUpdate").get_to(setting.rgbUpdate);
                userTriggers.push_back(setting);
            }

            logger::info("Loaded {} trigger settings", userTriggers.size());
            return true;
        } catch (const std::exception& e) {
            logger::error("Exception in LoadTriggerSettings: {}", e.what());
            return false;
        }
    }

    void GeneratePackets() {
        triggerPackets.clear();
        
        for (const auto& trigger : userTriggers) {
            DSX::Packet packet;
            
            if (trigger.triggerType == static_cast<int>(DSX::TriggerMode::CustomTriggerValue)) {
                packet.AddCustomAdaptiveTrigger(
                    trigger.controllerIndex,
                    static_cast<DSX::Trigger>(trigger.triggerSide),
                    static_cast<DSX::TriggerMode>(trigger.triggerType),
                    static_cast<DSX::CustomTriggerValueMode>(trigger.customTriggerMode),
                    trigger.triggerParams
                );
            } else {
                packet.AddAdaptiveTrigger(
                    trigger.controllerIndex,
                    static_cast<DSX::Trigger>(trigger.triggerSide),
                    static_cast<DSX::TriggerMode>(trigger.triggerType),
                    trigger.triggerParams
                );
            }

            packet.AddTriggerThreshold(
                trigger.controllerIndex,
                static_cast<DSX::Trigger>(trigger.triggerSide),
                trigger.triggerThresh
            );

            if (!trigger.rgbUpdate.empty()) {
                packet.AddRGB(
                    trigger.controllerIndex,
                    trigger.rgbUpdate[0],
                    trigger.rgbUpdate[1],
                    trigger.rgbUpdate[2]
                );
            }

            packet.AddPlayerLED(
                trigger.controllerIndex,
                static_cast<DSX::PlayerLEDNewRevision>(trigger.playerLEDNewRev)
            );

            packet.AddMicLED(
                trigger.controllerIndex,
                static_cast<DSX::MicLEDMode>(trigger.micLEDMode)
            );

            triggerPackets.push_back(packet);
        }
        
        logger::info("Generated {} trigger packets", triggerPackets.size());
    }

    void BackgroundThread(std::chrono::milliseconds interval)
	{
		while (true) {
			if (!lastLeftPacket.instructions.empty()) {
				networkManager.SendPacket(lastLeftPacket);
			}
			if (!lastRightPacket.instructions.empty()) {
				networkManager.SendPacket(lastRightPacket);
			}
			std::this_thread::sleep_for(interval);
		}
	}

}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* a_f4se, F4SE::PluginInfo* a_info) {

    a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = Plugin::NAME.data();
	a_info->version = Plugin::VERSION[0];

    if (a_f4se->IsEditor()) {
        logger::critical("loaded in editor"sv);
        return false;
    }

    const auto ver = a_f4se->RuntimeVersion();
	/*if (ver < F4SE::RUNTIME_1_10_980) 
	{
        logger::critical("unsupported runtime v{}"sv, ver.string());
        return false;
    }*/
    return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se) {
    F4SE::Init(a_f4se);
    spdlog::set_pattern("[%Y-%m-%d %T.%e][%-16s:%-4#][%L]: %v"s);
    
    logger::info("DSXFallout4 v{}.{}.{} {} {} is loading"sv, 1, 4, 0, __DATE__, __TIME__);
    const auto runtimeVer = REL::Module::get().version();
    logger::info("Fallout 4 v{}.{}.{}"sv, runtimeVer[0], runtimeVer[1], runtimeVer[2]);
    
    const auto messaging = F4SE::GetMessagingInterface();
    if (!messaging || !messaging->RegisterListener(MessageHandler)) {
        return false;
    }

    return true;
}

F4SE_EXPORT constinit auto F4SEPlugin_Version = []() noexcept {
	F4SE::PluginVersionData data{};

	data.PluginName(Plugin::NAME.data());
	data.PluginVersion(Plugin::VERSION);
	data.AuthorName("dvize");
	data.UsesAddressLibrary(true);
	data.UsesSigScanning(false);
	data.IsLayoutDependent(true);
	data.HasNoStructUse(false);
	data.CompatibleVersions({ F4SE::RUNTIME_1_10_163, F4SE::RUNTIME_LATEST});

	return data;
}();

