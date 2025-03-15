#include "SKSE/logger.h"
#include "_ts_SKSEFunctions.h"

namespace _ts_SKSEFunctions {

	void InitializeLogging() {
		auto path = log_directory();
		if (!path) {
			report_and_fail("Unable to lookup SKSE logs directory.");
	}
		*path /= PluginDeclaration::GetSingleton()->GetName();
		*path += L".log";

		std::shared_ptr<spdlog::logger> log;
		if (IsDebuggerPresent()) {
			log = std::make_shared<spdlog::logger>(
				"Global", std::make_shared<spdlog::sinks::msvc_sink_mt>());
		}
		else {
			log = std::make_shared<spdlog::logger>(
				"Global", std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true));
		}
		log->set_level({ spdlog::level::level_enum::info });
		log->flush_on({ spdlog::level::level_enum::trace });

		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [%t] [%s:%#] %v");
	}

/******************************************************************************************/

	bool IsFormValid(RE::TESForm* form, bool checkDeleted) {
		// Copy from DBSkseFunctions - author: Dylbill
		if (!form) {
			return false;
		}
	
		if (IsBadReadPtr(form, sizeof(form))) {
			return false;
		}
	
		if (form->GetFormID() == 0) {
			return false;
		}
	
		if (checkDeleted) {
			if (form->IsDeleted()) {
				return false;
			}
		}
	
		return true;
	}
	
/******************************************************************************************/

	void SetAngle(RE::TESObjectREFR* a_ref, RE::NiPoint3 a_angle) {
		a_ref->data.angle = a_angle;
		a_ref->Update3DPosition(true);
	}

	void SetAngleX(RE::TESObjectREFR* a_ref, float a_angleX) {
		auto Angle = a_ref->GetAngle();
		Angle.x = a_angleX;
		a_ref->data.angle = Angle;
		a_ref->Update3DPosition(true);
	}

	void SetAngleY(RE::TESObjectREFR* a_ref, float a_angleY) {
		auto Angle = a_ref->GetAngle();
		Angle.y = a_angleY;
		a_ref->data.angle = Angle;
		a_ref->Update3DPosition(true);
	}

	void SetAngleZ(RE::TESObjectREFR* a_ref, float a_angleZ) {
		auto Angle = a_ref->GetAngle();
		Angle.z = a_angleZ;
		a_ref->data.angle = Angle;
		a_ref->Update3DPosition(true);
	}

/******************************************************************************************/

	void SetLookAt(RE::Actor* actor, RE::TESObjectREFR* target) {
		if (!actor || !target) {
			return;
		}

		auto* high = actor->GetActorRuntimeData().currentProcess->high;
		if (high) {
			high->SetHeadtrackTarget(RE::HighProcessData::HEAD_TRACK_TYPE::kScript, target);
		}
	}

	void ClearLookAt(RE::Actor* actor) {
		if (!actor) {
			return;
		}
		auto* high = actor->GetActorRuntimeData().currentProcess->high;
		if (high) {
			high->ClearHeadtrackTarget(RE::HighProcessData::HEAD_TRACK_TYPE::kScript, true);
		}
	}

/******************************************************************************************/

	void SendCustomEvent(std::string eventName, std::string result, float numArg, RE::TESObjectREFR* sender) {
		auto* args = RE::MakeFunctionArguments(std::string(eventName), std::string(result), float(numArg), (RE::TESObjectREFR*)sender);
		auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		if (vm) {
			vm->SendEventAll("OnCustomEvent", args);
		}
	}

/******************************************************************************************/

	bool CheckForPackage(RE::Actor* akActor, const RE::BGSListForm* Packagelist, RE::TESPackage* CheckPackage) {
		if (!akActor) {
			spdlog::error("_ts_SKSEFunctions - {}: error, akActor doesn't exist", __func__);
			return false;
		}

		if (!Packagelist) {
			return false;
		}

		if (!CheckPackage) {
			CheckPackage = akActor->GetCurrentPackage();
		}

		for (auto& form : Packagelist->forms) {
			if (form && form->As<RE::TESPackage>() == CheckPackage) {
				return true;
			}
		}

		return false;
	}

/******************************************************************************************/

	RE::TESCondition* condition_IsPlayerInRegion;
	bool IsPlayerInRegion(const std::string& regionName) {
	
		auto player = RE::PlayerCharacter::GetSingleton();
	
		if (!IsFormValid(player)) {
			spdlog::error("_ts_SKSEFunctions - {}: error, Could not access Player actor", __func__);
			return false;
		}
	
		if (!condition_IsPlayerInRegion) {
			spdlog::info("_ts_SKSEFunctions - {}: creating IsPlayerInBorderRegion condition", __func__);
			auto* conditionItem = new RE::TESConditionItem;
		   
			conditionItem->data.comparisonValue.f = 1.0f;
			conditionItem->data.functionData.function = RE::FUNCTION_DATA::FunctionID::kIsPlayerInRegion;
			condition_IsPlayerInRegion = new RE::TESCondition;
			condition_IsPlayerInRegion->head = conditionItem;
		}
	
		auto* regionForm =  RE::TESForm::LookupByEditorID(regionName);
		if (!regionForm) {
			spdlog::error("_ts_SKSEFunctions - {}: Failed to lookup region form {}", __func__, regionName);
			return false;
		}
		condition_IsPlayerInRegion->head->data.functionData.params[0] = regionForm;
	
		return condition_IsPlayerInRegion->IsTrue(player, nullptr);
	}
	
/******************************************************************************************/
	
