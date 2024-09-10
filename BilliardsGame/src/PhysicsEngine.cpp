#include "PhysicsEngine.h"

PhysicsEngine::PhysicsEngine(Game& game)
	: mGameRef{game}
{
}

void PhysicsEngine::Update(float deltaTime)
{
	MoveBalls(deltaTime);
}

void PhysicsEngine::MoveBalls(float deltaTime)
{
	std::vector<std::unique_ptr<Ball>>& balls = mGameRef.GetEntityManager().GetBallVector();

	for (auto& ball : balls)
	{
		ball->updatePosition(deltaTime);
	}
}

bool PhysicsEngine::doBallsOverlap(const Ball& b1, const Ball& b2) const
{
	// Uses Vec2 distance function to get distance between b1 and b2
	double distance = b1.getPosition().distance( 
		b2.getPosition() 
	);

	// If distance is less than b1 + b2 radius, they intercept
	return (distance <= b1.getRadius() + b2.getRadius());
}

void PhysicsEngine::debugRandomizeBalls()
{

}

void PhysicsEngine::HandleCollisions(float deltaTime)
{
	Handle_BvTableRect(deltaTime);
	Handle_BvB(deltaTime);
}

void PhysicsEngine::Handle_BvTableRect(float deltaTime)
{
	std::vector<std::unique_ptr<Ball>>& balls = mGameRef.GetEntityManager().GetBallVector();
	auto& tableRect = mGameRef.GetEntityManager().getTable().getRect();
	
	for (auto& ball : balls)
	{
		double ballVelX = ball->getVelocity().getx();
		double ballVelY = ball->getVelocity().gety(); 

		if (ball->getPosition().gety() - ball->getRadius() > tableRect.getGlobalBounds().top)
		{
			ball->setVelocity(Vec2{ballVelX, ballVelY * -1});
		}
		if (ball->getPosition().gety() + ball->getRadius() > tableRect.getGlobalBounds().height)
		{
			ball->setVelocity(Vec2{ ballVelX, ballVelY * -1 });
		}
		if (ball->getPosition().getx() - ball->getRadius() > tableRect.getGlobalBounds().left)
		{
			ball->setVelocity(Vec2{ ballVelX * -1, ballVelY });
		}
		if (ball->getPosition().getx() + ball->getRadius() > tableRect.getGlobalBounds().width)
		{
			ball->setVelocity(Vec2{ ballVelX * -1, ballVelY });
		}
	}
}

void PhysicsEngine::Handle_BvB(float deltaTime)
{
	double totalVelocity = 0;
	std::vector<std::unique_ptr<Ball>>& balls = mGameRef.GetEntityManager().GetBallVector();

	for (size_t i{ 0 }; i < balls.size(); i++)
	{
		if (balls[i]->isVisible() == false) { continue;	}				// If Ball 1 is not visible, skip this iteration
		else { totalVelocity += balls[i]->getVelocity().magnitude();}	// To debug for energy conservation print this value at end

		for (size_t j{ i + 1 }; j < balls.size(); j++)
		{
			if (balls[j]->isVisible() == false) { continue;}			// if Ball 2 is not visible, skip this iteration
			if (i == j) { continue; }									// Most likely redundant safeguard
			if (!doBallsOverlap(*balls[i], *balls[j])) { continue; }	// If balls don't overlap, no need to do anything

			BvB_ResolvePosition(*balls[i], *balls[j]);					// Resolve the positional overlap
			BvB_ResolveVelocity(*balls[i], *balls[j]);					// Calculate the new velocities
		}
	}
	// Uncomment to print the total velocity in the system
	//std::cout << "Total Velocity: " << totalVelocity << '\n';
}

void PhysicsEngine::BvB_ResolvePosition(Ball& b1, Ball& b2)
{
	double b1Radius = b1.getRadius();
	double b2Radius = b2.getRadius();

	Vec2 b1Pos = b1.getPosition();
	Vec2 b2Pos = b2.getPosition();

	double b1mass = b1.getMass();
	double b2mass = b2.getMass();

	// What is the distance between the center of the two balls
	double distance = b1Pos.distance(b2Pos);

	// What is the distance they actually overlap, halved so it's the amount of overlap for each ball
	double overlap = 0.5 * (distance - b1Radius - b2Radius);

	// Resolve the position of Ball 1
	double xpos_offset = overlap * (b1Pos.getx() - b2Pos.getx()) / distance;
	double ypos_offset = overlap * (b1Pos.getx() - b2Pos.gety()) / distance;
	b1.setPosition({ b1Pos.getx() - xpos_offset , b1Pos.gety() - ypos_offset });

	// Resolve the position of Ball2
	b2.setPosition({ b2Pos.getx() + xpos_offset, b2Pos.getx() + ypos_offset });
}

void PhysicsEngine::BvB_ResolveVelocity(Ball& b1, Ball& b2)
{
	double b1Radius = b1.getRadius();
	double b2Radius = b2.getRadius();

	Vec2 b1Pos = b1.getPosition();
	Vec2 b2Pos = b2.getPosition();

	Vec2 b1Vel = b1.getVelocity();
	Vec2 b2Vel = b2.getVelocity();

	double b1mass = b1.getMass();
	double b2mass = b2.getMass();

	// # Handle the changes in velocity
	Vec2 normal = b1Vel.normalVectorTo(b2Vel);		// Normal vector vel1 | vel2
	Vec2 tangent = normal.getTangent();				// Tangent of normal Vector:

	double dotProdTan1 = b1Vel.dotProduct(tangent);	// Dot product vel1 | tangent
	double dotProdTan2 = b2Vel.dotProduct(tangent);	// Dot product vel2 | tangent

	double dotProdNorm1 = b1Vel.dotProduct(normal);	// Dot product vel1 | normal
	double dotProdNorm2 = b2Vel.dotProduct(normal);	// Dot Product vel2 | normal

	// Conservation of momentum - This might be irrelevant now because poolballs all have the same mass
	double m1 = (dotProdNorm1 * (b1mass - b2mass) + 2.0 * b2mass * dotProdNorm2) / (b1mass + b2mass);
	double m2 = (dotProdNorm2 * (b2mass - b1mass) + 2.0 * b1mass * dotProdNorm1) / (b1mass + b2mass);

	// New Velocities
	Vec2 b1NewVel{
		{tangent.getx() * dotProdTan1 + normal.getx() * m1},	// New X velocity of ball 1
		{tangent.gety() * dotProdTan1 + normal.gety() * m1}		// New Y Velocity of ball 1
	};
	Vec2 b2NewVel{
		{tangent.getx() * dotProdTan2 + normal.getx() * m2 },	// New X velocity of ball 2
		{tangent.gety() * dotProdTan2 + normal.gety() * m2 }	// New Y Velocity of ball 2
	};

	b1.setVelocity(b1NewVel);
	b2.setVelocity(b2NewVel);
}

bool PhysicsEngine::AreBallsAtRest()
{
	bool atRest = true;
	std::vector<std::unique_ptr<Ball>>& balls = mGameRef.GetEntityManager().GetBallVector();
	for (auto& ball : balls)
	{
		if (ball->getVelocity().getx() != 0)
		{
			atRest = false;
		}
		if (ball->getVelocity().gety() != 0)
		{
			atRest = false;
		}
	}

	return atRest;
}


