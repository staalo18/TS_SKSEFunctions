#pragma once

#include <string>
#include <filesystem>
#include <SimpleIni.h>

namespace _ts_SKSEFunctions {
    void InitializeLogging();

    bool IsFormValid(RE::TESForm* form, bool checkDeleted = true);

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
			spdlog::info("_ts_SKSEFunctions - {}: Calling function {}.{}", __func__, functionClass, function);
			return vm->DispatchStaticCall(std::string(functionClass).c_str(), std::string(function).c_str(), args, callback);
		}
		spdlog::error("_ts_SKSEFunctions - {}: could not call function {}.{}", __func__, functionClass, function);
		return false;
	}   
}
