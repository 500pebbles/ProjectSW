// 


#include "SoulInputConfig.h"

#include "Utility/SoulLog.h"

const UInputAction* USoulInputConfig::FindAbilityInputActionForTag(const FGameplayTag& InputTag, bool bLogNotFound) const
{
	for (const FSoulInputAction& Action : AbilityInputActions)
		if (Action.InputAction && Action.InputTag == InputTag) return Action.InputAction;
	
	if (bLogNotFound) SW_LOG_ERROR("InputTag [%s] is not found", *InputTag.ToString());	
		return nullptr;
}