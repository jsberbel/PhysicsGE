
// main
input.windowHalfSize = { 960, 540 };
glm::mat4 projection = glm::ortho(-(float)input.windowHalfSize.x, (float)input.windowHalfSize.x, -(float)input.windowHalfSize.y, (float)input.windowHalfSize.y, -5.0f, 5.0f);



// update
gameData.prevBalls = std::move(gameData.balls);
gameData.balls.clear(); // std::vector

// update balls
for (const auto& ball : gameData.prevBalls)
{
	glm::dvec2 f = {};
	Ball ballNext = {};
	ballNext.pos = ball.pos + ball.vel * input.dt + f * (0.5 * input.dt * input.dt);
	ballNext.vel = ball.vel + f * input.dt;

	if (ballNext.pos.x > input.windowHalfSize.x || ballNext.pos.x < -input.windowHalfSize.x)
		ballNext.vel.x = -ballNext.vel.x;
	if (ballNext.pos.y > input.windowHalfSize.y || ballNext.pos.y < -input.windowHalfSize.y)
		ballNext.vel.y = -ballNext.vel.y;

	gameData.balls.push_back(ballNext);
}


// N1: un objecte manté la seva velocitat si cap força actua sobre seu
// N2: una força actuant sobre un objecte provoca una acceleració proporcional a la massa (f = m * a)
// a = f / m

// velocitat = derivada(posició)
// acceleració = derivada(velocitat)

// principi d'Alembert ->  f = sum(f[i])

struct ContactData
{
	glm::dvec2 point, normal;
	double penetatrion;
	// --
	double restitution, friction;
};

struct Line
{
	glm::dvec2 normal;
	double distance;
};
// plane x = 960 -> normal = {-1, 0}, distance = -960;

// distance pont - plane ->  point · plane.normal - plane.distance;

struct Circle
{
	glm::dvec2 position;
	double radius;
}


// resoldre velocitats del xoc


// velocitat d'acostament = a.vel · (b.pos - a.pos) + b.vel · (a.pos - b.pos)
// velocitat d'acostament = -(a.vel - b.vel) · (a.pos - b.pos)
// velocitat de separació (vs) = (a.vel - b.vel) · (a.pos - b.pos)

// conservació del moment
// a.mass * a.velocitat + b.mass * b.velocitat == a.mass * a'.velocitat + b.mass * b'.velocitat
// vs' = -c * vs
// c -> coeficient de restitució (0 - 1)




// impuls = massa * velocitat (Newton * segon)
// v' = v + invmass * sum(impulsos[i])

// resum:
// - calcular velocitat de separació (Vs)
// - si Vs > 0
//   - novaVs = -c * Vs
//   - totalInvMass = invMass[0] + invMass[1]
//   - deltaV = novaVs - Vs
//   - impuls = deltaV / totalInvMass
//   - impulsPerIMass = contactNormal * impuls
//   - novaVelA = velA + impulsPerIMass * invMassA
//   - novaVelB = velB - impulsPerIMass * invMassB


// resoldre interpendetració

// desplaçament A + desplaçament B == interpendetració (al llarg de la normal)
// massaA * deltaA == massaB * deltaB

// deltaA =   contactNormal * interpendetració * massaB / (massaA + massaB)
// deltaB = - contactNormal * interpendetració * massaA / (massaA + massaB)


// resum:
// - si interpendetració > 0
//   - totalInvMass = invMass[0] + invMass[1]
//   - movePerIMass = contactNormal * (interpendetració / totalInvMass)
//   - movimentA =   movePerIMass * invMassA
//   - movimentB = - movePerIMass * invMassB



// nota: movePerIMass = 1 / massA + 1 / massB = massB / (massA * massB) + massA / (massA * massB) = massA + massB / (massA * massB)
