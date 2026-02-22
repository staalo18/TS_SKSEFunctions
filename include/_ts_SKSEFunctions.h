#pragma once

#include <string>
#include <filesystem>
#include <SimpleIni.h>
#include <thread>
#include <future>

#define PI 3.1415926535f

namespace _ts_SKSEFunctions {

    void InitializeLogging(spdlog::level::level_enum a_loglevel = spdlog::level::level_enum::info);

	void WaitWhileGameIsPaused(int a_checkInterval_ms = 100);

	RE::VMHandle GetHandle(const RE::TESForm* a_akForm);

    bool IsFormValid(RE::TESForm* a_form, bool a_checkDeleted = true);

	void RegisterForSingleUpdate(RE::VMHandle a_handle, float a_delayInSeconds);

	void SetAngle(RE::TESObjectREFR* a_ref, RE::NiPoint3 a_angle);

	void SetAngleX(RE::TESObjectREFR* a_ref, float a_angleX);

	void SetAngleY(RE::TESObjectREFR* a_ref, float a_angleY);

	void SetAngleZ(RE::TESObjectREFR* a_ref, float a_angleZ);

	void MoveTo(RE::TESObjectREFR* a_object, RE::TESObjectREFR* a_target, 
				float a_fOffsetX = 0.0f, float a_fOffsetY = 0.0f, float a_fOffsetZ = 0.0f);

	float GetDistance(RE::TESObjectREFR* a_ref1, RE::TESObjectREFR* a_ref2);

	void SetLookAt(RE::Actor* a_actor, RE::TESObjectREFR* a_target, bool a_pathingLookAt = false);

	void ClearLookAt(RE::Actor* a_actor);

	void SendCustomEvent(RE::VMHandle a_handle, std::string a_eventName, RE::BSScript::IFunctionArguments * a_args);

	bool CheckForPackage(RE::Actor* a_akActor, const RE::BGSListForm* a_Packagelist, RE::TESPackage* a_CheckPackage = nullptr);

    bool IsPlayerInRegion(const std::string& a_regionName);

	int GetFlyingState(RE::Actor* a_akActor);

	bool IsFlying(RE::Actor* a_akActor);

	bool HasLOS(RE::Actor* a_akActor, RE::TESObjectREFR* a_target);

	int GetCombatState(RE::Actor* a_akActor);

    bool IsFlyingMountPatrolQueued(RE::Actor* a_akActor);

    bool IsFlyingMountFastTravelling(RE::Actor* a_akActor);

	float GetHealthPercentage(RE::Actor* a_actor);

    float GetLandHeight(float a_x, float a_y, float a_z);

    float GetLandHeightWithWater(RE::TESObjectREFR* a_ref);

	float GetLandHeightWithWater(RE::NiPoint3& a_pos);

    bool ClearCombatTargets(RE::Actor* a_actor);

	RE::Actor* GetCombatTarget(RE::Actor* a_actor);

	void UpdateCombatTarget(RE::Actor* a_actor, RE::Actor* a_target);

    std::vector<RE::Actor*> GetCombatMembers(const RE::Actor* a_actor);

	// returns the angle between two RE::NiPoint3 vectors in degrees
	float GetAngleBetweenVectors(const RE::NiPoint3& a, const RE::NiPoint3& b);

	float GetCameraYaw();

	float GetCameraPitch();
	
	float GetAngleZ(const RE::NiPoint3& a_from, const RE::NiPoint3& a_to);

	[[nodiscard]] inline float GetYaw(const RE::NiQuaternion a_rotation)
	{
		// will not produce reliable results near the gimbal lock (pitch approaching +/- PI/2, ie straight upwards or downwards pitch)
		return -std::atan2(2.0f * (a_rotation.w * a_rotation.z + a_rotation.x * a_rotation.y), 1.0f - 2.0f * (a_rotation.y * a_rotation.y + a_rotation.z * a_rotation.z));
	}

	[[nodiscard]] inline float GetPitch(const RE::NiQuaternion a_rotation)
	{
		return std::atan2(2.0f * (a_rotation.w * a_rotation.x + a_rotation.y * a_rotation.z), 1.0f - 2.0f * (a_rotation.x * a_rotation.x + a_rotation.y * a_rotation.y));
	}
    
