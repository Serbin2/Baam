// BANG! 카드 인스턴스 → UI 뷰모델 변환. 게임 로직과 UI 의 유일한 접점이므로 한곳에 모은다.
//
//   FBaamCardInstance (게임 로직: InstanceId + CardId)
//        + DT_BaamCard 의 FBaamCardRow (이름/설명/특성)
//   → FBangCardView   (UI: 화면에 그릴 것 전부)
//
// UI 계층(UBangHandWidget 등)은 FBaamCardInstance 도 DT 도 모른다. 그 경계를 이 파일이 지킨다.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UI/BangCardView.h"
#include "BaamCardViewLibrary.generated.h"

struct FBaamCardInstance;
class ABaamPlayerState;

UCLASS()
class BAAM_API UBaamCardViewLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 카드 인스턴스 1장을 UI 뷰모델로 변환한다.
	 * DT 에서 Row 를 못 찾으면 CardId 를 그대로 이름에 넣어 화면에서 바로 눈에 띄게 한다.
	 */
	UFUNCTION(BlueprintPure, Category = "Baam|Card", meta = (WorldContext = "WorldContextObject"))
	static FBangCardView MakeCardView(const UObject* WorldContextObject, const FBaamCardInstance& Card);

	/**
	 * 손패 전체를 변환한다.
	 * ⚠️ Hand 는 COND_OwnerOnly 라, 클라에서 다른 플레이어의 PlayerState 를 넘기면 빈 배열이 나온다.
	 *    이는 버그가 아니라 은닉 정보가 정상 동작한다는 뜻이다.
	 */
	UFUNCTION(BlueprintPure, Category = "Baam|Card")
	static TArray<FBangCardView> MakeHandViews(const ABaamPlayerState* PlayerState);
};
