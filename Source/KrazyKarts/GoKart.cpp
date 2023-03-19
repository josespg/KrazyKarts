// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKart.h"

#include "Net/UnrealNetwork.h"

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

	//DOREPLIFETIME(AGoKart, ReplicatedTransform);
	//DOREPLIFETIME(AGoKart, Velocity);
	DOREPLIFETIME(AGoKart, ServerState);
	DOREPLIFETIME(AGoKart, Throttle);
	DOREPLIFETIME(AGoKart, SteeringThrow);
}

// Called every frame
void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (IsLocallyControlled())
	{
		FGoKartMove Move;
		Move.DeltaTime = DeltaTime;
		Move.SteeringThrow = SteeringThrow;
		Move.Throttle = Throttle;
		// TODO: Set Time

		Server_SendMove(Move);
	}

	FVector Force = GetActorForwardVector() * MaxDrivingForce * Throttle;
	
	Force += GetAirResistance();
	Force += GetRollingResistance();
	
	const FVector Acceleration = Force / Mass;

	Velocity = Velocity + Acceleration * DeltaTime;
	
	ApplyRotation(DeltaTime);
	
	UpdateLocationFromVelocity(DeltaTime);

	if (HasAuthority())
	{
		ServerState.Transform = GetActorTransform();
		ServerState.Velocity = Velocity;
		// TODO: Update last move
	}

	DrawDebugString(GetWorld(), FVector(0.0f, 0.0f, 1.0f), RoleNames[GetLocalRole()], this, FColor::White, DeltaTime);
}

void AGoKart::OnRep_ServerState()
{
	SetActorTransform(ServerState.Transform);
	Velocity = ServerState.Velocity;
}

void AGoKart::UpdateLocationFromVelocity(float DeltaTime)
{
	const FVector Translation = Velocity * 100 * DeltaTime;

	FHitResult Hit;
	AddActorWorldOffset(Translation, true, &Hit);
	if (Hit.IsValidBlockingHit())
	{
		Velocity = FVector::Zero();
	}
}

void AGoKart::ApplyRotation(float DeltaTime)
{
	// dx = dTHETA x radius

	const float DeltaLocation = FVector::DotProduct(GetActorForwardVector(), Velocity) * DeltaTime; //Velocity.Size() * DeltaTime; 
	const float RotationAngle = (DeltaLocation / MinTurningRadius) * SteeringThrow;
	const FQuat RotationDelta(GetActorUpVector(), RotationAngle);

	Velocity = RotationDelta.RotateVector(Velocity);

	AddActorWorldRotation(RotationDelta);
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
	Throttle = Move.Throttle;
	SteeringThrow = Move.SteeringThrow;
}

bool AGoKart::Server_SendMove_Validate(const FGoKartMove& Move)
{
	return true; // TODO: Make better validation
}
