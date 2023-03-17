// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKart.h"

#include "AITypes.h"
#include "Math/UnitConversion.h"

// Sets default values
AGoKart::AGoKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AGoKart::BeginPlay()
{
	Super::BeginPlay();

	Velocity = FVector::Zero();
}

void AGoKart::UpdateLocationFromVelocity(float DeltaTime)
{
	const FVector Translation = Velocity * 100 * DeltaTime;
	
	FHitResult Hit;
	AddActorWorldOffset(Translation, true, &Hit);
	if(Hit.IsValidBlockingHit())
	{
		Velocity = FVector::Zero();
	}
}

FVector AGoKart::GetAirResistance() const
{
	return - Velocity.GetSafeNormal() * Velocity.SizeSquared() * DragCoefficient;
}

FVector AGoKart::GetRollingResistance() const
{
	const float AccelerationDueToGravity = - GetWorld()->GetGravityZ() / 100.0f;
	const float NormalForce = Mass * AccelerationDueToGravity;
	return - Velocity.GetSafeNormal() * RollingResistanceCoefficient * NormalForce;
}

void AGoKart::ApplyRotation(float DeltaTime)
{
	// const float RotationAngle = MaxDegreesPerSecond * DeltaTime * SteeringThrow;
	// const FQuat RotationDelta = FQuat(GetActorUpVector(), FMath::DegreesToRadians(RotationAngle));
	// AddActorWorldRotation(RotationDelta);
	//
	// Velocity = RotationDelta.RotateVector(Velocity);


	// dx = dTHETA x radius
	
	const float DeltaLocation = FVector::DotProduct(GetActorForwardVector(), Velocity) * DeltaTime; //Velocity.Size() * DeltaTime; 
	const float RotationAngle = (DeltaLocation / MinTurningRadius) * SteeringThrow;
	const FQuat RotationDelta(GetActorUpVector(), RotationAngle);

	Velocity = RotationDelta.RotateVector(Velocity);

	AddActorWorldRotation(RotationDelta);
}

// Called every frame
void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//float Speed = Velocity.Size();
	//float AirResistance = - (Speed * Speed * DragCoefficient);

	FVector Force = GetActorForwardVector() * MaxDrivingForce * Throttle;
	// FVector NormalisedForce = Force;
	// NormalisedForce.Normalize();
	// Force = Force - NormalisedForce * (Speed * Speed * DragCoefficient);
	
	Force += GetAirResistance();
	Force += GetRollingResistance();
	
	const FVector Acceleration = Force / Mass;

	Velocity = Velocity + Acceleration * DeltaTime;
	
	ApplyRotation(DeltaTime);
	
	UpdateLocationFromVelocity(DeltaTime);
}

// Called to bind functionality to input
void AGoKart::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AGoKart::Server_MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGoKart::Server_MoveRight);
}

void AGoKart::Server_MoveForward_Implementation(float Value)
{
	Throttle = Value;
}

bool AGoKart::Server_MoveForward_Validate(float Value)
{
	return FMath::Abs(Value) <= 1;
}

void AGoKart::Server_MoveRight_Implementation(float Value)
{
	SteeringThrow = Value;
}

bool AGoKart::Server_MoveRight_Validate(float Value)
{
	return FMath::Abs(Value) <= 1;
}
