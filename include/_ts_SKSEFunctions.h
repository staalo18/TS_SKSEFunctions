#pragma once

#include <string>
#include <filesystem>
#include <SimpleIni.h>
#include <thread>
#include <future>


namespace _ts_SKSEFunctions {

    void InitializeLogging();

	void WaitWhileGameIsPaused(int checkInterval_ms = 100);

	RE::VMHandle GetHandle(const RE::TESForm* akForm);

    bool IsFormValid(RE::TESForm* form, bool checkDeleted = true);

	void RegisterForSingleUpdate(RE::VMHandle handle, float delayInSeconds);
	
	void SetAngle(RE::TESObjectREFR* a_ref, RE::NiPoint3 a_angle);

	void SetAngleX(RE::TESObjectREFR* a_ref, float a_angleX);

	void SetAngleY(RE::TESObjectREFR* a_ref, float a_angleY);

	void SetAngleZ(RE::TESObjectREFR* a_ref, float a_angleZ);

	void MoveTo(RE::TESObjectREFR* object, const RE::TESObjectREFR* target, 
				float fOffsetX = 0.0f, float fOffsetY = 0.0f, float fOffsetZ = 0.0f);

	float GetDistance(RE::TESObjectREFR* a_ref1, RE::TESObjectREFR* a_ref2);

	void SetLookAt(RE::Actor* actor, RE::TESObjectREFR* target, bool pathingLookAt = false);

	void ClearLookAt(RE::Actor* actor);

	void SendCustomEvent(RE::VMHandle handle, std::string eventName, RE::BSScript::IFunctionArguments * args);

	bool CheckForPackage(RE::Actor* akActor, const RE::BGSListForm* Packagelist, RE::TESPackage* CheckPackage = nullptr);

    bool IsPlayerInRegion(const std::string& regionName);

	int GetFlyingState(RE::Actor* akActor);

	bool IsFlying(RE::Actor* akActor);

	bool HasLOS(RE::Actor* akActor, RE::TESObjectREFR* target);

	int GetCombatState(RE::Actor* akActor);

    bool IsFlyingMountPatrolQueued(RE::Actor* akActor);

    bool IsFlyingMountFastTravelling(RE::Actor* akActor);

    float GetLandHeight(float a_x, float a_y, float a_z);

    float GetLandHeightWithWater(RE::TESObjectREFR* a_ref);

    bool ClearCombatTargets(RE::Actor* a_actor);

	RE::Actor* GetCombatTarget(RE::Actor* actor);

	void StartCombat(RE::Actor* a_actor, RE::Actor* a_target);

    std::vector<RE::Actor*> GetCombatMembers(const RE::Actor* a_actor);

	// call a global papyrus function from C++
    template <class ... Args>
	bool CallPapyrusFunction(std::string_view functionClass, std::string_view function, Args... a_args) {
		// example usage:
		// _ts_SKSEFunctions::CallPapyrusFunction("Game"sv, "FastTravel"sv, FastTravelTarget);
		const auto skyrimVM = RE::SkyrimVM::GetSingleton();
		auto vm = skyrimVM ? skyrimVM->impl : nullptr;
		if (vm) {
			RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
			auto args = RE::MakeFunctionArguments(std::forward<Args>(a_args)...);
			return vm->DispatchStaticCall(std::string(functionClass).c_str(), std::string(function).c_str(), args, callback);
		}
		spdlog::error("_ts_SKSEFunctions - {}: could not call function {}.{}", __func__, functionClass, function);
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
	bool CallPapyrusFunctionOn(RE::TESForm* a_form, std::string_view formKind, std::string_view function, Args... a_args) {
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
			auto objectPtr = GetObjectPtr(a_form, std::string(formKind).c_str(), false);
			if (!objectPtr) {
				spdlog::error("_ts_SKSEFunctions - {}: Could not bind form", __func__);
				return false;
			}
			bool bDispatch = vm->DispatchMethodCall1(objectPtr, std::string(function).c_str(), args, callback);
			if (!bDispatch) {
				spdlog::error("_ts_SKSEFunctions - {}: Could not dispatch method call", __func__);
			}
			return bDispatch;
		}
		return false;
	}


