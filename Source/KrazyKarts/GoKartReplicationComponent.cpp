// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKartReplicationComponent.h"

#include "Net/UnrealNetwork.h"

#include "GoKart.h"


const FString RoleNames[ENetRole::ROLE_MAX] = { "None", "Simulated Proxy", "Autonomous Proxy", "Authority" };

// Sets default values for this component's properties
UGoKartReplicationComponent::UGoKartReplicationComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	SetIsReplicated(true);
}

// Called when the game starts
void UGoKartReplicationComponent::BeginPlay()
{
	Super::BeginPlay();

	MovementComponent = GetOwner()->FindComponentByClass<UGoKartMovementComponent>();
}

void UGoKartReplicationComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGoKartReplicationComponent, ServerState);
}

// Called every frame
void UGoKartReplicationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!MovementComponent)	return;
	//APawn* Pawn = Cast<APawn>(GetOwner());
	//if (!Pawn) return;

	const FGoKartMove& LastMove = MovementComponent->GetLastMove();

	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		UnacknowledgedMoves.Add(LastMove);
		Server_SendMove(LastMove);
	}

	// We are the server and in control of the pawn
	//if(/*GetOwnerRole() == ROLE_Authority && Pawn->IsLocallyControlled())
	if (GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)
	{
		UpdateServerState(LastMove);
	}

	if(GetOwnerRole() == ROLE_SimulatedProxy)
	{
		//MovementComponent->SimulateMove(ServerState.LastMove);
		ClientTick(DeltaTime);
	}

	DrawDebugString(GetWorld(), FVector(0.0f, 0.0f, 1.0f), RoleNames[GetOwnerRole()], GetOwner(), FColor::White, DeltaTime);
	DrawDebugString(GetWorld(), FVector(0.0f, 0.0f, 100.0f), RoleNames[GetOwner()->GetRemoteRole()], GetOwner(), FColor::Green, DeltaTime);
}

void UGoKartReplicationComponent::UpdateServerState(const FGoKartMove& Move)
{
	ServerState.LastMove = Move;
	ServerState.Transform = GetOwner()->GetActorTransform();
	ServerState.Velocity = MovementComponent->GetVelocity();
}

void UGoKartReplicationComponent::ClientTick(float DeltaTime)
{
	ClientTimeSinceUpdate += DeltaTime;

	// We need at least two updates between doing a LERP
	if (ClientTimeBetweenLastUpdates < KINDA_SMALL_NUMBER) return;

	if (!MovementComponent)	return;

	float LerpRatio = ClientTimeSinceUpdate / ClientTimeBetweenLastUpdates;

	FVector TargetLocation = ServerState.Transform.GetLocation();
	FVector StartLocation = ClientStartTransform.GetLocation();
	const float VelocityToDerivative = ClientTimeBetweenLastUpdates * 100.0f;
	FVector StartDerivative = ClientStartVelocity * VelocityToDerivative;
	FVector TargetDerivative = ServerState.Velocity * VelocityToDerivative;
	//FVector NewLocation = FMath::LerpStable<FVector>(StartLocation, TargetLocation, LerpRatio);
	FVector NewLocation = FMath::CubicInterp(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio);
	GetOwner()->SetActorLocation(NewLocation);

	FVector NewDerivative = FMath::CubicInterpDerivative(StartLocation, StartDerivative, TargetLocation, TargetDerivative, LerpRatio);
	FVector NewVelocity = NewDerivative / VelocityToDerivative;
	MovementComponent->SetVelocity(NewVelocity);
		
	FQuat TargetRotation = ServerState.Transform.GetRotation();
	FQuat StartRotation = ClientStartTransform.GetRotation();
	FQuat NewRotation = FQuat::Slerp(StartRotation, TargetRotation, LerpRatio);
	GetOwner()->SetActorRotation(NewRotation);
}

void UGoKartReplicationComponent::OnRep_ServerState()
{
	switch (GetOwnerRole())
	{
		case ROLE_AutonomousProxy:
			AutonomousProxy_OnRep_ServerState();
			break;
		
		case ROLE_SimulatedProxy:
			SimulatedProxy_OnRep_ServerState();
			break;

		default:
			break;
	}
}

void UGoKartReplicationComponent::AutonomousProxy_OnRep_ServerState()
{
	if (!MovementComponent)	return;

	GetOwner()->SetActorTransform(ServerState.Transform);
	MovementComponent->SetVelocity(ServerState.Velocity);

	ClearAcknowledgedMoves(ServerState.LastMove);

	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		MovementComponent->SimulateMove(Move);
	}
}

void UGoKartReplicationComponent::SimulatedProxy_OnRep_ServerState()
{
	if (!MovementComponent)	return;

	ClientTimeBetweenLastUpdates = ClientTimeSinceUpdate;
	ClientTimeSinceUpdate = 0.0f;

	ClientStartTransform = GetOwner()->GetActorTransform();
	ClientStartVelocity = MovementComponent->GetVelocity();
}

void UGoKartReplicationComponent::ClearAcknowledgedMoves(const FGoKartMove& LastMove)
{
	TArray<FGoKartMove> NewMoves;

	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		if (Move.Time > LastMove.Time)
		{
			NewMoves.Add(Move);
		}
	}

	UnacknowledgedMoves = NewMoves;
}

void UGoKartReplicationComponent::Server_SendMove_Implementation(const FGoKartMove& Move)
{
	if (!MovementComponent)	return;

	MovementComponent->SimulateMove(Move);

	UpdateServerState(Move);
}

bool UGoKartReplicationComponent::Server_SendMove_Validate(const FGoKartMove& Move)
{
	return true; // TODO: Make better validation
}
