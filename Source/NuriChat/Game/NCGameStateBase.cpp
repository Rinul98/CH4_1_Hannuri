// NCGameStateBase.cpp


#include "NCGameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Player/NCPlayerController.h"
#include "Net/UnrealNetwork.h"

void ANCGameStateBase::MulticastRPCBroadcastLoginMessage_Implementation(const FString& InNameString)
{
	if (HasAuthority() == false) // 서버가 아니면
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0); // 내 로컬만 있다
		if (IsValid(PC) == true)
		{
			ANCPlayerController* NCPC = Cast<ANCPlayerController>(PC);
			if (IsValid(NCPC) == true)
			{
				FString NotificationString = InNameString + TEXT(" has joined the game.");
				NCPC->PrintChatMessageString(NotificationString);
			}
		}
	}
}

void ANCGameStateBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, RemainingTurnTime);
	DOREPLIFETIME(ThisClass, CurrentTurnPlayerName);
}