	RE::NiPointer<RE::NiAVObject> GetTargetPoint(RE::Actor* a_actor, RE::BGSBodyPartDefs::LIMB_ENUM a_bodyPart);

	void GetBodyPartCoordinateFrame(RE::Actor* a_actor, RE::BGSBodyPartDefs::LIMB_ENUM a_bodyPart,
                                     RE::NiPoint3& a_forward, RE::NiPoint3& a_right, RE::NiPoint3& a_up);

    RE::NiPoint3 GetBodyPartRotation(RE::Actor* a_actor, RE::BGSBodyPartDefs::LIMB_ENUM a_bodyPart);

	RE::NiPoint3 GetCameraRotation();
	
	/******************************************************************************************/
	// Below functions are from 'True Directional Movement':
	// https://github.com/ersh1/TrueDirectionalMovement
	// All credits go to the original author Ersh!

	RE::NiPoint3 GetCameraPos();

	float NormalAbsoluteAngle(float a_angle);

	float NormalRelativeAngle(float a_angle);
	
	[[nodiscard]] inline float InterpEaseIn(const float& A, const float& B, float alpha, float exp)
	{
		float const modifiedAlpha = std::pow(alpha, exp);
		return std::lerp(A, B, modifiedAlpha);
	}

static float* g_deltaTimeRealTime = (float*)RELOCATION_ID(523661, 410200).address();                 // 2F6B94C, 30064CC

	[[nodiscard]] inline float GetRealTimeDeltaTime()
	{
		return *g_deltaTimeRealTime;
	}
	// End True Directional Movement functions
	/******************************************************************************************/

	float ApplyEasing(float t, bool easeIn, bool easeOut);

	float SCurveFromLinear(float x, float x1, float x2);

	// returns the closest living actor in the camera direction within a certain angle tolerance (in degrees) and distance
	// setting a_maxDistance < 0.0f will search for all actors that have their 3D loaded (ie maxDistance is ignored)
	// excludeActors is a list of actors to exclude from the search
	RE::Actor* FindClosestActorInCameraDirection(
		float a_angleTolerance = 360.0f, 
		float a_maxDistance = -1.0f,
		bool a_excludeAllies = true,
		const std::vector<RE::Actor*>& excludeActors = std::vector<RE::Actor*>());


	// returns the closest actor under the crosshair within a certain distance and scan angle
	// setting a_maxTargetDistance = 0.0f will search for all actors that have their 3D loaded (ie maxTargetDistance is ignored)
	// a_maxTargetScanAngle is the maximum angle (in degrees) from the center of the crosshair to scan for actors
	// a_excludeActors is a list of actors to exclude from the search
	// Note: this function also finds actors that are occluded from sight (eg behind walls)
	RE::Actor* GetCrosshairTarget(float a_maxTargetDistance = 0.0f, float a_maxTargetScanAngle = 7.0f, std::vector<RE::Actor*> a_excludeActors = {});

	// gets all target points from the actor's 3D
	std::vector<RE::NiPointer<RE::NiAVObject>> GetAllTargetPoints(RE::Actor* a_actor);

	// call a global papyrus function from C++
    template <class ... Args>
	bool CallPapyrusFunction(std::string_view a_functionClass, std::string_view a_function, Args... a_args) {
		// example usage:
		// _ts_SKSEFunctions::CallPapyrusFunction("Game"sv, "FastTravel"sv, FastTravelTarget);
		const auto skyrimVM = RE::SkyrimVM::GetSingleton();
		auto vm = skyrimVM ? skyrimVM->impl : nullptr;
		if (vm) {
			RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
			auto args = RE::MakeFunctionArguments(std::forward<Args>(a_args)...);
			return vm->DispatchStaticCall(std::string(a_functionClass).c_str(), std::string(a_function).c_str(), args, callback);
		}
		spdlog::error("_ts_SKSEFunctions - {}: could not call function {}.{}", __func__, a_functionClass, a_function);
		return false;
    }

