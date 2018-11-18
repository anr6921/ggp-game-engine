#include "stdafx.h"

#include "PhysicsManager.h"
#include "RigidBody.h"
#include "GameObject.h"

using namespace DirectX;
using namespace Physics;

PhysicsManager* PhysicsManager::Instance = nullptr;

const UINT PhysicsManager::GetRigidBodyCount() const
{
    return rBodyCount;
}

const bool Physics::PhysicsManager::Raycast( const DirectX::XMFLOAT3 & aOrigin, const DirectX::XMFLOAT3 & aDirectoin, const float distance )
{
    printf( "\tStart raycast!\n" );


    return false;
}

PhysicsManager::PhysicsManager()
{
    rBodyCount = 0;
}

PhysicsManager::~PhysicsManager()
{
    for ( auto itr = RigidBodyUIDMap.begin(); itr != RigidBodyUIDMap.end(); ++itr )
    {
        RigidBody* temp = itr->second;
        if ( temp != nullptr )
        {
            delete temp;
        }
    }
}

PhysicsManager* PhysicsManager::GetInstance()
{
    if ( Instance == nullptr )
    {
        Instance = new PhysicsManager();
    }
    return Instance;
}

void PhysicsManager::ReleaseInstance()
{
    if ( Instance != nullptr )
    {
        delete Instance;
        Instance = nullptr;
    }
}

UINT PhysicsManager::AddRigidBody( GameObject* aGameObj, float aMass, EPhysicsLayer aLayer )
{
    assert( aGameObj );
    RigidBody* rb = new RigidBody( aGameObj, aMass, aLayer );
    // #FixForNextBuild
    // Whyyyyy does this happen? I shouldn't have to put instance in front of this
    Instance->RigidBodyUIDMap.insert( std::pair<UINT, RigidBody*>( Instance->rBodyCount, rb ) );
    
    return Instance->rBodyCount++;
}

RigidBody * PhysicsManager::GetRigidBody( UINT uID )
{
    auto thisRB = RigidBodyUIDMap.find( uID );
    //If found, return it.  Else, return nullptr
    if ( thisRB != RigidBodyUIDMap.end() ) {
        return thisRB->second;
    }
    return nullptr;
}

/** Iteration over a map is o(n), but it is not optimal for caching 
because the data is not stored contiguously. */
void PhysicsManager::UpdatePhysics( float deltaTime )
{
    RigidBody* entityA = nullptr;

    for ( auto itr = RigidBodyUIDMap.begin(); itr != RigidBodyUIDMap.end(); ++itr )
    {
        entityA = itr->second;

        if ( entityA->GetPhysicsLayer() == EPhysicsLayer::STATIC ) continue;

        XMFLOAT3 forceToApply = XMFLOAT3( 0.f, 0.f, 0.f );
        forceToApply.y += Gravity;

        forceToApply.x *= deltaTime;
        forceToApply.y *= deltaTime;
        forceToApply.z *= deltaTime;

        entityA->ApplyForce( forceToApply );

         
        // For each object, we need to check it against all the others 
        // in the scene that can possibly be colliding with it
        Physics::SphereCollider col1 = entityA->GetCollider();
        col1.Center = entityA->gameObject->transform.position;

        for ( auto innerItr = RigidBodyUIDMap.begin(); innerItr != RigidBodyUIDMap.end(); ++innerItr )
        {
            if( innerItr->second == entityA ) continue;
            // For each object, we need to check it against all the others 
            // in the scene that can possibly be colliding with it
            Physics::SphereCollider colOther = innerItr->second->GetCollider();
            colOther.Center = innerItr->second->gameObject->transform.position;
            if ( Physics::Collisions::Intersects( col1, colOther ) )
            {
                //printf( "Collision!\n" );
                // Apply a force to these objects going the oppose way
                XMFLOAT3 difference {};
                difference.x = ( col1.Center.x - colOther.Center.x ) * deltaTime;
                difference.y = ( col1.Center.y - colOther.Center.y ) * deltaTime;
                difference.z = ( col1.Center.z - colOther.Center.z ) * deltaTime;

                entityA->ApplyForce( difference );

            }
        }
        // Check the floor pos        
        if ( entityA->gameObject->transform.position.y < -2.f )
        {
            XMFLOAT3 opposingVel = entityA->GetVelocity();
            opposingVel.x *= -1.f;
            opposingVel.y *= -1.f;
            opposingVel.z *= -1.f;
            entityA->ApplyForce( opposingVel );
        }
        entityA->ApplyAcceleration();

    }

}