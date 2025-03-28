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

	// Function to pause a while loop if the game is in menu mode, console is open, or out of focus
	void WaitWhileGameIsPaused(int checkInterval_ms) {

		auto* ui = RE::UI::GetSingleton();
		auto* main = RE::Main::GetSingleton();

		while (ui && (ui->GameIsPaused() || ui->IsMenuOpen(RE::Console::MENU_NAME)) ||
		(main && !main->gameActive)) {
			std::this_thread::sleep_for(std::chrono::milliseconds(checkInterval_ms));
		}
	}

/******************************************************************************************/

	RE::VMHandle GetHandle(const RE::TESForm* akForm) {
        if (!(akForm)) {
            log::warn("{}: akForm doesn't exist or isn't valid", __func__);
            return NULL;
        }

        RE::VMTypeID id = static_cast<RE::VMTypeID>(akForm->GetFormType());
        RE::VMHandle handle = RE::SkyrimVM::GetSingleton()->handlePolicy.GetHandleForObject(id, akForm);

        if (handle == NULL) {
            return NULL;
        }

        return handle;
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

	void RegisterForSingleUpdate(RE::VMHandle handle, float delayInSeconds)
	{
		if (delayInSeconds < 0.0f) {
			spdlog::error("_ts_SKSEFunctions - {}: delayInSeconds is negative", __func__);
			return;
		}
		if (!handle) {
			spdlog::error("_ts_SKSEFunctions - {}: handle is None", __func__);
			return;
		}

		auto* skyrimVM = RE::SkyrimVM::GetSingleton();
		if (!skyrimVM) {
			spdlog::error("_ts_SKSEFunctions - {}: skyrimVM is None", __func__);
			return;
		}

		auto updateEvent = RE::BSTSmartPointer<RE::SkyrimVM::UpdateDataEvent>(new RE::SkyrimVM::UpdateDataEvent());
		if (!updateEvent) {
			spdlog::error("_ts_SKSEFunctions - {}: updateEvent is None", __func__);
			return;
		}

		updateEvent->updateType = RE::SkyrimVM::UpdateDataEvent::UpdateType::kNoRepeat;  // Single update
		updateEvent->timeToSendEvent = skyrimVM->currentVMMenuModeTime + static_cast<std::uint32_t>(delayInSeconds * 1000);  // Delay in milliseconds
		updateEvent->updateTime = static_cast<std::uint32_t>(delayInSeconds * 1000);  // Delay in milliseconds
		updateEvent->handle = handle;

		// Queue the event
		{
			RE::BSSpinLockGuard lock(skyrimVM->queuedOnUpdateEventLock);
			skyrimVM->queuedOnUpdateEvents.push_back(std::move(updateEvent));
		}
	}
	
/******************************************************************************************/

	void SetAngle(RE::TESObjectREFR* a_ref, RE::NiPoint3 a_angle) {
		if (!a_ref) {
			spdlog::error("_ts_SKSEFunctions - {}: a_ref is None", __func__);
			return;
		}
		a_ref->data.angle = a_angle;
		a_ref->Update3DPosition(true);
	}

	void SetAngleX(RE::TESObjectREFR* a_ref, float a_angleX) {
		if (!a_ref) {
			spdlog::error("_ts_SKSEFunctions - {}: a_ref is None", __func__);
			return;
		}
		auto Angle = a_ref->GetAngle();
		Angle.x = a_angleX;
		a_ref->data.angle = Angle;
		a_ref->Update3DPosition(true);
	}

	void SetAngleY(RE::TESObjectREFR* a_ref, float a_angleY) {
		if (!a_ref) {
			spdlog::error("_ts_SKSEFunctions - {}: a_ref is None", __func__);
			return;
		}
		auto Angle = a_ref->GetAngle();
		Angle.y = a_angleY;
		a_ref->data.angle = Angle;
		a_ref->Update3DPosition(true);
	}

	void SetAngleZ(RE::TESObjectREFR* a_ref, float a_angleZ) {
		if (!a_ref) {
			spdlog::error("_ts_SKSEFunctions - {}: a_ref is None", __func__);
			return;
		}
		auto Angle = a_ref->GetAngle();
		Angle.z = a_angleZ;
		a_ref->data.angle = Angle;
		a_ref->Update3DPosition(true);
	}

/******************************************************************************************/
	
	void MoveTo(RE::TESObjectREFR* object, const RE::TESObjectREFR* target, float fOffsetX, float fOffsetY, float fOffsetZ) {
		if (!object) {
			spdlog::error("_ts_SKSEFunctions - {}: object is None", __func__);
			return;
		}
		if (!target) {
			spdlog::error("_ts_SKSEFunctions - {}: target is None", __func__);
			return;
		}
		RE::NiPoint3 pos = target->GetPosition();
		object->SetPosition(pos.x + fOffsetX , pos.y + fOffsetY, pos.z + fOffsetZ);
	}

/******************************************************************************************/

	float GetDistance(RE::TESObjectREFR* a_ref1, RE::TESObjectREFR* a_ref2) {
		if (!a_ref1 || !a_ref2) {
			spdlog::error("_ts_SKSEFunctions - {}: a_ref1 or a_ref2 is None", __func__);
			return -1.0f;
		}
		return a_ref1->GetPosition().GetDistance(a_ref2->GetPosition());
	}

/******************************************************************************************/

	void SetLookAt(RE::Actor* actor, RE::TESObjectREFR* target, bool pathingLookAt) {
		if (!actor || !target) {
			spdlog::error("_ts_SKSEFunctions - {}: actor or target is None", __func__);
			return;
		}

		auto* high = actor->GetActorRuntimeData().currentProcess->high;
		if (high) {
			high->SetHeadtrackTarget(RE::HighProcessData::HEAD_TRACK_TYPE::kScript, target);

			if (pathingLookAt) {
				high->pathLookAtTarget = target->GetHandle();
			} else {
				high->pathLookAtTarget.reset();
			}
		}
	}

	void ClearLookAt(RE::Actor* actor) {
		if (!actor) {
			spdlog::error("_ts_SKSEFunctions - {}: actor is None", __func__);
			return;
		}
		auto* high = actor->GetActorRuntimeData().currentProcess->high;
		if (high) {
			high->ClearHeadtrackTarget(RE::HighProcessData::HEAD_TRACK_TYPE::kScript, true);
		}
	}

/******************************************************************************************/

	void SendCustomEvent(RE::VMHandle handle, std::string eventName, RE::BSScript::IFunctionArguments * args) {
        auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
        if (vm) {
			if (handle) {
				vm->SendEvent(handle, eventName.c_str(), args);
			} else {
				spdlog::error("_ts_SKSEFunctions - {}: invalid handle (event {})", __func__, eventName);
			}

        } else {
			spdlog::error("_ts_SKSEFunctions - {}: could not send event {}", __func__, eventName);
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
	
	RE::TESCondition* condition_GetFlyingState;
	int GetFlyingState(RE::Actor* akActor) {
		if (!IsFormValid(akActor)) {
			spdlog::warn("_ts_SKSEFunctions - {}: error, akActor doesn't exist", __func__);
			return -1;
		}

		if (!condition_GetFlyingState) {
			spdlog::info("_ts_SKSEFunctions - {}: creating GetFlyingState condition", __func__);
			auto* conditionItem = new RE::TESConditionItem;
			conditionItem->data.functionData.function = RE::FUNCTION_DATA::FunctionID::kGetFlyingState;

			condition_GetFlyingState = new RE::TESCondition;
			condition_GetFlyingState->head = conditionItem;
		}

		for (int i = 0; i < 6; i++) {
			condition_GetFlyingState->head->data.comparisonValue.f = i;
			if (condition_GetFlyingState->IsTrue(akActor, nullptr)) {
				return i;
			}
		}
		return -1;
	}

/******************************************************************************************/

	RE::TESCondition* condition_GetIsFlying;
	bool IsFlying(RE::Actor* akActor) {
		if (!IsFormValid(akActor)) {
			spdlog::warn("_ts_SKSEFunctions - {}: error, akActor doesn't exist", __func__);
			return false;
		}

		if (!condition_GetIsFlying) {
			spdlog::info("_ts_SKSEFunctions - {}: creating GetIsFlying condition", __func__);
			auto* conditionItem = new RE::TESConditionItem;
			conditionItem->data.comparisonValue.f = 1.0f;
			conditionItem->data.functionData.function = RE::FUNCTION_DATA::FunctionID::kGetIsFlying;

			condition_GetIsFlying = new RE::TESCondition;
			condition_GetIsFlying->head = conditionItem;
		}

		return condition_GetIsFlying->IsTrue(akActor, nullptr);
	}

/******************************************************************************************/

	RE::TESCondition* condition_HasLOS;
	bool HasLOS(RE::Actor* akActor, RE::TESObjectREFR* target) {
		if (!IsFormValid(akActor)) {
			spdlog::warn("_ts_SKSEFunctions - {}: error, akActor doesn't exist", __func__);
			return false;
		}

		if (!IsFormValid(target)) {
			spdlog::warn("_ts_SKSEFunctions - {}: error, target doesn't exist", __func__);
			return false;
		}

		if (!condition_HasLOS) {
			spdlog::info("_ts_SKSEFunctions - {}: creating HasLOS condition", __func__);
			auto* conditionItem = new RE::TESConditionItem;
			conditionItem->data.comparisonValue.f = 1.0f;
			conditionItem->data.functionData.function = RE::FUNCTION_DATA::FunctionID::kGetLineOfSight;

			condition_HasLOS = new RE::TESCondition;
			condition_HasLOS->head = conditionItem;
		}

		condition_HasLOS->head->data.functionData.params[0] = target;
		return condition_HasLOS->IsTrue(akActor, nullptr);
	}

/******************************************************************************************/

	RE::TESCondition* condition_GetCombatState;
	int GetCombatState(RE::Actor* akActor) {
		if (!IsFormValid(akActor)) {
			spdlog::warn("_ts_SKSEFunctions - {}: error, akActor doesn't exist", __func__);
			return -1;
		}

		if (!condition_GetCombatState) {
			spdlog::info("_ts_SKSEFunctions - {}: creating GetCombatState condition", __func__);
			auto* conditionItem = new RE::TESConditionItem;
			conditionItem->data.functionData.function = RE::FUNCTION_DATA::FunctionID::kGetCombatState;

			condition_GetCombatState = new RE::TESCondition;
			condition_GetCombatState->head = conditionItem;
		}

		for (int i = 0; i < 3; i++) {
				condition_GetCombatState->head->data.comparisonValue.f = i;
			if (condition_GetCombatState->IsTrue(akActor, nullptr)) {
				return i;
			}
		}
		return -1;
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

	float GetHealthPercentage(RE::Actor* actor) {
		if (!actor) {
			spdlog::error("_ts_SKSEFunctions - {}: actor is None", __func__);
			return -1.0f;
		}

		float currenthealth = actor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kHealth);
        float maxHealth = actor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kHealth);
        return currenthealth / maxHealth;
	}	

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

/******************************************************************************************/

	RE::Actor* GetCombatTarget(RE::Actor* actor) {
		if (!actor) {
			return nullptr;
		}

		auto targetHandle = actor->GetActorRuntimeData().currentCombatTarget;
        RE::Actor* target = nullptr;
        if (targetHandle) {
			auto targetPtr = targetHandle.get();
			if (targetPtr) {
				target = targetPtr.get();
			}
		}
		return target;
	}

/******************************************************************************************/

	void StartCombat(RE::Actor* a_actor, RE::Actor* a_target) {

		// TODO: Not yet working as expected
		spdlog::error("_ts_SKSEFunctions - {}: Error - Not yet functional!", __func__);
		return;

/*

		if (!a_actor || !a_target) {
			spdlog::error("_ts_SKSEFunctions - {}: actor or target is None", __func__);
			return;
		}

		auto* combatGroup = a_actor->GetCombatGroup();

		if (!combatGroup) {
			log::info("StartCombat - creating new combatGroup");
			combatGroup = new RE::CombatGroup();

			auto member = new RE::CombatMember;
			member->memberHandle = a_actor->GetHandle();
			combatGroup->members.push_back(*member);

			auto target = new RE::CombatTarget;
			target->targetHandle = a_target->GetHandle();
			combatGroup->targets.push_back(*target);

			a_actor->SetCombatGroup(combatGroup);
			a_actor->GetActorRuntimeData().currentCombatTarget = a_target->GetHandle();
			a_actor->UpdateCombat();
		} else {
			log::info("StartCombat - combatGroup exists");
		}

		log::info("StartCombat - IsInCombat: {}", a_actor->IsInCombat()); */
	}
}