	using ObjectPtr = RE::BSTSmartPointer<RE::BSScript::Object>;

	inline ObjectPtr GetObjectPtr(RE::TESForm* a_form, const char* a_class, bool a_create) {
		auto vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
		auto handle = GetHandle(a_form);

		ObjectPtr object = nullptr;
		bool found = vm->FindBoundObject(handle, a_class, object);
		if (!found && a_create) {
			vm->CreateObject2(a_class, object);
			vm->BindObject(object, handle, false);
		}

		return object;
	}	

	// Call a papyrus function from a script that extends a form (actor, quest etc) from C++
	template <class ... Args>
	bool CallPapyrusFunctionOn(RE::TESForm* a_form, std::string_view a_formKind, std::string_view a_function, Args... a_args) {
		// example usage:
		// RE::TESQuest* RideQuest = ...;
		// RE::TESActor* Player = ...;
		// _ts_SKSEFunctions::CallPapyrusFunctionOn(RideQuest, "Quest", "MyPapyrusFunction", <papyrus function arg list>)
		// _ts_SKSEFunctions::CallPapyrusFunctionOn(Player, "actor", "AnotherPapyrusFunction", <papyrus function arg list>)
		const auto skyrimVM = RE::SkyrimVM::GetSingleton();
		auto vm = skyrimVM ? skyrimVM->impl : nullptr;
		if (vm) {
			RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
			auto args = RE::MakeFunctionArguments(std::forward<Args>(a_args)...);
			auto objectPtr = GetObjectPtr(a_form, std::string(a_formKind).c_str(), false);
			if (!objectPtr) {
				spdlog::error("_ts_SKSEFunctions - {}: Could not bind form", __func__);
				return false;
			}
			bool bDispatch = vm->DispatchMethodCall1(objectPtr, std::string(a_function).c_str(), args, callback);
			if (!bDispatch) {
				spdlog::error("_ts_SKSEFunctions - {}: Could not dispatch method call", __func__);
			}
			return bDispatch;
		}
		return false;
	}


	template <typename T>
	bool UpdateIniSetting(const std::string& a_settingName, T a_value) {
		auto* settingCollection = RE::INISettingCollection::GetSingleton();
		RE::Setting* setting = settingCollection->GetSetting(a_settingName.c_str());
		if (!setting) {
			spdlog::error("_ts_SKSEFunctions - {}: Failed to get INI variable: {}", __func__, a_settingName);
			return false;
		}

		switch (setting->GetType()) {
			case RE::Setting::Type::kBool:
				if constexpr (std::is_same_v<T, bool>) {
					setting->data.b = a_value;
				} else {
					spdlog::error("_ts_SKSEFunctions - {}: Type mismatch for INI variable: {}", __func__, a_settingName);
					return false;
				}
				break;
			case RE::Setting::Type::kFloat:
				if constexpr (std::is_same_v<T, float>) {
					setting->data.f = a_value;
				} else {
					spdlog::error("_ts_SKSEFunctions - {}: Type mismatch for INI variable: {}", __func__, a_settingName);
					return false;
				}
				break;
			case RE::Setting::Type::kSignedInteger:
				if constexpr (std::is_same_v<T, std::int32_t>) {
					setting->data.i = a_value;
				} else {
					spdlog::error("_ts_SKSEFunctions - {}: Type mismatch for INI variable: {}", __func__, a_settingName);
					return false;
				}
				break;
			case RE::Setting::Type::kColor:
				if constexpr (std::is_same_v<T, RE::Color>) {
					setting->data.r = a_value;
				} else {
					spdlog::error("_ts_SKSEFunctions - {}: Type mismatch for INI variable: {}", __func__, a_settingName);
					return false;
				}
				break;
			case RE::Setting::Type::kString:
				if constexpr (std::is_same_v<T, const char*>) {
					setting->data.s = const_cast<char*>(a_value);
				} else {
					spdlog::error("_ts_SKSEFunctions - {}: Type mismatch for INI variable: {}", __func__, a_settingName);
					return false;
				}
				break;
			case RE::Setting::Type::kUnsignedInteger:
				if constexpr (std::is_same_v<T, std::uint32_t>) {
					setting->data.u = a_value;
				} else {
					spdlog::error("_ts_SKSEFunctions - {}: Type mismatch for INI variable: {}", __func__, a_settingName);
					return false;
				}
				break;
			default:
			spdlog::error("_ts_SKSEFunctions - {}: Unknown type for INI variable: {}", __func__, a_settingName);
				return false;
		}
	return true;
	}

