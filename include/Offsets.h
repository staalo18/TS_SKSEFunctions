#pragma once
// Offsets are from 'True Directional Movement':
// https://github.com/ersh1/TrueDirectionalMovement
// All credits go to the original author Ersh!

// functions
typedef RE::NiAVObject*(__fastcall* tNiAVObject_LookupBoneNodeByName)(RE::NiAVObject* a_this, const RE::BSFixedString& a_name, bool a3);
static REL::Relocation<tNiAVObject_LookupBoneNodeByName> NiAVObject_LookupBoneNodeByName{ RELOCATION_ID(74481, 76207) };
