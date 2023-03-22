// Fill out your copyright notice in the Description page of Project Settings.


#include "GoKart.h"

#include "GameFramework/GameStateBase.h"

#include "GoKartMovementComponent.h"
#include "GoKartReplicationComponent.h"


// Sets default values
AGoKart::AGoKart()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	
	MovementComponent = CreateDefaultSubobject<UGoKartMovementComponent>(TEXT("Movement Component"));
	ReplicationComponent = CreateDefaultSubobject<UGoKartReplicationComponent>(TEXT("Replication Component"));
}

// Called when the game starts or when spawned
void AGoKart::BeginPlay()
{
	Super::BeginPlay();

	SetReplicateMovement(false);

	if (HasAuthority())
	{
		NetUpdateFrequency = 1.0f;
	}
}

// Called every frame
void AGoKart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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
	if (!MovementComponent)	return; 
	
	MovementComponent->SetThrottle(Value);
}

void AGoKart::MoveRight(float Value)
{
	if (!MovementComponent)	return; 
	
	MovementComponent->SetSteeringThrow(Value);
}
