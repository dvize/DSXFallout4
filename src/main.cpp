#include "PCH.h"
#include "RE/Fallout.h"
#include "PCH.h"
#include <nlohmann/json.hpp>
#include <ObjectArray.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <spdlog/spdlog.h>
#include <chrono>
#include <thread>


#define SERVER "127.0.0.1"  // or "localhost" - ip address of UDP server
#define BUFLEN 512          // max length of answer
#define PORT 6969           // the port on which to listen for incoming data

using std::string;
using std::vector;
using json = nlohmann::json;
using namespace RE;

#pragma warning(push)
#pragma warning(disable: 4189)


#pragma region Global Classes

class TriggerSetting
{
    private:
    public:
	    string name;
	    string category;
		string formID;
	    string description;
	    int triggerSide;
	    int triggerType;
	    int customTriggerMode;
	    int playerLEDNewRev;
	    int micLEDMode;
	    int triggerThresh;
	    int controllerIndex;
	    vector<int> triggerParams;
	    vector<int> rgbUpdate;
	    vector<bool> playerLED;

	    TriggerSetting()
	    {
		    controllerIndex = 0;
		    name = "Default";
			formID = "";
		    category = "One-Handed Bow";
		    description = "Drawing Hand";
		    triggerSide = 2;
		    triggerType = 14;
		    customTriggerMode = 0;
		    triggerParams = { 0, 8, 2, 5 };
		    rgbUpdate = { 0, 0, 0 };
		    playerLED = { false, false, false, false, false };
		    playerLEDNewRev = 5;
		    micLEDMode = 2;
		    triggerThresh = 0;
	    }
};

class Instruction
{
    public:
	    int type;
	    vector<string> parameters;

	    Instruction()
	    {
		    type = 1;
		    parameters = {};
	    }
};

class Packet
{
    public:
	    Instruction instructions[6];

	    Packet()
	    {
		    instructions[0].type = 1;
		    instructions[0].parameters = {};

		    instructions[1].type = 4;
		    instructions[1].parameters = {};

		    instructions[2].type = 2;
		    instructions[2].parameters = {};

		    instructions[3].type = 3;
		    instructions[3].parameters = {};

		    instructions[4].type = 6;
		    instructions[4].parameters = {};

		    instructions[5].type = 5;
		    instructions[5].parameters = {};
	    }
};

class TriggersCollection
{
    public:
	    vector<TriggerSetting> TriggersList;
};

json to_json(json& j, const TriggerSetting& p)
{
	j = {
		{ "Name", p.name },
		{ "Category", p.category },
		{ "CustomFormID", p.formID },
		{ "Description", p.description },
		{ "TriggerSide", p.triggerSide },
		{ "TriggerType", p.triggerType },
		{ "customTriggerMode", p.customTriggerMode },
		{ "playerLEDNewRev", p.playerLEDNewRev },
		{ "MicLEDMode", p.micLEDMode },
		{ "TriggerThreshold", p.triggerThresh },
		{ "ControllerIndex", p.controllerIndex },
		{ "TriggerParams", p.triggerParams },
		{ "RGBUpdate", p.rgbUpdate },
		{ "PlayerLED", p.playerLED }
	};

	return j;
}

void from_json(const json& j, TriggerSetting& p)
{
	j.at("Name").get_to(p.name);
	j.at("Category").get_to(p.category);
	j.at("CustomFormID").get_to(p.formID);
	j.at("Description").get_to(p.description);
	j.at("TriggerSide").get_to(p.triggerSide);
	j.at("TriggerType").get_to(p.triggerType);
	j.at("customTriggerMode").get_to(p.customTriggerMode);
	j.at("playerLEDNewRev").get_to(p.playerLEDNewRev);
	j.at("MicLEDMode").get_to(p.micLEDMode);
	j.at("TriggerThreshold").get_to(p.triggerThresh);
	j.at("ControllerIndex").get_to(p.controllerIndex);
	j.at("TriggerParams").get_to(p.triggerParams);
	j.at("RGBUpdate").get_to(p.rgbUpdate);
	j.at("PlayerLED").get_to(p.playerLED);
}

void to_json(json& j, const Instruction& p)
{
	j = {
		{ "type", p.type },
		{ "parameters", p.parameters }

	};
}

void from_json(const json& j, Instruction& p)
{
	j.at("type").get_to(p.type);
	j.at("parameters").get_to(p.parameters);
}