	template <typename T>
	T GetValueFromINI(RE::BSScript::Internal::VirtualMachine* a_vm, RE::VMStackID a_stackId, 
									const std::string& a_iniKey, const std::string& a_iniFilename, T a_defaultValue) {

		std::filesystem::path iniPath = std::filesystem::current_path() / "Data" /  a_iniFilename;
		
		if (!std::filesystem::is_regular_file(iniPath)) {
			if (a_vm) {
				a_vm->TraceStack(("_ts_SKSEFunctions - GetValueFromINI: No such file: " +iniPath.string()).c_str(), a_stackId);
			}
			return a_defaultValue;
		}

		size_t separatorPos = a_iniKey.find(':');
		std::string key;
		std::string section;

		if (separatorPos != std::string::npos) {
			key = a_iniKey.substr(0, separatorPos);
			section = a_iniKey.substr(separatorPos + 1);
		} else {
			// Handle case where the separator is not found
			if (a_vm) {
				a_vm->TraceStack(("_ts_SKSEFunctions - GetValueFromINI: Error - Invalid ini setting format '" + a_iniKey + "'. Expecting 'key:section'.").c_str(), 
							a_stackId, RE::BSScript::ErrorLogger::Severity::kError);
			}
			return a_defaultValue;
		}

		try {
			CSimpleIniA ini;
			if (ini.LoadFile(iniPath.string().c_str()) != SI_OK) {
				if (a_vm) {
					a_vm->TraceStack(("_ts_SKSEFunctions - GetValueFromINI: Failed to parse " +iniPath.string()).c_str(), a_stackId);
				}
			}

			if constexpr (std::is_same_v<T, bool>) {
				return ini.GetBoolValue(section.c_str(), key.c_str(), a_defaultValue);
			} else if constexpr (std::is_same_v<T, double>) {
				return ini.GetDoubleValue(section.c_str(), key.c_str(), a_defaultValue);
			} else if constexpr (std::is_same_v<T, long>) {
				return ini.GetLongValue(section.c_str(), key.c_str(), a_defaultValue);
			} else if constexpr (std::is_same_v<T, std::string>) {
				const char* value = ini.GetValue(section.c_str(), key.c_str(), a_defaultValue.c_str());
				return value ? std::string(value) : a_defaultValue;
			} else {
				static_assert(std::false_type::value, "_ts_SKSEFunctions - GetValueFromINI: Unsupported type for INI retrieval");
			}
		} catch (const std::exception& ex) {
			if (a_vm) {
				a_vm->TraceStack(("_ts_SKSEFunctions - GetValueFromINI: Failed to load from .ini: " +std::string(ex.what())).c_str(), 
							a_stackId, RE::BSScript::ErrorLogger::Severity::kError);
			}
		} catch (...) {
			if (a_vm) {
				a_vm->TraceStack("_ts_SKSEFunctions - GetValueFromINI: Failed to load from .ini: Unknown error", 
							a_stackId, RE::BSScript::ErrorLogger::Severity::kError);
			}
		}

		return a_defaultValue;
	}


