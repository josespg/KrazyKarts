// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKart.h"

#include "Net/UnrealNetwork.h"
#include "GameFramework/GameStateBase.h"

const FString RoleNames[ENetRole::ROLE_MAX] = { "None", "Simulated Proxy", "Autonomous Proxy", "Authority" };

// Sets default values
AGoKart::AGoKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
}

// Called when the game starts or when spawned
void AGoKart::BeginPlay()
{
	Super::BeginPlay();

	Velocity = FVector::Zero();

	if (HasAuthority())
	{
		NetUpdateFrequency = 1.0f;
	}
}

void AGoKart::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGoKart, ServerState);
}

// Called every frame
void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		FGoKartMove Move = CreateMove(DeltaTime);
		SimulateMove(Move);

		UnacknowledgedMoves.Add(Move);
		Server_SendMove(Move);
	}

	// We are the server and in control of the pawn
	if (GetLocalRole() == ROLE_Authority && IsLocallyControlled())
	{
		FGoKartMove Move = CreateMove(DeltaTime);
		Server_SendMove(Move);
	}

	if (GetLocalRole() == ROLE_SimulatedProxy)
	{
		SimulateMove(ServerState.LastMove);
	}

	DrawDebugString(GetWorld(), FVector(0.0f, 0.0f, 1.0f), RoleNames[GetLocalRole()], this, FColor::White, DeltaTime);
}

void AGoKart::OnRep_ServerState()
{
	SetActorTransform(ServerState.Transform);
	Velocity = ServerState.Velocity;

	ClearAcknowledgedMoves(ServerState.LastMove);

	for (const FGoKartMove& Move : UnacknowledgedMoves)
	{
		SimulateMove(Move);
	}
}

void AGoKart::SimulateMove(const FGoKartMove& Move)
{
	FVector Force = GetActorForwardVector() * MaxDrivingForce * Move.Throttle;

	Force += GetAirResistance();
	Force += GetRollingResistance();

	const FVector Acceleration = Force / Mass;

	Velocity = Velocity + Acceleration * Move.DeltaTime;

	ApplyRotation(Move.DeltaTime, Move.SteeringThrow);

	UpdateLocationFromVelocity(Move.DeltaTime);
}

FGoKartMove AGoKart::CreateMove(float DeltaTime) const
{
	FGoKartMove Move;
	Move.DeltaTime = DeltaTime;
	Move.SteeringThrow = SteeringThrow;
	Move.Throttle = Throttle;
	Move.Time = GetWorld()->TimeSeconds;
	//Move.Time = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();

	return Move;
}

void AGoKart::ClearAcknowledgedMoves(const FGoKartMove& LastMove)
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

FVector AGoKart::GetAirResistance() const
{
	return -Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient;
}

FVector AGoKart::GetRollingResistance() const
{
	const float AccelerationDueToGravity = -GetWorld()->GetGravityZ() / 100.0f;
	const float NormalForce = Mass * AccelerationDueToGravity;
	return -Velocity.GetSafeNormal() * RollingResistanceCoefficient * NormalForce;
}

void AGoKart::ApplyRotation(float DeltaTime, float _SteeringThrow)
{
	// dx = dTHETA x radius

	const float DeltaLocation = FVector::DotProduct(GetActorForwardVector(), Velocity) * DeltaTime;
	const float RotationAngle = (DeltaLocation / MinTurningRadius) * _SteeringThrow;
	const FQuat RotationDelta(GetActorUpVector(), RotationAngle);

	Velocity = RotationDelta.RotateVector(Velocity);

	AddActorWorldRotation(RotationDelta);
}

void AGoKart::UpdateLocationFromVelocity(float DeltaTime)
{
	const FVector Translation = Velocity * 100.0f * DeltaTime;

	FHitResult Hit;
	AddActorWorldOffset(Translation, true, &Hit);
	if (Hit.IsValidBlockingHit())
	{
		Velocity = FVector::Zero();
	}
}

// Called to bind functionality to input
void AGoKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AGoKart::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGoKart::MoveRight);		
}

void AGoKart::MoveForward(float Value)
{
	Throttle = Value;
}

void AGoKart::MoveRight(float Value)
{
	SteeringThrow = Value;
}

void AGoKart::Server_SendMove_Implementation(const FGoKartMove& Move)
{
	SimulateMove(Move);
	
	ServerState.LastMove = Move;
	ServerState.Transform = GetActorTransform();
	ServerState.Velocity = Velocity;
	// TODO: Update last move
}

bool AGoKart::Server_SendMove_Validate(const FGoKartMove& Move)
{
	return true; // TODO: Make better validation
}
