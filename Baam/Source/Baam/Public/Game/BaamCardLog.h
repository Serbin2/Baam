// BANG! 카드/덱 전용 로그 카테고리 — PIE 다중 클라 로그에서 카드 흐름만 골라 보기 위함.
// Network/BaamNetLog.h 의 LogBaamNet 과 같은 패턴.
//
// 콘솔에서 이것만 보려면:  Log LogBaamCard Verbose

#pragma once

#include "CoreMinimal.h"
#include "Logging/LogMacros.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBaamCard, Log, All);