	template <typename T>
	bool UpdateIniSetting(const std::string& settingName, T value) {
		auto* settingCollection = RE::INISettingCollection::GetSingleton();
		RE::Setting* setting = settingCollection->GetSetting(settingName.c_str());
		if (!setting) {
			spdlog::error("_ts_SKSEFunctions - {}: Failed to get INI variable: {}", __func__, settingName);
			return false;
		}

		switch (setting->GetType()) {
			case RE::Setting::Type::kBool:
				if constexpr (std::is_same_v<T, bool>) {
					setting->data.b = value;
				} else {
					spdlog::error("_ts_SKSEFunctions - {}: Type mismatch for INI variable: {}", __func__, settingName);
					return false;
				}
				break;
			case RE::Setting::Type::kFloat:
				if constexpr (std::is_same_v<T, float>) {
					setting->data.f = value;
				} else {
					spdlog::error("_ts_SKSEFunctions - {}: Type mismatch for INI variable: {}", __func__, settingName);
					return false;
				}
				break;
			case RE::Setting::Type::kSignedInteger:
				if constexpr (std::is_same_v<T, std::int32_t>) {
					setting->data.i = value;
				} else {
					spdlog::error("_ts_SKSEFunctions - {}: Type mismatch for INI variable: {}", __func__, settingName);
					return false;
				}
				break;
			case RE::Setting::Type::kColor:
				if constexpr (std::is_same_v<T, RE::Color>) {
					setting->data.r = value;
				} else {
					spdlog::error("_ts_SKSEFunctions - {}: Type mismatch for INI variable: {}", __func__, settingName);
					return false;
				}
				break;
			case RE::Setting::Type::kString:
				if constexpr (std::is_same_v<T, const char*>) {
					setting->data.s = const_cast<char*>(value);
				} else {
					spdlog::error("_ts_SKSEFunctions - {}: Type mismatch for INI variable: {}", __func__, settingName);
					return false;
				}
				break;
			case RE::Setting::Type::kUnsignedInteger:
				if constexpr (std::is_same_v<T, std::uint32_t>) {
					setting->data.u = value;
				} else {
					spdlog::error("_ts_SKSEFunctions - {}: Type mismatch for INI variable: {}", __func__, settingName);
					return false;
				}
				break;
			default:
			spdlog::error("_ts_SKSEFunctions - {}: Unknown type for INI variable: {}", __func__, settingName);
				return false;
		}
	return true;
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
	auto ExecuteOnMainThread(Func&& func, Args&&... args) -> decltype(func(std::forward<Args>(args)...)) {
		using ReturnType = decltype(func(std::forward<Args>(args)...));
		auto currentThreadId = std::this_thread::get_id();
    
		/* Not yet implemented: check if the function is called from a Papyrus thread
		   That will require obtaining the threadIDs for all Papyrus threads for comparison
		if (std::this_thread::get_id() == mainThreadId) {
			spdlog::info("_ts_SKSEFunctions - {}: Executing function on main thread", __func__);
			// If called from the main thread, execute the function directly
			if constexpr (std::is_void_v<ReturnType>) {
				func(std::forward<Args>(args)...);
			} else {
				return func(std::forward<Args>(args)...);
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
				SKSE::GetTaskInterface()->AddTask([func = std::forward<Func>(func), args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
					std::apply(func, args);
//					promise->set_value();
				});
//				future.get();
			} else {
				auto promise = std::make_shared<std::promise<ReturnType>>();
				auto future = promise->get_future();
				SKSE::GetTaskInterface()->AddTask([func = std::forward<Func>(func), promise, args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
					promise->set_value(std::apply(func, args));
				});
				return future.get();
			}
//		}
	}

	template <typename Func, typename... Args>
	void SendToMainThread(Func&& func, Args&&... args) {
		SKSE::GetTaskInterface()->AddTask([func = std::forward<Func>(func), args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
			std::apply(func, args);
		});
	}
}
