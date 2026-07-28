// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BaamCardType.Generated.h"

/** 덱에 실재하는 카드 1장. DT Row 가 아니라 런타임 인스턴스. */
USTRUCT(BlueprintType)
struct FBaamCardInstance
{
	GENERATED_BODY()

	// 덱 생성 시 1부터 부여. UI 의 FBangCardView::InstanceId 와 같은 키다.
	UPROPERTY(BlueprintReadOnly) 
	int32 InstanceId = INDEX_NONE;
	
	// DT_BaamCard Row 이름
	UPROPERTY(BlueprintReadOnly) 
	FName CardId;   

	bool IsValid() const { return InstanceId != INDEX_NONE; }
};
