#include "NCPawn.h"
#include "NuriChat.h"


void ANCPawn::BeginPlay()
{
	Super::BeginPlay();

	FString NetRoleString = NuriChatFunctionLibrary::GetRoleString(this);
	FString CombinedString = FString::Printf(TEXT("CXPawn::BeginPlay() %s [%s]"), *NuriChatFunctionLibrary::GetNetModeString(this), *NetRoleString);
	NuriChatFunctionLibrary::MyPrintString(this, CombinedString, 10.f);
}

void ANCPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	FString NetRoleString = NuriChatFunctionLibrary::GetRoleString(this);
	FString CombinedString = FString::Printf(TEXT("CXPawn::PossessedBy() %s [%s]"), *NuriChatFunctionLibrary::GetNetModeString(this), *NetRoleString);
	NuriChatFunctionLibrary::MyPrintString(this, CombinedString, 10.f);
}