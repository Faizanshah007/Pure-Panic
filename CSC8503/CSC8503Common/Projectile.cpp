#include "Projectile.h"
#include "../GameTech/Renderer.h"
#include "../CSC8503Common/AudioManager.h"
#include "../../Common/Assets.h"


void Projectile::Update(float dt) {
	Vector3 velocityDir = (this->GetPhysicsObject()->GetLinearVelocity()).Normalised();
	this->GetTransform().SetOrientation(Quaternion(Matrix3::Rotation(-acos(Vector3::Dot(Vector3(0, 1, 0), velocityDir)) * 180.0f / 3.14f, Vector3::Cross(velocityDir, Vector3(0, 1, 0)).Normalised())));
  
	lifeSpan -= dt;
	if (lifeSpan < 0.0f)
	{
		GameWorld::RemoveGameObject(this, true);
	}
}

void Projectile::OnCollisionBegin(GameObject* otherObject, Vector3 localA, Vector3 localB, Vector3 normal) {
#ifndef _ORBIS
	int soundToPlay = rand() % 2;
	NCL::AudioManager::GetInstance()->StartPlayingSound(Assets::AUDIODIR + (soundToPlay == 0 ? "splat_neutral_01.ogg" : "splat_neutral_02.ogg"), this->GetTransform().GetPosition());
#endif

	Ray ray(this->GetTransform().GetPosition() - this->GetPhysicsObject()->GetLinearVelocity().Normalised(), this->GetPhysicsObject()->GetLinearVelocity());
	ray.SetCollisionLayers(CollisionLayer::LAYER_ONE | CollisionLayer::LAYER_THREE);

	RayCollision closestCollision;
	if (GameWorld::Raycast(ray, closestCollision, true)) {
		RenderObject* test = ((GameObject*)closestCollision.node)->GetRenderObject();
		
		if (test) {
			if (test->GetPaintMask() != nullptr) {

				Vector2 texUV_a, texUV_b, texUV_c;
				Vector3 collisionPoint;
				Vector3 barycentric;
				CollisionDetection::GetBarycentricFromRay(ray, *test, texUV_a, texUV_b, texUV_c, barycentric, collisionPoint);

				Vector4 colour;
				if (GetOwnerPlayerID() % 2 == 0) {
					colour = Vector4(0.3, 0, 0.5, 1);
				}
				else {
					colour = Vector4(0.250, 0.878, 0.815, 1);
				}
				
				// Get the uv from the ray
				//renderInst->Paint(test, barycentric, collisionPoint, texUV_a, texUV_b, texUV_c, ((GameObject*)closestCollision.node)->GetPaintRadius(), 0.7, 1, colour);

				float randRad = ((GameObject*)closestCollision.node)->GetPaintRadius() + (((static_cast <float> (rand()) / static_cast <float> (RAND_MAX)) * 2.0f) - 1.0f) * ((GameObject*)closestCollision.node)->GetPaintRadius() * 0.25f;
				
				//uncomment for rainbow gun lmao
				renderInst->Paint(test, barycentric, collisionPoint, texUV_a, texUV_b, texUV_c, randRad, 0.7, 1, Vector4(static_cast <float> (rand()) / static_cast <float> (RAND_MAX) , static_cast <float> (rand()) / static_cast <float> (RAND_MAX), static_cast <float> (rand()) / static_cast <float> (RAND_MAX), 1));
			}
		}
	}

	GameWorld::RemoveGameObject(this, true);
}