	/* Execute a function on the main thread if called from a different thread

		Example usage for a function that returns a value:
				float fPosX = _ts_SKSEFunctions::ExecuteOnMainThread([](RE::PlayerCharacter* player) {
					return player->GetPositionX();
				}, player);
		Example usage for a function that does not return a value:
				_ts_SKSEFunctions::ExecuteOnMainThread([](RE::Actor* actor) {
					actor->EvaluatePackage();
				}, myActor);

		NOTE: Don't use this template function from one of Skyrim's Papyrus threads, 
			when passing a function that returns a value! 
			First of all, using the template is not needed anyways
			because the function can be executed on the Papyrus thread directly.

			Secondly, it that case will you will cause a deadlock, 
			as the Papyrus thread will wait for the result of the function, 
			which is executed on the main thread.

	*/
	template <typename Func, typename... Args>
	auto ExecuteOnMainThread(Func&& a_func, Args&&... a_args) -> decltype(a_func(std::forward<Args>(a_args)...)) {
		using ReturnType = decltype(a_func(std::forward<Args>(a_args)...));
		auto currentThreadId = std::this_thread::get_id();
    
		/* Not yet implemented: check if the function is called from a Papyrus thread
		   That will require obtaining the threadIDs for all Papyrus threads for comparison
		if (std::this_thread::get_id() == mainThreadId) {
			spdlog::info("_ts_SKSEFunctions - {}: Executing function on main thread", __func__);
			// If called from the main thread, execute the function directly
			if constexpr (std::is_void_v<ReturnType>) {
				a_func(std::forward<Args>(a_args)...);
			} else {
				return a_func(std::forward<Args>(a_args)...);
			}
		} else {
			*/
			spdlog::info("_ts_SKSEFunctions - {}: Executing function on task interface", __func__);
        // If not called from the main thread, use SKSE::GetTaskInterface()->AddTask
			if constexpr (std::is_void_v<ReturnType>) {
// commented out promise/future code, as it is not needed for void functions
// and will cause delays in the execution, and deadlock in case called from a Papyrus thread

//				auto promise = std::make_shared<std::promise<void>>();
//				auto future = promise->get_future();
//				SKSE::GetTaskInterface()->AddTask([func = std::forward<Func>(func), promise, args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
				SKSE::GetTaskInterface()->AddTask([a_func = std::forward<Func>(a_func), args = std::make_tuple(std::forward<Args>(a_args)...)]() mutable {
					std::apply(a_func, args);
//					promise->set_value();
				});
//				future.get();
			} else {
				auto promise = std::make_shared<std::promise<ReturnType>>();
				auto future = promise->get_future();
				SKSE::GetTaskInterface()->AddTask([a_func = std::forward<Func>(a_func), promise, args = std::make_tuple(std::forward<Args>(a_args)...)]() mutable {
					promise->set_value(std::apply(a_func, args));
				});
				return future.get();
			}
//		}
	}

	template <typename Func, typename... Args>
	void SendToMainThread(Func&& a_func, Args&&... a_args) {
		SKSE::GetTaskInterface()->AddTask([a_func = std::forward<Func>(a_func), args = std::make_tuple(std::forward<Args>(a_args)...)]() mutable {
			std::apply(a_func, args);
		});
	}

	/******************************************************************************************/
	// Below function is authored by Merdiano
	// https://github.com/Meridiano/SkyrimDLL/blob/d9ecea0524b4fd7cd1ec560ab628c9517cba57c6/GetIniConsoleFix/src/main.cpp#L2
	// All credits go to the original author Meridiano!

	// NOTE: This function requires allocation of tramponine memory via SKSE::AllocTrampoline() in the consuming plugin code!
	
	template<typename Func>
	auto WriteFunctionHook(REL::VariantID id, std::size_t copyCount, Func destination) {
		const auto target = REL::Relocation(id).address();
		auto& trampoline = SKSE::GetTrampoline();

		struct XPatch: Xbyak::CodeGenerator {
			using ull = unsigned long long;
			using uch = unsigned char;
			uch workspace[64];
			XPatch(std::uintptr_t baseAddress, ull bytesCount): Xbyak::CodeGenerator(bytesCount + 14, workspace) {
				auto bytePtr = reinterpret_cast<uch*>(baseAddress);
				for (ull i = 0; i < bytesCount; i++) db(*bytePtr++);
				jmp(qword[rip]);
				dq(ull(bytePtr));
			}
		};
		XPatch patch(target, copyCount);
		patch.ready();
		auto patchSize = patch.getSize();
		trampoline.write_branch<5>(target, destination);
		auto alloc = trampoline.allocate(patchSize);
		std::memcpy(alloc, patch.getCode(), patchSize);
		return reinterpret_cast<std::uintptr_t>(alloc);
	}
}