void to_json(json& j, const Packet& p)
{
	j = {
		{ "instructions", p.instructions }

	};
}

void from_json(const json& j, Packet& p)
{
	j.at("instructions").get_to(p.instructions);
}

void ReplaceStringInPlace(std::string& subject, const std::string& search,
	const std::string& replace)
{
	size_t pos = 0;
	while ((pos = subject.find(search, pos)) != std::string::npos) {
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
	}
}

json PacketToString(Packet& packet)
{
	logger::info("PacketToString function called");

	json j = packet;
	std::string s = j.dump();

	ReplaceStringInPlace(s, "\"", "");                            //removed from all to clear ints
	ReplaceStringInPlace(s, "instructions", "\"instructions\"");  // add back for instructions
	ReplaceStringInPlace(s, "type", "\"type\"");                  // add back for type
	ReplaceStringInPlace(s, "parameters", "\"parameters\"");

	return s;  //what is packet data structure with wierd names?
}


Packet StringToPacket(json j)
{
	logger::info("StringToPacket function called");
	auto conversion = j.get<Packet>();
	return conversion;
}


struct TECrapEvent
{
	NiPoint3 hitPos;                              //0x0000
	uint32_t unk_0C;                              //0x000C
	NiPoint3 normal;                              //0x0010
	uint32_t unk_1C;                              //0x001C
	NiPoint3 normal2;                             //0x0020 melee hits return 0,0,0
	uint32_t unk_2C;                              //0x002C
	bhkNPCollisionObject* colObj;                 //0x0030
	char pad_0x0038[0x18];                        //0x0038
	BGSAttackData* attackData;                    //0x0050
	TESObjectWEAP* weapon;                        //0x0058
	TESObjectWEAP::InstanceData* weaponInstance;  //0x0060
	char pad_0x0068[0x18];                        //0x0068
	TESForm* unkitem;                             //0x0080
	char pad_0x0088[0x8];                         //0x0088
	float unk90;                                  //0x0090
	float unk94;                                  //0x0094
	float unk98;                                  //0x0098
	float unk9C;                                  //0x009C
	char pad_0x00A0[0x10];                        //0x00A0
	float unkB0;                                  //0x00B0
	float unkB4;                                  //0x00B4
	char pad_0x00B8[0x8];                         //0x00B8
	float unkC0;                                  //0x00C0
	char pad_0x00C4[0xC];                         //0x00C4
	uint32_t bodypart;                            //0x00D0
	char pad_0x00D4[0x4C];                        //0x00D4
	TESObjectREFR* victim;                        //0x0120
	char pad_0x0128[0x38];                        //0x0128
	TESObjectREFR* attacker;                      //0x0160
};



#pragma endregion


#pragma region GlobalVariables

PlayerCharacter* player;
std::unordered_map<TESAmmo*, bool> ammoWhitelist;
using socket_t = decltype(socket(0, 0, 0));
socket_t mysocket;
sockaddr_in server;
TriggersCollection userTriggers;
vector<Packet> myPackets;
string actionLeft;
string actionRight;
const auto unequipped = 0x00000000ff000000;

#pragma endregion

#pragma region Utilities

char tempbuf[8192] = { 0 };
char* _MESSAGE(const char* fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vsnprintf(tempbuf, sizeof(tempbuf), fmt, args);
	va_end(args);
	spdlog::log(spdlog::level::warn, tempbuf);

	return tempbuf;
}

TESForm* GetFormFromMod(std::string modname, uint32_t formid)
{
	if (!modname.length() || !formid)
		return nullptr;
	TESDataHandler* dh = TESDataHandler::GetSingleton();
	TESFile* modFile = nullptr;
	for (auto it = dh->files.begin(); it != dh->files.end(); ++it) {
		TESFile* f = *it;
		if (strcmp(f->filename, modname.c_str()) == 0) {
			modFile = f;
			break;
		}
	}
	if (!modFile)
		return nullptr;
	uint8_t modIndex = modFile->compileIndex;
	uint32_t id = formid;
	if (modIndex < 0xFE) {
		id |= ((uint32_t)modIndex) << 24;
	} else {
		uint16_t lightModIndex = modFile->smallFileCompileIndex;
		if (lightModIndex != 0xFFFF) {
			id |= 0xFE000000 | (uint32_t(lightModIndex) << 12);
		}
	}
	return TESForm::GetFormByID(id);
}

