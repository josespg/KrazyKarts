// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GoKart.generated.h"

USTRUCT()
struct FGoKartMove
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	float Throttle;

	UPROPERTY()
	float SteeringThrow;

	UPROPERTY()
	float DeltaTime;

	// To distinguish moves, use a timestamp like this, time where the move started.
	UPROPERTY()
	float Time;
};

USTRUCT()
struct FGoKartState
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FTransform Transform;

	UPROPERTY()
	FVector Velocity;

	UPROPERTY()
	FGoKartMove LastMove;
};

UCLASS()
class KRAZYKARTS_API AGoKart : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	AGoKart();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

private:

	void SimulateMove(const FGoKartMove& Move);

	FGoKartMove CreateMove(float DeltaTime) const;

	void ClearAcknowledgedMoves(const FGoKartMove& LastMove);

	FVector GetAirResistance() const;
	FVector GetRollingResistance() const;
	void UpdateLocationFromVelocity(float DeltaTime);
	void ApplyRotation(float DeltaTime, float SteeringThrow);

	// The mass of the car (kg).
	UPROPERTY(EditAnywhere)
	float Mass = 1000.0f;

	// The force applied to the car when the throttle is full down (N).
	UPROPERTY(EditAnywhere)
	//float MaxDrivingForce = 1000000.0f;
	float MaxDrivingForce = 10000.0f;

	// The number of degrees rotated per second at full control throw (degrees/s).
	UPROPERTY(EditAnywhere)
	float MaxDegreesPerSecond = 90.0f;
	
	// Minimum radius of the car turning circle at full lock (m).
	UPROPERTY(EditAnywhere)
	float MinTurningRadius = 10.0f;

	// Higher means more drag (Kg*m ?).
	// AirResistance = -Speed^2 * DragCoefficient
	// AirResistance / Speed^2 = DragCoefficient
	// 10000 / 25^2 = DragCoefficient = 16
	UPROPERTY(EditAnywhere)
	float DragCoefficient = 16.0f;//0.16

	// Higher means more rolling resistance
	// From Rolling Resistance wikipedia page coefficient examples
	UPROPERTY(EditAnywhere)
	float RollingResistanceCoefficient = 0.15f;//15.0f;

	void MoveForward(float Value);
	void MoveRight(float Value);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendMove(const FGoKartMove& Move);

	UPROPERTY(ReplicatedUsing = OnRep_ServerState)
	FGoKartState ServerState;

	UFUNCTION()
	void OnRep_ServerState();

	FVector Velocity;

	UPROPERTY(Replicated)
	FGoKartMove CurrentMove;

	float Throttle;
	float SteeringThrow;

	TArray<FGoKartMove> UnacknowledgedMoves;
};
