#include "FuriSkillDataAsset.h"

bool UFuriSkillDataAsset::GetSkillDataByTag(FGameplayTag SkillTag, FFuriSkillData& OutData) const
{
	// Map에 해당 태그를 가진 스킬이 존재하는지 찾습니다.
	if (const FFuriSkillData* FoundData = SkillDataMap.Find(SkillTag))
	{
		OutData = *FoundData;
		return true;
	}
    
	// 못 찾았을 경우 에러 로그를 띄워 기획자의 데이터 누락 실수를 방지합니다.
	UE_LOG(LogTemp, Error, TEXT("UFuriSkillDataAsset: SkillTag [%s]에 해당하는 스킬 데이터가 없습니다!"), *SkillTag.ToString());
	return false;
}