ActorValueInfo* GetAVIFByEditorID(std::string editorID)
{
	TESDataHandler* dh = TESDataHandler::GetSingleton();
	BSTArray<ActorValueInfo*> avifs = dh->GetFormArray<ActorValueInfo>();
	for (auto it = avifs.begin(); it != avifs.end(); ++it) {
		if (strcmp((*it)->formEditorID.c_str(), editorID.c_str()) == 0) {
			return (*it);
		}
	}
	return nullptr;
}

bool CheckPowerArmor(Actor* a)
{
	if (!a->extraList) {
		return false;
	}
	return a->extraList->HasType(EXTRA_DATA_TYPE::kPowerArmor);
	;
}

#pragma warning(push)
#pragma warning(disable: 4267)
#pragma warning(disable: 4459)

void sendToDSX(string& s)
{
	//convert json to string and then char array
	char message[512];
	strcpy_s(message, s.c_str());
	for (int i = s.size(); i < 512; i++) {
		message[i] = '\0';
	}

	// send the message
	if (sendto(mysocket, message, strlen(message), 0, (sockaddr*)&server, sizeof(sockaddr_in)) == SOCKET_ERROR) {
		logger::error("sendtodsx() failed with error code: %d", WSAGetLastError());
	}
}

void readFromConfig()
{
	try {
		json j;
		std::ifstream stream(".\\Data\\F4SE\\Plugins\\DSXFallout4\\DSXFallout4Config.json");
		stream >> j;
		logger::info("JSON File Read from location");

		logger::info("Try assign j");

		TriggerSetting conversion;

		for (int i = 0; i < j.size(); i++) {
			conversion = j.at(i).get<TriggerSetting>();
			userTriggers.TriggersList.push_back(conversion);
		}

		logger::info("Breakpoint here to check value of userTriggers");

	} catch (std::exception e) {
		throw e;
	}
}

inline void writeToConfig(json j)
{
	try {

		const char* path = ".\\Data\\SKSE\\Plugins\\DSXSkyrim\\TestJson23.json";

		std::ofstream file_ofstream(path);
		file_ofstream << j.dump(4) << std::endl;

	} catch (std::exception& e) {
		logger::debug(FMT_STRING("Exception thrown :  {}"), e.what());
	}
	logger::info("JSON File Written and Closed Supposedly");
}

