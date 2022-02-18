#pragma once
#include "../CSC8503Common/GameWorld.h"
#include <set>

namespace NCL {
	namespace CSC8503 {
		class PhysicsSystem	{
		public:
			PhysicsSystem(GameWorld& g);
			~PhysicsSystem();

			void Clear();

			void Update(float dt);

			void UseGravity(bool state) {
				applyGravity = state;
			}
			bool GetGravity() { return applyGravity; }

			void SetGlobalDamping(float d) {
				globalDamping = d;
			}

			void SetLinearDamping(float d) {
				linearDamping = d;
			}

			void SetGravity(const Vector3& g);

			void BuildStaticList();

		protected:
			void BroadPhase();
			void NarrowPhase();

			void ClearForces();

			void CheckToWake(PhysicsObject* object);
			void CheckToSleep(PhysicsObject* object);

			void IntegrateAccel(float dt, GameObject* object);
			void IntegrateVelocity(float dt, PhysicsObject* object, Transform& transform);

			void UpdateConstraints(float dt);

			void UpdateCollisionList();
			void UpdateObjectAABBs();

			void ImpulseResolveCollision(GameObject& a , GameObject&b, CollisionDetection::ContactPoint& p) const;
			void ResolveSpringCollision(GameObject& a , GameObject&b, CollisionDetection::ContactPoint& p) const;

			GameWorld& gameWorld;
			Octree<GameObject*>* staticTree;

			bool	applyGravity;
			Vector3 gravity;
			float	dTOffset;
			float	globalDamping;
			float	linearDamping;
			std::set<CollisionDetection::CollisionInfo> allCollisions;
			std::set<CollisionDetection::CollisionInfo> broadphaseCollisions;

			bool useBroadPhase		= true;
			int numCollisionFrames	= 5;
		};
	}
}