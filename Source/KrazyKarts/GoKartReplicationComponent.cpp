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

	// We are a client owning this vehicle
	if (GetOwnerRole() == ROLE_AutonomousProxy)
	{
		UnacknowledgedMoves.Add(LastMove);
		Server_SendMove(LastMove);
	}

	// We are the server and in control of this pawn (listen server)
	//if(/*GetOwnerRole() == ROLE_Authority && Pawn->IsLocallyControlled())
	if (GetOwner()->GetRemoteRole() == ROLE_SimulatedProxy)
	{
		UpdateServerState(LastMove);
	}

	// We are a client and this vehicle is owned by another client
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
	// Cubic interpolation for vehicle owned by other clients
	
	ClientTimeSinceUpdate += DeltaTime;

	// We need at least two updates between doing a LERP
	if (ClientTimeBetweenLastUpdates < KINDA_SMALL_NUMBER) return;

	if (!MovementComponent)	return;

	const float LerpRatio = ClientTimeSinceUpdate / ClientTimeBetweenLastUpdates;

	const FHermiteCubicSpline Spline = CreateSpline();
	
	InterpolateLocation(Spline, LerpRatio);

	InterpolateVelocity(Spline, LerpRatio);
		
	InterpolateRotation(Spline, LerpRatio);
}

FHermiteCubicSpline UGoKartReplicationComponent::CreateSpline() const
{
	FHermiteCubicSpline Spline;
	Spline.TargetLocation = ServerState.Transform.GetLocation();
	Spline.StartLocation = ClientStartTransform.GetLocation();
	Spline.StartDerivative = ClientStartVelocity * VelocityToDerivative();
	Spline.TargetDerivative = ServerState.Velocity * VelocityToDerivative();

	return Spline;
}

void UGoKartReplicationComponent::InterpolateLocation(const FHermiteCubicSpline& Spline, float LerpRatio)
{
	FVector NewLocation = Spline.InterpolateLocation(LerpRatio);
	//GetOwner()->SetActorLocation(NewLocation);

	// We interpolate the mesh of the vehicle (location, velocity and rotation) per frame
	// But the collision box only is updated when receiving an update from the server with
	// the real position in the server, aka, no interpolation for the collision box, to
	// avoid collision artifacts. (This comment applies to InterpolateVelocity and InterpolateRotation).
	if (MeshOffsetRoot)
	{
		MeshOffsetRoot->SetWorldLocation(NewLocation);
	}
}

void UGoKartReplicationComponent::InterpolateVelocity(const FHermiteCubicSpline& Spline, float LerpRatio)
{
	FVector NewDerivative = Spline.InterpolateDerivative(LerpRatio);
	FVector NewVelocity = NewDerivative / VelocityToDerivative();
	MovementComponent->SetVelocity(NewVelocity);
}

void UGoKartReplicationComponent::InterpolateRotation(const FHermiteCubicSpline& Spline, float LerpRatio)
{
	FQuat TargetRotation = ServerState.Transform.GetRotation();
	FQuat StartRotation = ClientStartTransform.GetRotation();
	FQuat NewRotation = FQuat::Slerp(StartRotation, TargetRotation, LerpRatio);
	//GetOwner()->SetActorRotation(NewRotation);
	if (MeshOffsetRoot)
	{
		MeshOffsetRoot->SetWorldRotation(NewRotation);
	}
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
	// This is run in client for a vehicle owned by this client

	if (!MovementComponent)	return;

	GetOwner()->SetActorTransform(ServerState.Transform);
	MovementComponent->SetVelocity(ServerState.Velocity);

	// Clear server moves older than the one we have just received
	ClearAcknowledgedMoves(ServerState.LastMove);

	// And replay the moves newer
	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		MovementComponent->SimulateMove(Move);
	}
}

void UGoKartReplicationComponent::SimulatedProxy_OnRep_ServerState()
{
	// This is run in client for a vehicle owned by another client

	if (!MovementComponent)	return;

	ClientTimeBetweenLastUpdates = ClientTimeSinceUpdate;
	ClientTimeSinceUpdate = 0.0f;

	if (MeshOffsetRoot)
	{
		//ClientStartTransform = GetOwner()->GetActorTransform();
		ClientStartTransform.SetLocation(MeshOffsetRoot->GetComponentLocation());
		ClientStartTransform.SetRotation(MeshOffsetRoot->GetComponentQuat());
	}

	ClientStartVelocity = MovementComponent->GetVelocity();

	GetOwner()->SetActorTransform(ServerState.Transform);
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

	ClientSimulatedTime += Move.DeltaTime;

	MovementComponent->SimulateMove(Move);

	UpdateServerState(Move);
}

bool UGoKartReplicationComponent::Server_SendMove_Validate(const FGoKartMove& Move)
{	
	float ProposedTime = ClientSimulatedTime + Move.DeltaTime;
	bool ClientNotRunningAhead = ProposedTime < GetWorld()->TimeSeconds;
	if (!ClientNotRunningAhead)
	{
		UE_LOG(LogTemp, Error, TEXT("Client running too fast."));
		return false;
	}

	if(!Move.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Received invalid move."));
		return false;
	}

	return true;
}