void setInstructionParameters(TriggerSetting& TempTrigger, Instruction& TempInstruction)
{
	switch (TempInstruction.type) {
	case 1:  //TriggerUpdate
		switch (TempTrigger.triggerParams.size()) {
		case 0:
			TempInstruction.parameters = { std::to_string(TempTrigger.controllerIndex), std::to_string(TempTrigger.triggerSide),
				std::to_string(TempTrigger.triggerType) };
			break;

		case 1:
			TempInstruction.parameters = { std::to_string(TempTrigger.controllerIndex), std::to_string(TempTrigger.triggerSide), std::to_string(TempTrigger.triggerType),
				std::to_string(TempTrigger.triggerParams.at(0)) };
			break;

		case 2:
			TempInstruction.parameters = { std::to_string(TempTrigger.controllerIndex), std::to_string(TempTrigger.triggerSide), std::to_string(TempTrigger.triggerType),
				std::to_string(TempTrigger.triggerParams.at(0)), std::to_string(TempTrigger.triggerParams.at(1)) };
			break;

		case 3:
			TempInstruction.parameters = { std::to_string(TempTrigger.controllerIndex), std::to_string(TempTrigger.triggerSide), std::to_string(TempTrigger.triggerType),
				std::to_string(TempTrigger.triggerParams.at(0)), std::to_string(TempTrigger.triggerParams.at(1)),
				std::to_string(TempTrigger.triggerParams.at(2)) };
			break;

		case 4:
			TempInstruction.parameters = { std::to_string(TempTrigger.controllerIndex), std::to_string(TempTrigger.triggerSide), std::to_string(TempTrigger.triggerType),
				std::to_string(TempTrigger.triggerParams.at(0)), std::to_string(TempTrigger.triggerParams.at(1)),
				std::to_string(TempTrigger.triggerParams.at(2)), std::to_string(TempTrigger.triggerParams.at(3)) };
			break;

		case 5:
			TempInstruction.parameters = { std::to_string(TempTrigger.controllerIndex), std::to_string(TempTrigger.triggerSide), std::to_string(TempTrigger.triggerType),
				std::to_string(TempTrigger.triggerParams.at(0)), std::to_string(TempTrigger.triggerParams.at(1)),
				std::to_string(TempTrigger.triggerParams.at(2)), std::to_string(TempTrigger.triggerParams.at(3)),
				std::to_string(TempTrigger.triggerParams.at(4)) };
			break;

		case 6:
			TempInstruction.parameters = { std::to_string(TempTrigger.controllerIndex), std::to_string(TempTrigger.triggerSide), std::to_string(TempTrigger.triggerType),
				std::to_string(TempTrigger.triggerParams.at(0)), std::to_string(TempTrigger.triggerParams.at(1)),
				std::to_string(TempTrigger.triggerParams.at(2)), std::to_string(TempTrigger.triggerParams.at(3)),
				std::to_string(TempTrigger.triggerParams.at(4)), std::to_string(TempTrigger.triggerParams.at(5)) };
			break;

		case 7:
			if (TempTrigger.triggerType == 12) {
				TempInstruction.parameters = { std::to_string(TempTrigger.controllerIndex), std::to_string(TempTrigger.triggerSide), std::to_string(TempTrigger.triggerType),
					std::to_string(TempTrigger.customTriggerMode), std::to_string(TempTrigger.triggerParams.at(0)), std::to_string(TempTrigger.triggerParams.at(1)),
					std::to_string(TempTrigger.triggerParams.at(2)), std::to_string(TempTrigger.triggerParams.at(3)),
					std::to_string(TempTrigger.triggerParams.at(4)), std::to_string(TempTrigger.triggerParams.at(5)),
					std::to_string(TempTrigger.triggerParams.at(6)) };
				break;
			} else {
				TempInstruction.parameters = { std::to_string(TempTrigger.controllerIndex), std::to_string(TempTrigger.triggerSide), std::to_string(TempTrigger.triggerType),
					std::to_string(TempTrigger.triggerParams.at(0)), std::to_string(TempTrigger.triggerParams.at(1)),
					std::to_string(TempTrigger.triggerParams.at(2)), std::to_string(TempTrigger.triggerParams.at(3)),
					std::to_string(TempTrigger.triggerParams.at(4)), std::to_string(TempTrigger.triggerParams.at(5)),
					std::to_string(TempTrigger.triggerParams.at(6)) };
				break;
			}

		default:
			TempInstruction.parameters = { std::to_string(TempTrigger.controllerIndex), std::to_string(TempTrigger.triggerSide), std::to_string(TempTrigger.triggerType) };
			break;
		}
		break;

	case 2:  //RGBUpdate
		TempInstruction.parameters = { std::to_string(TempTrigger.controllerIndex), std::to_string(TempTrigger.rgbUpdate.at(0)),
			std::to_string(TempTrigger.rgbUpdate.at(1)), std::to_string(TempTrigger.rgbUpdate.at(2)) };
		break;

	case 3:  //PlayerLED    --- parameters is set to vector<int> so the bool is not coming across. need to fix
		TempInstruction.parameters = {
			std::to_string(TempTrigger.controllerIndex), "false", "false", "false", "false", "false"
		};
		break;

	case 4:  //TriggerThreshold
		TempInstruction.parameters = { std::to_string(TempTrigger.controllerIndex), std::to_string(TempTrigger.triggerSide), std::to_string(TempTrigger.triggerThresh) };
		break;

	case 5:  //InstructionType.MicLED
		TempInstruction.parameters = { std::to_string(TempTrigger.controllerIndex), std::to_string(TempTrigger.micLEDMode) };
		break;

	case 6:  //InstructionType.PlayerLEDNewRevision
		TempInstruction.parameters = { std::to_string(TempTrigger.controllerIndex), std::to_string(TempTrigger.playerLEDNewRev) };
		break;
	}
}

