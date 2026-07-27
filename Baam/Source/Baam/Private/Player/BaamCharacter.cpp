#include "Player/BaamCharacter.h"
#include "Player/Component/BaamAbilitySystemComponent.h"
#include "Player/Component/BaamAttributeSet.h"
#include "Game/BaamDataSubsystem.h"
#include "Game/BaamGameDataTypes.h"
#include "Net/UnrealNetwork.h"

ABaamCharacter::ABaamCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	AbilitySystemComponent = CreateDefaultSubobject<UBaamAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	// 7인 턴제라 대역폭이 무의미하고, 모든 클라가 동일한 GE 상태를 봐 디버깅이 쉽다 (md §M3).
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Full);

	AttributeSet = CreateDefaultSubobject<UBaamAttributeSet>(TEXT("AttributeSet"));
}

UAbilitySystemComponent* ABaamCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ABaamCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABaamCharacter, CharacterTag);
}

void ABaamCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 서버: ActorInfo 초기화 후 GAS 최초 부여.
	InitializeAbilityActorInfo();
	ServerInitGAS();
}

void ABaamCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	// 소유 클라: ActorInfo 초기화 (없으면 UI 가 어트리뷰트/태그를 못 읽는다).
	InitializeAbilityActorInfo();
}

void ABaamCharacter::InitializeAbilityActorInfo()
{
	if (AbilitySystemComponent)
	{
		// OwnerActor·AvatarActor 모두 this (Character 소유 ASC).
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void ABaamCharacter::ServerInitGAS()
{
	if (!HasAuthority() || !AbilitySystemComponent || bGASInitialized)
	{
		return;
	}
	bGASInitialized = true;

	ApplyCharacterDataRow(CharacterTag);
	RefreshCharacterLooseTag();
}

void ABaamCharacter::SetCharacterTag(const FGameplayTag& NewTag)
{
	CharacterTag = NewTag;

	// 이미 초기화됐다면 즉시 재적용, 아니면 ServerInitGAS 가 최초 적용한다.
	if (HasAuthority() && bGASInitialized)
	{
		ApplyCharacterDataRow(CharacterTag);
		RefreshCharacterLooseTag();
	}
}

void ABaamCharacter::OnRep_CharacterTag()
{
	// 소유 클라: 역할 태그를 로컬 ASC 루즈 태그에 반영 (HUD/능력 조건 검사용).
	RefreshCharacterLooseTag();
}

void ABaamCharacter::ApplyCharacterDataRow(const FGameplayTag& RowTag)
{
	if (!HasAuthority() || !AbilitySystemComponent || !RowTag.IsValid())
	{
		return;
	}

	const UGameInstance* GI = GetGameInstance();
	UBaamDataSubsystem* DataSubsystem = GI ? GI->GetSubsystem<UBaamDataSubsystem>() : nullptr;
	if (!DataSubsystem)
	{
		return;
	}

	const FBaamCharacterRow* Row = DataSubsystem->GetCharacterRow(RowTag);
	if (!Row)
	{
		return;
	}

	// 유효한 Row 를 찾았을 때만 이전 부여분을 정리한다
	// (잘못된 태그로 능력이 전부 날아가는 것 방지).
	ClearCharacterDataRow();

	// ── 스탯 GE + 패시브 GE 적용 ──
	TArray<TSubclassOf<UGameplayEffect>> EffectsToApply;
	if (Row->DefaultAttributeGE)
	{
		EffectsToApply.Add(Row->DefaultAttributeGE);
	}
	EffectsToApply.Append(Row->PassiveEffects);

	for (const TSubclassOf<UGameplayEffect>& EffectClass : EffectsToApply)
	{
		if (!EffectClass)
		{
			continue;
		}

		FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
		Context.AddSourceObject(this);

		const FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1.f, Context);
		if (Spec.IsValid())
		{
			// Infinite/Duration GE 만 유효 핸들을 돌려준다(Instant 는 무효) → 재적용 시 제거용으로 추적.
			const FActiveGameplayEffectHandle Handle =
				AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			if (Handle.IsValid())
			{
				AppliedEffectHandles.Add(Handle);
			}
		}
	}

	// ── 능동 어빌리티 부여 ──
	for (const TSubclassOf<UGameplayAbility>& AbilityClass : Row->GrantedAbilities)
	{
		if (AbilityClass)
		{
			GrantedAbilityHandles.Add(
				AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this)));
		}
	}
}

void ABaamCharacter::ClearCharacterDataRow()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	for (const FActiveGameplayEffectHandle& Handle : AppliedEffectHandles)
	{
		if (Handle.IsValid())
		{
			AbilitySystemComponent->RemoveActiveGameplayEffect(Handle);
		}
	}
	AppliedEffectHandles.Reset();

	for (const FGameplayAbilitySpecHandle& Handle : GrantedAbilityHandles)
	{
		if (Handle.IsValid())
		{
			AbilitySystemComponent->ClearAbility(Handle);
		}
	}
	GrantedAbilityHandles.Reset();
}

void ABaamCharacter::RefreshCharacterLooseTag()
{
	if (!AbilitySystemComponent || AppliedLooseTag == CharacterTag)
	{
		return;
	}

	if (AppliedLooseTag.IsValid())
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(AppliedLooseTag);
	}
	if (CharacterTag.IsValid())
	{
		AbilitySystemComponent->AddLooseGameplayTag(CharacterTag);
	}
	AppliedLooseTag = CharacterTag;
}