	RE::TESCondition* condition_IsFlyingMountPatrolQueued;
	bool IsFlyingMountPatrolQueued(RE::Actor* akActor) {
		if (!IsFormValid(akActor)) {
			spdlog::warn("_ts_SKSEFunctions - {}: error, akActor doesn't exist", __func__);
			return false;
		}

		if (!condition_IsFlyingMountPatrolQueued) {
			spdlog::info("_ts_SKSEFunctions - {}: creating IsFlyingMountPatrolQueued condition", __func__);
			auto* conditionItem = new RE::TESConditionItem;
			conditionItem->data.comparisonValue.f = 1.0f;
			conditionItem->data.functionData.function = RE::FUNCTION_DATA::FunctionID::kIsFlyingMountPatrolQueued;

			condition_IsFlyingMountPatrolQueued = new RE::TESCondition;
			condition_IsFlyingMountPatrolQueued->head = conditionItem;
		}

		return condition_IsFlyingMountPatrolQueued->IsTrue(akActor, nullptr);
	}

/******************************************************************************************/
	
	RE::TESCondition* condition_IsFlyingMountFastTravelling;
	bool IsFlyingMountFastTravelling(RE::Actor* akActor) {
		if (!IsFormValid(akActor)) {
			spdlog::warn("_ts_SKSEFunctions - {}: error, akActor doesn't exist", __func__);
			return false;
		}

		if (!condition_IsFlyingMountFastTravelling) {
			spdlog::info("_ts_SKSEFunctions - {}: creating IsFlyingMountFastTravelling condition", __func__);
			auto* conditionItem = new RE::TESConditionItem;
			conditionItem->data.comparisonValue.f = 1.0f;
			conditionItem->data.functionData.function = RE::FUNCTION_DATA::FunctionID::kIsFlyingMountFastTravelling;

			condition_IsFlyingMountFastTravelling = new RE::TESCondition;
			condition_IsFlyingMountFastTravelling->head = conditionItem;
		}

		return condition_IsFlyingMountFastTravelling->IsTrue(akActor, nullptr);
	}

/******************************************************************************************/

	float GetLandHeight(float a_x, float a_y, float a_z)
	{
		/* Only works in for coords wich lie in cells that are currently attached. Otherwise returns height -2048.0*/
		/* This code is copied from PO3_SKSEFunctions, because the function is not accessible via the released Papyrus Extender version*/
		float heightOut = -1;

		if (auto TES = RE::TES::GetSingleton()) {
			RE::NiPoint3 pos(a_x, a_y, a_z);
			TES->GetLandHeight(pos, heightOut);
		}

		spdlog::info("_ts_SKSEFunctions - {}: height: {}", __func__, heightOut);
		return heightOut;
	}

/******************************************************************************************/

	float GetLandHeightWithWater(RE::TESObjectREFR* a_ref)
	{
		/* Not yet tested: possibly only works in for coords wich lie in cells that are currently attached? Otherwise returns height -2048.0*/

		float heightOut = -1;

		if (auto TES = RE::TES::GetSingleton()) {
			TES->GetLandHeight(a_ref->GetPosition(), heightOut);
	
			auto cell = a_ref->GetParentCell();
			auto waterHeight = !cell || cell == a_ref->parentCell ? a_ref->GetWaterHeight() : cell->GetExteriorWaterHeight();

			if (waterHeight == -FLT_MAX && cell) {
				waterHeight = cell->GetExteriorWaterHeight();
			}

			if (heightOut < waterHeight) {
				heightOut = waterHeight;
			}
		}

		return heightOut;
	}

/******************************************************************************************/

	bool ClearCombatTargets(RE::Actor* a_actor)
	{
		if (!a_actor) {
			return false;
		}

		auto combatGroup = a_actor->GetCombatGroup();
		if (!combatGroup) {
			return false;
		}

		int i = 0;
		for (auto& targetData : combatGroup->targets) {
			auto target = targetData.targetHandle.get();
			i++;
		}
		
		combatGroup->targets.clear();
		a_actor->SetCombatGroup(combatGroup);

		return true;
	}

/******************************************************************************************/

	std::vector<RE::Actor*> GetCombatMembers(const RE::Actor* a_actor)
	{
		std::vector<RE::Actor*> result;

		if (!a_actor) {
			spdlog::info("_ts_SKSEFunctions - {}: Actor is None", __func__);
			return result;
		}

		spdlog::info("_ts_SKSEFunctions - {}", __func__);
		int i = 0;
		if (const auto combatGroup = a_actor->GetCombatGroup()) {
			for (auto& memberData : combatGroup->members) {
				auto member = memberData.memberHandle.get();
				spdlog::info("_ts_SKSEFunctions - {}: member[{}]: {}", __func__, i, member.get()->GetFormID());	
				if (member) {
					result.push_back(member.get());
				}
				i++;
			}
		}	
		i = 0;
		if (const auto combatGroup = a_actor->GetCombatGroup()) {
			for (auto& targetData : combatGroup->targets) {
				auto target = targetData.targetHandle.get();
				spdlog::info("_ts_SKSEFunctions - {}: target[{}]: {}", __func__, i, target.get()->GetFormID());	

				i++;
			}
		}

		return result;
	}
}