void generatePacketInfo(TriggersCollection& userTriggers, vector<Packet>& myPackets)
{
	for (int i = 0; i < userTriggers.TriggersList.size(); i++) {
		Packet TempPacket;
		myPackets.push_back(TempPacket);

		myPackets.at(i).instructions[0].type = 1;  //InstructionType.TriggerUpdate
		setInstructionParameters(userTriggers.TriggersList.at(i), myPackets.at(i).instructions[0]);

		myPackets.at(i).instructions[2].type = 2;  //InstructionType.RGBUpdate
		setInstructionParameters(userTriggers.TriggersList.at(i), myPackets.at(i).instructions[2]);

		myPackets.at(i).instructions[3].type = 3;  //InstructionType.PlayerLED - fk this contains bools and mixed int
		setInstructionParameters(userTriggers.TriggersList.at(i), myPackets.at(i).instructions[3]);

		myPackets.at(i).instructions[1].type = 4;  //InstructionType.TriggerThreshold
		setInstructionParameters(userTriggers.TriggersList.at(i), myPackets.at(i).instructions[1]);

		myPackets.at(i).instructions[5].type = 5;  //InstructionType.MicLED
		setInstructionParameters(userTriggers.TriggersList.at(i), myPackets.at(i).instructions[5]);

		myPackets.at(i).instructions[4].type = 6;  //InstructionType.PlayerLEDNewRevision
		setInstructionParameters(userTriggers.TriggersList.at(i), myPackets.at(i).instructions[4]);
	}
}

#pragma warning(pop)

void background(std::chrono::milliseconds interval)
{
	while (1) {
		if (!actionLeft.empty()) {
			sendToDSX(actionLeft);
		}
		if (!actionRight.empty()) {
			sendToDSX(actionRight);
		}
		std::this_thread::sleep_for(interval);
	}
}

BGSKeywordForm* GetKeywordForm(TESObjectWEAP* wep)
{
	BGSKeywordForm* result = nullptr;

	result = static_cast<BGSKeywordForm*>(wep);
    return result;
}


RE::ENUM_FORM_ID getFormTypeFromID(int id)
{
	const auto base = RE::TESForm::GetFormByID(id)->GetFormType();
	return base;
}

RE::TESForm* getTESFormFromID(int id)
{
	const auto base = RE::TESForm::GetFormByID(id);

	return base;
}



#pragma endregion




#pragma region UDPServer

#ifndef WIN32
#	define WIN32_LEAN_AND_MEAN
#	include <WinSock2.h>
#endif

#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable: 4996)

#define SERVER "127.0.0.1"  // or "localhost" - ip address of UDP server
#define BUFLEN 512          // max length of answer
#define PORT 6969           // the port on which to listen for incoming data

#pragma warning(push)
#pragma warning(disable: 4244)

