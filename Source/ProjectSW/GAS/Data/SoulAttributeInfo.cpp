// 


#include "SoulAttributeInfo.h"

#include "Utility/SoulLog.h"

FSoulAttributeInfo UAttributeInfo::FindAttributeInfoForTag(const FGameplayTag& AttributeTag, bool bLogNotFound) const
{
	/* TODO : Move to UI폴더? */
	/* AttributeInfo 클래스는 엔진에 등록된 모든 Attribute 이하 선언된 GameplayTag를 저장한 동적 배열 보유.
	 * 파라미터로 넘겨받은 GameplayTag를 동적배열을 순회해 검사하고, 일치하는 FSoulAttributeInfo 구조체를 반환. 
	 * 해당 동적 배열에 등록은 DA 에셋에서 수동으로 진행. */
	
	for (const FSoulAttributeInfo& Info : AttributeInformation)
	{
		if (Info.AttributeTag.MatchesTagExact(AttributeTag)) return Info;
	}
	
	if (bLogNotFound) SW_LOG_ERROR("Failed to find matching GameplayTag [%s]", *AttributeTag.ToString());
	return FSoulAttributeInfo();
}