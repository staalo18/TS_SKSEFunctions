#include "SKSE/logger.h"
#include "_ts_SKSEFunctions.h"

namespace _ts_SKSEFunctions {

	void InitializeLogging(spdlog::level::level_enum a_loglevel) {
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
		log->set_level({ a_loglevel });
		log->flush_on({ spdlog::level::level_enum::trace });

		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [%t] [%s:%#] %v");
	}

/******************************************************************************************/

	// Function to pause a while loop if the game is in menu mode, console is open, or out of focus
	void WaitWhileGameIsPaused(int a_checkInterval_ms) {

		auto* ui = RE::UI::GetSingleton();
		auto* main = RE::Main::GetSingleton();

		while (ui && (ui->GameIsPaused() || ui->IsMenuOpen(RE::Console::MENU_NAME)) ||
		(main && !main->gameActive)) {
			std::this_thread::sleep_for(std::chrono::milliseconds(a_checkInterval_ms));
		}
	}

/******************************************************************************************/

	RE::VMHandle GetHandle(const RE::TESForm* a_akForm) {
        if (!(a_akForm)) {
            log::warn("{}: a_akForm doesn't exist or isn't valid", __func__);
            return NULL;
        }

        RE::VMTypeID id = static_cast<RE::VMTypeID>(a_akForm->GetFormType());
        RE::VMHandle handle = RE::SkyrimVM::GetSingleton()->handlePolicy.GetHandleForObject(id, a_akForm);

        if (handle == NULL) {
            return NULL;
        }

        return handle;
    }

/******************************************************************************************/

	bool IsFormValid(RE::TESForm* a_form, bool a_checkDeleted) {
		// Copy from DBSkseFunctions - author: Dylbill
		if (!a_form) {
			return false;
		}
	
		if (IsBadReadPtr(a_form, sizeof(a_form))) {
			return false;
		}
	
		if (a_form->GetFormID() == 0) {
			return false;
		}
	
		if (a_checkDeleted) {
			if (a_form->IsDeleted()) {
				return false;
			}
		}
	
		return true;
	}

/******************************************************************************************/

	void RegisterForSingleUpdate(RE::VMHandle a_handle, float a_delayInSeconds)
	{
		if (a_delayInSeconds < 0.0f) {
			spdlog::error("_ts_SKSEFunctions - {}: a_delayInSeconds is negative", __func__);
			return;
		}
		if (!a_handle) {
			spdlog::error("_ts_SKSEFunctions - {}: a_handle is None", __func__);
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
		updateEvent->timeToSendEvent = skyrimVM->currentVMMenuModeTime + static_cast<std::uint32_t>(a_delayInSeconds * 1000);  // Delay in milliseconds
		updateEvent->updateTime = static_cast<std::uint32_t>(a_delayInSeconds * 1000);  // Delay in milliseconds
		updateEvent->handle = a_handle;

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
	
	void MoveTo(RE::TESObjectREFR* a_object, RE::TESObjectREFR* a_target, float a_fOffsetX, float a_fOffsetY, float a_fOffsetZ) {
		if (!a_object) {
			spdlog::error("_ts_SKSEFunctions - {}: a_object is None", __func__);
			return;
		}
		if (!a_target) {
			spdlog::error("_ts_SKSEFunctions - {}: a_target is None", __func__);
			return;
		}
		a_object->MoveTo(a_target); //using MoveTo ensures that object is moved across Worldspaces if needed.
		
		// now process offset
		RE::NiPoint3 pos = a_target->GetPosition();
		a_object->SetPosition(pos.x + a_fOffsetX , pos.y + a_fOffsetY, pos.z + a_fOffsetZ);
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

	void SetLookAt(RE::Actor* a_actor, RE::TESObjectREFR* a_target, bool a_pathingLookAt) {
		if (!a_actor || !a_target) {
			spdlog::error("_ts_SKSEFunctions - {}: a_actor or a_target is None", __func__);
			return;
		}

		auto* high = a_actor->GetActorRuntimeData().currentProcess->high;
		if (high) {
			high->SetHeadtrackTarget(RE::HighProcessData::HEAD_TRACK_TYPE::kScript, a_target);

			if (a_pathingLookAt) {
				high->pathLookAtTarget = a_target->GetHandle();
			} else {
				high->pathLookAtTarget.reset();
			}
		}
	}

	void ClearLookAt(RE::Actor* a_actor) {
		if (!a_actor) {
			spdlog::error("_ts_SKSEFunctions - {}: a_actor is None", __func__);
			return;
		}
		auto* high = a_actor->GetActorRuntimeData().currentProcess->high;
		if (high) {
			high->ClearHeadtrackTarget(RE::HighProcessData::HEAD_TRACK_TYPE::kScript, true);
		}
	}

/******************************************************************************************/

	void SendCustomEvent(RE::VMHandle a_handle, std::string a_eventName, RE::BSScript::IFunctionArguments * a_args) {
        auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
        if (vm) {
			if (a_handle) {
				vm->SendEvent(a_handle, a_eventName.c_str(), a_args);
			} else {
				spdlog::error("_ts_SKSEFunctions - {}: invalid handle (event {})", __func__, a_eventName);
			}

        } else {
			spdlog::error("_ts_SKSEFunctions - {}: could not send event {}", __func__, a_eventName);
		}
    }

/******************************************************************************************/

	bool CheckForPackage(RE::Actor* a_akActor, const RE::BGSListForm* a_Packagelist, RE::TESPackage* a_CheckPackage) {
		if (!a_akActor) {
			spdlog::error("_ts_SKSEFunctions - {}: error, a_akActor doesn't exist", __func__);
			return false;
		}

		if (!a_Packagelist) {
			return false;
		}

		if (!a_CheckPackage) {
			a_CheckPackage = a_akActor->GetCurrentPackage();
		}

		for (auto& form : a_Packagelist->forms) {
			if (form && form->As<RE::TESPackage>() == a_CheckPackage) {
				return true;
			}
		}

		return false;
	}

/******************************************************************************************/

	RE::TESCondition* condition_IsPlayerInRegion;
	bool IsPlayerInRegion(const std::string& a_regionName) {
	
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
	
		auto* regionForm =  RE::TESForm::LookupByEditorID(a_regionName);
		if (!regionForm) {
			spdlog::error("_ts_SKSEFunctions - {}: Failed to lookup region form {}", __func__, a_regionName);
			return false;
		}
		condition_IsPlayerInRegion->head->data.functionData.params[0] = regionForm;
	
		return condition_IsPlayerInRegion->IsTrue(player, nullptr);
	}
	
/******************************************************************************************/
	
	RE::TESCondition* condition_GetFlyingState;
	int GetFlyingState(RE::Actor* a_akActor) {
		if (!IsFormValid(a_akActor)) {
			spdlog::warn("_ts_SKSEFunctions - {}: error, a_akActor doesn't exist", __func__);
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
			if (condition_GetFlyingState->IsTrue(a_akActor, nullptr)) {
				return i;
			}
		}
		return -1;
	}

/******************************************************************************************/

	RE::TESCondition* condition_GetIsFlying;
	bool IsFlying(RE::Actor* a_akActor) {
		if (!IsFormValid(a_akActor)) {
			spdlog::warn("_ts_SKSEFunctions - {}: error, a_akActor doesn't exist", __func__);
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

		return condition_GetIsFlying->IsTrue(a_akActor, nullptr);
	}

/******************************************************************************************/

	RE::TESCondition* condition_HasLOS;
	bool HasLOS(RE::Actor* a_akActor, RE::TESObjectREFR* a_target) {
		if (!IsFormValid(a_akActor)) {
			spdlog::warn("_ts_SKSEFunctions - {}: error, a_akActor doesn't exist", __func__);
			return false;
		}

		if (!IsFormValid(a_target)) {
			spdlog::warn("_ts_SKSEFunctions - {}: error, a_target doesn't exist", __func__);
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

		condition_HasLOS->head->data.functionData.params[0] = a_target;
		return condition_HasLOS->IsTrue(a_akActor, nullptr);
	}

/******************************************************************************************/

	RE::TESCondition* condition_GetCombatState;
	int GetCombatState(RE::Actor* a_akActor) {
		if (!IsFormValid(a_akActor)) {
			spdlog::warn("_ts_SKSEFunctions - {}: error, a_akActor doesn't exist", __func__);
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
			if (condition_GetCombatState->IsTrue(a_akActor, nullptr)) {
				return i;
			}
		}
		return -1;
	}

/******************************************************************************************/
	
	RE::TESCondition* condition_IsFlyingMountPatrolQueued;
	bool IsFlyingMountPatrolQueued(RE::Actor* a_akActor) {
		if (!IsFormValid(a_akActor)) {
			spdlog::warn("_ts_SKSEFunctions - {}: error, a_akActor doesn't exist", __func__);
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

		return condition_IsFlyingMountPatrolQueued->IsTrue(a_akActor, nullptr);
	}

/******************************************************************************************/
	
	RE::TESCondition* condition_IsFlyingMountFastTravelling;
	bool IsFlyingMountFastTravelling(RE::Actor* a_akActor) {
		if (!IsFormValid(a_akActor)) {
			spdlog::warn("_ts_SKSEFunctions - {}: error, a_akActor doesn't exist", __func__);
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

		return condition_IsFlyingMountFastTravelling->IsTrue(a_akActor, nullptr);
	}

/******************************************************************************************/

	float GetHealthPercentage(RE::Actor* a_actor) {
		if (!a_actor) {
			spdlog::error("_ts_SKSEFunctions - {}: a_actor is None", __func__);
			return -1.0f;
		}

		float currenthealth = a_actor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kHealth);
        float maxHealth = a_actor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kHealth);
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
			spdlog::info("_ts_SKSEFunctions - {}: a_actor is None", __func__);
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

	RE::Actor* GetCombatTarget(RE::Actor* a_actor) {
		if (!a_actor) {
			return nullptr;
		}

		auto targetHandle = a_actor->GetActorRuntimeData().currentCombatTarget;
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

	void UpdateCombatTarget(RE::Actor* a_actor, RE::Actor* a_target) {
		if (!a_actor || !a_target) {
			spdlog::error("_ts_SKSEFunctions - {}: a_actor or a_target is None", __func__);
			return;
		}

		auto* combatGroup = a_actor->GetCombatGroup();

		if (combatGroup) {
			// Already in combat: switch target
			combatGroup->targets.clear();

			RE::CombatTarget newTarget;
			newTarget.targetHandle = a_target->GetHandle();
			combatGroup->targets.push_back(newTarget);

			a_actor->SetCombatGroup(combatGroup);
			a_actor->GetActorRuntimeData().currentCombatTarget = a_target->GetHandle();
			a_actor->UpdateCombat();
			if (a_actor->GetActorRuntimeData().currentCombatTarget) {
				spdlog::info("_ts_SKSEFunctions - {}: Switched combat target for actor {} to {}", __func__, a_actor->GetFormID(), a_actor->GetActorRuntimeData().currentCombatTarget.get()->GetFormID());
			} else {
				spdlog::warn("_ts_SKSEFunctions - {}: Failed to set combat target for actor {}", __func__, a_actor->GetFormID());
			}
		} else {
			spdlog::info("_ts_SKSEFunctions - {}: Actor {} is not in combat...", __func__, a_actor->GetFormID());
		}
/* Starting combat is not working as of now
		// Not in combat: create new combat group and set target
		auto* newCombatGroup = new RE::CombatGroup();
		RE::CombatMember newMember;
		newMember.memberHandle = a_actor->GetHandle();
		newCombatGroup->members.push_back(newMember);

		RE::CombatTarget newTarget;
		newTarget.targetHandle = a_target->GetHandle();
		newCombatGroup->targets.push_back(newTarget);

		a_actor->SetCombatGroup(newCombatGroup);
		a_actor->GetActorRuntimeData().currentCombatTarget = a_target->GetHandle();
		a_actor->UpdateCombat();
		spdlog::info("_ts_SKSEFunctions - {}: Started combat for actor {} with target {}", __func__, a_actor->GetFormID(), a_target->GetFormID());
*/		
	}

/******************************************************************************************/

    float GetAngleBetweenVectors(const RE::NiPoint3& a, const RE::NiPoint3& b) {
        float dotProduct = a * b;
        float magnitudeA = a.Length();
        float magnitudeB = b.Length();
        if (magnitudeA == 0 || magnitudeB == 0) {
            return 0.0f; // Avoid division by zero
        }
        float cosTheta = dotProduct / (magnitudeA * magnitudeB);
        // Clamp the value to the valid range for acos to avoid NaN due to floating point inaccuracies
        cosTheta = std::clamp(cosTheta, -1.0f, 1.0f);
        return 180.0f * std::acos(cosTheta) / 3.14159265f;
    }

/******************************************************************************************/

	float GetAngleZ(const RE::NiPoint3& a_from, const RE::NiPoint3& a_to)
	{
		const auto x = a_to.x - a_from.x;
		const auto y = a_to.y - a_from.y;

		return atan2(x, y);
	}

/******************************************************************************************/

	float GetCameraYaw() {
		// Only reliable if camera is not in firstperson state!
		// Alternative is to use GetCameraRotation() function
		auto playerCamera = RE::PlayerCamera::GetSingleton();
		if (playerCamera && playerCamera->cameraRoot) {
			RE::NiNode* root = playerCamera->cameraRoot.get();
			if (root) {
				auto forwardVector = root->world.rotate.GetVectorZ();
				float cameraYaw = NormalRelativeAngle(std::atan2(forwardVector.x, forwardVector.y));

				float cameraPlayerDirection = GetAngleZ(GetCameraPos(), RE::PlayerCharacter::GetSingleton()->GetPosition());

				if (fabs(NormalRelativeAngle(cameraPlayerDirection - cameraYaw)) > 0.9*PI) { // DOES NOT WORK for firstperson state!s
					// in case the camera representation is flipped compared to Skyrim convention, align it with player direction
					// look up "quaternion double cover" to learn more about this
					cameraYaw = NormalRelativeAngle(cameraYaw + PI);
				}

				return cameraYaw;
			}
		}
		return 0.0f;
	}

/******************************************************************************************/

	float GetCameraPitch() {
		auto playerCamera = RE::PlayerCamera::GetSingleton();
		if (playerCamera && playerCamera->cameraRoot) {
			RE::NiNode* root = playerCamera->cameraRoot.get();
			if (root) {
				auto forwardVector = root->world.rotate.GetVectorZ();
				auto upVector = root->world.rotate.GetVectorY();

				float cameraPitch = NormalRelativeAngle( 0.5f*PI - std::atan2(forwardVector.z, 
					std::sqrt(forwardVector.x * forwardVector.x + forwardVector.y * forwardVector.y)));

				if (upVector.z < 0) {
					// Camera is upside down due to quaternion flip
					cameraPitch = -cameraPitch;
				}

				return cameraPitch;
			}
		}
		return 0.0f;
	}

/******************************************************************************************/
	
	RE::NiPoint3 GetCameraRotation() {
		RE::NiPoint3 rotation;
		rotation.x = 0.0f; // Pitch
		rotation.y = 0.0f; // Roll is not computed
		rotation.z = 0.0f; // Yaw

		auto* playerCamera = RE::PlayerCamera::GetSingleton();

		if (playerCamera && playerCamera->currentState) {
			if (playerCamera->currentState->id == RE::CameraState::kFree) {
				RE::FreeCameraState* cameraState = static_cast<RE::FreeCameraState*>(playerCamera->currentState.get());
				
				if (cameraState) {
					rotation.x = cameraState->rotation.x;
					rotation.z = cameraState->rotation.y;
				}
			}  
			else if (playerCamera->currentState->id == RE::CameraState::kThirdPerson ||
					playerCamera->currentState->id == RE::CameraState::kMount ||
					playerCamera->currentState->id == RE::CameraState::kDragon) {
				RE::ThirdPersonState* cameraState = static_cast<RE::ThirdPersonState*>(playerCamera->currentState.get());
				
				if (cameraState) {
					rotation.x = -GetPitch(cameraState->rotation);
					rotation.z = GetYaw(cameraState->rotation);
				}
			} else if (playerCamera->currentState->id == RE::CameraState::kFirstPerson) {
				RE::FirstPersonState* cameraState = static_cast<RE::FirstPersonState*>(playerCamera->currentState.get());
				
				if (cameraState) {
					RE::PlayerCharacter* player = RE::PlayerCharacter::GetSingleton();
					RE::NiPoint3 angle;
					cameraState->firstPersonCameraObj->world.rotate.ToEulerAnglesXYZ(angle);

					rotation.x = player->data.angle.x - angle.x;
					rotation.z = player->data.angle.z;
				}
			}
		}

		return rotation;
	}

/******************************************************************************************/
// Below functions are from 'True Directional Movement':
// https://github.com/ersh1/TrueDirectionalMovement
// All credits go to the original author Ersh!
/******************************************************************************************/

	RE::NiPoint3 GetCameraPos()
	{
		auto player = RE::PlayerCharacter::GetSingleton();
		auto playerCamera = RE::PlayerCamera::GetSingleton();
		RE::NiPoint3 ret;

		if (!player || !playerCamera || !playerCamera->currentState) {
			return ret;
		}

		if (playerCamera->currentState->id == RE::CameraState::kFirstPerson ||
			playerCamera->currentState->id == RE::CameraState::kThirdPerson ||
			playerCamera->currentState->id == RE::CameraState::kMount ||
			playerCamera->currentState->id == RE::CameraState::kDragon)
		{
			RE::NiNode* root = playerCamera->cameraRoot.get();
			if (root) {
				ret = root->world.translate;
			}
		} else if(playerCamera->currentState->id == RE::CameraState::kFree) {
			RE::FreeCameraState* cameraState = static_cast<RE::FreeCameraState*>(playerCamera->currentState.get());			
			if (cameraState) {
				ret = cameraState->translation;
			}
		} else {
			RE::NiPoint3 playerPos = player->GetLookingAtLocation();

			ret.z = playerPos.z;
			ret.x = player->GetPositionX();
			ret.y = player->GetPositionY();
		}

		return ret;
	}

/******************************************************************************************/

	float NormalAbsoluteAngle(float a_angle)
	{
		while (a_angle < 0)
			a_angle += 2.f*PI;
		while (a_angle > 2.f*PI)
			a_angle -= 2.f*PI;
		return a_angle;
	}

/******************************************************************************************/

	float NormalRelativeAngle(float a_angle)
	{
		while (a_angle > PI)
			a_angle -= 2.f*PI;
		while (a_angle < -PI)
			a_angle += 2.f*PI;
		return a_angle;
	}


// End True Directional Movement functions
/******************************************************************************************/

    float ApplyEasing(float t, bool easeIn, bool easeOut) {
        if (easeIn && easeOut) {
            // Smoothstep (both ease in and out)
            return t * t * (3.0f - 2.0f * t);
        } else if (easeIn) {
            // Ease in only (accelerate)
            return t * t;
        } else if (easeOut) {
            // Ease out only (decelerate)
            return 1.0f - (1.0f - t) * (1.0f - t);
        }
        // No easing
        return t;
    }

    float SCurveFromLinear(float x, float x1, float x2) {
		// This function modifies a linear input x in [0,1] into a smooth s-shape response in [0,1]
		// with quadratic ease-in from 0 to x1, linear section from x1 to x2, and quadratic ease-out from x2 to 1
		// x1 and x2 must be in [0,1] and x2 >= x1
		// if x1 == x2, the function reduces to a standard smoothstep function

		x1 = std::clamp(x1, 0.0f, 1.0f);
        x2 = std::clamp(x2, 0.0f, 1.0f);
        x = std::clamp(x, 0.0f, 1.0f);
        
        if (x2 < x1) {
            x2 = x1;
        }
        
        if (x1 == x2) {
	        // Handle edge case where x1 == x2 (no linear section)
            // Simple smoothstep: 3x^2 - 2x^3
            return 3.0f * x * x - 2.0f * x * x * x;
        }
        
        // Calculate the slope of the linear section
        // We need to ensure f(1) = 1 and smooth connections at x1 and x2
        float m = 1.0f / (x2 - x1 + 0.5f * x1 + 0.5f * (1.0f - x2));
        
        if (x <= x1) {
            // Phase 1: Quadratic ease-in (0 to x1)
            // f(x) = a*x^2
            // f'(x1) = m -> 2*a*x1 = m -> a = m/(2*x1)
            if (x1 == 0.0f) return 0.0f;
            
            float a = m / (2.0f * x1);
            return a * x * x;
            
        } else if (x <= x2) {
            // Phase 2: Linear section (x1 to x2)
            // f(x) = m*x + c
            // Must pass through (x1, f(x1))
            float f_x1 = (m / (2.0f * x1)) * x1 * x1;
            float c = f_x1 - m * x1;
            return m * x + c;
            
        } else {
            // Phase 3: Quadratic ease-out (x2 to 1)
            // Use form: f(x) = 1 - a*(1-x)^2
            // This ensures f(1) = 1
            // f'(x) = 2*a*(1-x)
            // f'(x2) = m -> 2*a*(1-x2) = m -> a = m/(2*(1-x2))
            
            if (x2 == 1.0f) return 1.0f;
            
            float a = m / (2.0f * (1.0f - x2));
            return 1.0f - a * (1.0f - x) * (1.0f - x);
        }
    }

/******************************************************************************************/

    RE::Actor* FindClosestActorInCameraDirection(
            float a_angleTolerance, 
            float a_maxDistance,
			bool a_excludeAllies,
            const std::vector<RE::Actor*>& excludeActors) {

        auto* playerActor = RE::PlayerCharacter::GetSingleton();
        auto* processLists = RE::ProcessLists::GetSingleton();
        auto* playerCamera = RE::PlayerCamera::GetSingleton();

        if (!playerActor) {
            spdlog::error("_ts_SKSEFunctions - {}: PlayerActor is null", __func__);
            return nullptr;
        }
        if (!processLists) {
            spdlog::error("_ts_SKSEFunctions - {}: ProcessLists is null", __func__);
            return nullptr;
        }
        if (!playerCamera) {
            spdlog::error("_ts_SKSEFunctions - {}: PlayerCamera is null", __func__);
            return nullptr;
        }

        auto cameraPos = playerCamera->cameraRoot->world.translate;
        auto playerPos = playerActor->GetPosition();

        auto root = playerCamera->cameraRoot;
        const auto& worldTransform = root->world; // RE::NiTransform

        // The forward vector is the third column of the rotation matrix
        RE::NiPoint3 cameraForward = worldTransform.rotate * RE::NiPoint3{ 0.0f, 1.0f, 0.0f };

        RE::Actor* selectedActor = nullptr;
        for (auto handle : processLists->highActorHandles) {
            auto actor = handle.get().get();
            if (actor
                && std::find(excludeActors.begin(), excludeActors.end(), actor) == excludeActors.end() 
                && actor->Get3D() 
                && !actor->IsDead()
				&& !(a_excludeAllies && actor->GetFactionReaction(playerActor) == RE::FIGHT_REACTION::kAlly)
                && (a_maxDistance <= 0.0f || actor->GetPosition().GetDistance(playerPos) <= a_maxDistance)) {

                float fAngle = GetAngleBetweenVectors(actor->GetPosition() - cameraPos, cameraForward);

                if (fAngle <= a_angleTolerance) {
                    if (selectedActor) {
spdlog::info("_ts_SKSEFunctions - {}: Checking actor: {}, angle = {}, distance = {}", __func__, actor->GetName(), fAngle, actor->GetPosition().GetDistance(playerPos));
                        if (actor->GetPosition().GetDistance(playerPos) < selectedActor->GetPosition().GetDistance(playerPos)) {
spdlog::info("_ts_SKSEFunctions - {}: New selected actor: {}, angle = {}, distance = {}", __func__, actor->GetName(), fAngle, actor->GetPosition().GetDistance(playerPos));
                            selectedActor = actor;
                        }
                    } else {
spdlog::info("_ts_SKSEFunctions - {}: First selected actor {}: angle = {}, distance = {}", __func__, actor->GetName(), fAngle, actor->GetPosition().GetDistance(playerPos));
                        selectedActor = actor;
                    }
                }
            }
        }

        return selectedActor;
    }
}