void UDPStart()
{
	// initialise winsock
	WSADATA ws;
	logger::info("Initialising Winsock..."sv);
	if (WSAStartup(MAKEWORD(2, 2), &ws) != 0) {
		logger::error("Failed. Error Code: %d", WSAGetLastError());
	}
	logger::info("Initialised.\n");

	// create socket

	if ((mysocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR)  // <<< UDP socket
	{
		logger::error("socket() failed with error code: %d", WSAGetLastError());
	}

	// setup address structure
	memset((char*)&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT);
	server.sin_addr.S_un.S_addr = inet_addr(SERVER);

}


#pragma warning(pop)
#pragma endregion



#pragma region Events

	//*********************Biped Slots********************
	// 30	-	0x1
	// 31	-	0x2
	// 32	-	0x4
	// 33	-	0x8
	// 34	-	0x10
	// 35	-	0x20
	// 36	-	0x40
	// 37	-	0x80
	// 38	-	0x100
	// 39	-	0x200
	// 40	-	0x400
	// 41	-	0x800
	// 42	-	0x1000
	// 43	-	0x2000
	// 44	-	0x4000
	// 45	-	0x8000
	//****************************************************

	struct TESEquipEvent
	{
		Actor* actor;         //00
		uint32_t formId;  //0C
		uint32_t unk08;   //08
		uint64_t flag;    //10 0x00000000ff000000 for unequip
	};

#pragma warning(push)
#pragma warning(disable: 4100)



	class EquipWatcher : public BSTEventSink<TESEquipEvent>
	{
	public:
		virtual BSEventNotifyControl ProcessEvent(const TESEquipEvent& evn, BSTEventSource<TESEquipEvent>* a_source)
		{
			TESForm* item = TESForm::GetFormByID(evn.formId);
			if (item && item->formType == ENUM_FORM_ID::kWEAP) {
				if (evn.actor->IsPlayerRef())
				{
					if (!(evn.flag == unequipped)) {

					    auto equippedWeapon = static_cast<TESObjectWEAP*>(item);
					    auto result = equippedWeapon->weaponData.type;
						logger::info(FMT_STRING("Equip Detected of WeaponData type: {}"), result);
						bool formidcustomweap = false;


						//check if its a custom form first before switching to generic weapontype
						for (int i = 20; i < userTriggers.TriggersList.size(); i++) {

							if (!(userTriggers.TriggersList[i].formID.empty())) {
								uint32_t inDec = std::stol(userTriggers.TriggersList[i].formID, nullptr, 16);

								if (inDec == item->formID) {
									logger::info(FMT_STRING("Found custom weapon of FormID: {}"), inDec);
									formidcustomweap = true;

									if (userTriggers.TriggersList[i].triggerSide == 1) {
										//left trigger assign for custom
										actionLeft = PacketToString(myPackets.at(i));

									} else if (userTriggers.TriggersList[i].triggerSide == 2) {
										//right trigger assign for custom
										actionRight = PacketToString(myPackets.at(i));
									}
								}
							}
						}

						if (!formidcustomweap) 
						{
							BSTArray<EquippedItem>& equipped = evn.actor->currentProcess->middleHigh->equippedItems;
							TESObjectWEAP* wep = ((TESObjectWEAP*)equipped[0].item.object);
							TESObjectWEAP::InstanceData* instance = (TESObjectWEAP::InstanceData*)equipped[0].item.instanceData.get();

							_MESSAGE("Equip Detected of Object Type Name: %llx", item->GetObjectTypeName());

							switch (result) {
							case 0:
								actionLeft = PacketToString(myPackets.at(2));  //aim
								actionRight = PacketToString(myPackets.at(3));
								break;
							case 1:
								actionLeft = PacketToString(myPackets.at(4));
								actionRight = PacketToString(myPackets.at(5));
								break;
							case 5:
								actionLeft = PacketToString(myPackets.at(6));
								actionRight = PacketToString(myPackets.at(7));
								break;
							case 6:
								actionLeft = PacketToString(myPackets.at(8));
								actionRight = PacketToString(myPackets.at(9));
								break;
							case 9:
								if (equipped.size() > 0 && equipped[0].item.instanceData &&
									(instance)->type == 9) {
									//detected this is a gun

									bool isAutomatic = (instance->flags & 0x00008000) == 0x00008000;
									logger::info(FMT_STRING("isAutomatic: {}"), isAutomatic);

									bool isChargingAttack = (instance->flags & 0x00000200) == 0x00000200;  //slow spin up until attack
									logger::info(FMT_STRING("isChargingAttack: {}"), isChargingAttack);

									bool isBoltAction = (instance->flags & 0x00400000) == 0x00400000;
									logger::info(FMT_STRING("isBoltAction: {}"), isBoltAction);

									bool isHoldInputToPower = (instance->flags & 0x00000800) == 0x00000800;  //charges while trigger held down then fires when released.
									logger::info(FMT_STRING("isHoldInputToPower: {}"), isHoldInputToPower);

									bool isRepeatableSingleFire = (instance->flags & 0x00010000) == 0x00010000;
									logger::info(FMT_STRING("isRepeatableSingleFire: {}"), isRepeatableSingleFire);

									logger::info("Placeholder text");

									if (isAutomatic) {
										actionLeft = PacketToString(myPackets.at(10));   //aim
										actionRight = PacketToString(myPackets.at(11));  //vibrating trigger
										logger::info(FMT_STRING("Loading is Automatic Settings: {}"), isAutomatic);
									} else if (isBoltAction) {
										actionLeft = PacketToString(myPackets.at(14));   //aim
										actionRight = PacketToString(myPackets.at(15));  //vibrating trigger
										logger::info(FMT_STRING("Loading is Bolt Action Settings: {}"), isBoltAction);
									}
									if (isChargingAttack) {
										actionLeft = PacketToString(myPackets.at(12));   //aim
										actionRight = PacketToString(myPackets.at(13));  //vibrating trigger
										logger::info(FMT_STRING("Loading Hold Input To PowerAttack Settings: {}"), isChargingAttack);
									} else if (isHoldInputToPower) {
										actionLeft = PacketToString(myPackets.at(16));   //aim
										actionRight = PacketToString(myPackets.at(17));  //vibrating trigger
										logger::info(FMT_STRING("Loading Hold Input To PowerAttack Settings: {}"), isHoldInputToPower);
									} else {
										actionLeft = PacketToString(myPackets.at(18));   //aim
										actionRight = PacketToString(myPackets.at(19));  //any other gun trigger (shotguns)
										logger::info("Loading Regular Gun Settings");
									}
								}
								break;

							default:
								break;
							}
						}

						if (actionLeft.empty() && actionRight.empty()) {
							return RE::BSEventNotifyControl::kContinue;
						} else if (!actionLeft.empty() && actionRight.empty()) {
							sendToDSX(actionLeft);
						} else if (actionLeft.empty() && !actionRight.empty()) {
							sendToDSX(actionRight);

						} else if (!actionLeft.empty() && !actionRight.empty()) {
							sendToDSX(actionLeft);
							sendToDSX(actionRight);
						}

					}else 
					{
						
					    auto equippedWeapon = static_cast<TESObjectWEAP*>(item);
						auto result = equippedWeapon->weaponData.type;
						logger::info(FMT_STRING("UnEquip Detected of type: {}"), result);

						actionLeft = PacketToString(myPackets.at(0));  //empty hands left setting
						actionRight = PacketToString(myPackets.at(1)); //empty hands right setting
						sendToDSX(actionLeft);
						sendToDSX(actionRight);
						
					}

					return BSEventNotifyControl::kContinue;
				}
				
			}
			return BSEventNotifyControl::kContinue;
		}
		F4_HEAP_REDEFINE_NEW(EquipEventSink);
	};

#pragma warning(pop)
#pragma endregion

#pragma region Event Sources

class EquipEventSource : public BSTEventSource<TESEquipEvent>
{
public:
	[[nodiscard]] static EquipEventSource* GetSingleton()
	{
		REL::Relocation<EquipEventSource*> singleton{ REL::ID(485633) };
		return singleton.get();
	}
};

#pragma endregion


#pragma region F4SE Initialize

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* a_f4se, F4SE::PluginInfo* a_info)
{
#ifndef NDEBUG
	auto sink = std::make_shared<spdlog::sinks::msvc_sink_mt>();
#else
	auto path = logger::log_directory();
	if (!path) {
		return false;
	}

	*path /= fmt::format(FMT_STRING("{}.log"), Version::PROJECT);
	auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
#endif

	auto log = std::make_shared<spdlog::logger>("global log"s, std::move(sink));

#ifndef NDEBUG
	log->set_level(spdlog::level::trace);
#else
	log->set_level(spdlog::level::info);
	log->flush_on(spdlog::level::warn);
#endif

	spdlog::set_default_logger(std::move(log));
	spdlog::set_pattern("%g(%#): [%^%l%$] %v"s);

	logger::info(FMT_STRING("{} v{}"), Version::PROJECT, Version::NAME);

	a_info->infoVersion = F4SE::PluginInfo::kVersion;
	a_info->name = Version::PROJECT.data();
	a_info->version = Version::MAJOR;

	if (a_f4se->IsEditor()) {
		logger::critical("loaded in editor"sv);
		return false;
	}

	const auto ver = a_f4se->RuntimeVersion();
	if (ver < F4SE::RUNTIME_1_10_162) {
		logger::critical(FMT_STRING("unsupported runtime v{}"), ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
#ifndef NDEBUG
	while (!IsDebuggerPresent()) {};
#endif

	F4SE::Init(a_f4se);

	logger::info("Loading JSON Files");
	readFromConfig();

	logger::info("Generate Packet Vector from Config");
	generatePacketInfo(userTriggers, myPackets);


	logger::info("Starting up UDP"sv);
	UDPStart();

	logger::info("Starting up Background Trigger Applier"sv);
	auto interval = std::chrono::milliseconds(100);
	std::thread background_worker(background, interval);
	background_worker.detach();

	const F4SE::MessagingInterface* message = F4SE::GetMessagingInterface();
	message->RegisterListener([](F4SE::MessagingInterface::Message* msg) -> void {
		if (msg->type == F4SE::MessagingInterface::kGameDataReady) {
			player = PlayerCharacter::GetSingleton();

			EquipWatcher* ew = new EquipWatcher();
			logger::info("Registering Equip Listener for Event Callback"sv);
			EquipEventSource::GetSingleton()->RegisterSink(ew);
		}

		if (msg->type == F4SE::MessagingInterface::kGameLoaded) {
			player = PlayerCharacter::GetSingleton();

			EquipWatcher* ew = new EquipWatcher();
			logger::info("Registering Equip Listener for Event Callback"sv);
			EquipEventSource::GetSingleton()->RegisterSink(ew);
		}
	});

	

	return true;
}

#pragma endregion


#pragma warning(pop)
