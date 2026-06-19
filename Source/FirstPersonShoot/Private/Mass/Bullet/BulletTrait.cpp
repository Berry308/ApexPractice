// Fill out your copyright notice in the Description page of Project Settings.


#include "Mass/Bullet/BulletTrait.h"
#include "Mass/Bullet/BulletFragments.h"
#include "MassCommonFragments.h"
#include "MassEntityTemplateRegistry.h"


void UBulletTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const
{
	BuildContext.AddFragment<FTransformFragment>();
	BuildContext.AddFragment<FBulletSimulationFragment>();
	BuildContext.AddFragment<FBulletHitFragment>();
	BuildContext.AddFragment<FBulletVisionFragment>();
	BuildContext.AddChunkFragment<FBulletSimTimerChunkFragment>();
}
