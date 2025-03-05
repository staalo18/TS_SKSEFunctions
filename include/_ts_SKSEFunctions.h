#pragma once

#include <string>
#include <filesystem>
#include <SimpleIni.h>
#include <thread>
#include <future>


namespace _ts_SKSEFunctions {

    void InitializeLogging();

    bool IsFormValid(RE::TESForm* form, bool checkDeleted = true);

    void SendCustomEvent(std::string eventName, std::string result, float numArg, RE::TESObjectREFR* sender);

	bool CheckForPackage(RE::Actor* akActor, const RE::BGSListForm* Packagelist, RE::TESPackage* CheckPackage = nullptr);

    bool IsPlayerInRegion(const std::string& regionName);

    bool IsFlyingMountPatrolQueued(RE::Actor* akActor);

    bool IsFlyingMountFastTravelling(RE::Actor* akActor);

    float GetLandHeight(float a_x, float a_y, float a_z);

    float GetLandHeightWithWater(RE::TESObjectREFR* a_ref);

    bool ClearCombatTargets(RE::Actor* a_actor);

    std::vector<RE::Actor*> GetCombatMembers(const RE::Actor* a_actor);

    template <class ... Args>
	bool CallPapyrusFunction(std::string_view functionClass, std::string_view function, Args... a_args) {
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

    // Global variable to store the main thread ID
    extern std::thread::id mainThreadId;

	// Function to set the main thread ID
	void SetMainThread();

	template <typename Func, typename... Args>
	auto ExecuteOnMainThread(Func&& func, Args&&... args) -> decltype(func(std::forward<Args>(args)...)) {
		using ReturnType = decltype(func(std::forward<Args>(args)...));
		if (std::this_thread::get_id() == mainThreadId) {
			// If called from the main thread, execute the function directly
			if constexpr (std::is_void_v<ReturnType>) {
				func(std::forward<Args>(args)...);
			} else {
				return func(std::forward<Args>(args)...);
			}
		} else {
        // If not called from the main thread, use SKSE::GetTaskInterface()->AddTask
			if constexpr (std::is_void_v<ReturnType>) {
				auto promise = std::make_shared<std::promise<void>>();
				auto future = promise->get_future();
				SKSE::GetTaskInterface()->AddTask([func = std::forward<Func>(func), promise, args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
					std::apply(func, args);
					promise->set_value();
				});
				future.get();
			} else {
				auto promise = std::make_shared<std::promise<ReturnType>>();
				auto future = promise->get_future();
				SKSE::GetTaskInterface()->AddTask([func = std::forward<Func>(func), promise, args = std::make_tuple(std::forward<Args>(args)...)]() mutable {
					promise->set_value(std::apply(func, args));
				});
				return future.get();
			}
		}
	}
}
