
struct Simplex
{
	vec2 points[3];
	int numPoints;
};

Simplex GJK(function<vec2(vec2)> Support)
{
	Simplex simplex = {};
	
	simplex.points[0] = Support({0,1});
	simplex.numPoints = 1;
	
	vec2 direction = -normalize(simplex.points[0]);
	
	for(;;)
	{
		vec2 point = Support(direction);
		if(dot(point, direction) < 0)
			return {}; // No collission
		
		switch(simplex.numPoints)
		{
		case 1:
		{
			vec2 AB = point - simplex.points[0];
			vec2 A0 = -simplex.points[0];
			vec2 B0 = -point;
			if( dot(AB, A0) < 0 )
				assert(false); // això no hauria de passar mai
			else if(dot(-AB, B0) < 0)
			{
				simplex.points[0] = point;
				direction = normalize(B0);
			}
			else
			{
				simplex.points[1] = point;
				simplex.numPoints = 2;
				direction = normalize( -dot(AB.normalize, A0) );
			}
		}
			break;
		case 2:
		{
			vec2 p0 = simplex.points[0];
			vec2 p1 = simplex.points[1];
			vec2 p2 = point;
			float area = 0.5 *(-p1.y*p2.x + p0.y*(-p1.x + p2.x) + p0.x*(p1.y - p2.y) + p1.x*p2.y);
			
			float s = (p0.y*p1.x - p0.x*p2.y /*+ (p2.y - p0.y)*px + (p0.x - p2.x)*py*/) / (2*area);
			float t = (p0.x*p1.y - p0.y*p1.x /*+ (p2.y - p1.y)*px + (p1.x - p0.x)*py*/) / (2*area);
			float u = 1 - s - t;
			// TODO
			
			if(s >= 0 && s <= 1 && t >= 0 && t <= 1 && u >= 0)
			{
				simplex.points[2] = point;
				simplex.numPoints = 3;
				return simplex;
			}
		}
			break;
		}	
	}
}


// 2a part: EPA (Expanding Polytope Algorithm)

// 1 - fem llista de arestes
// 2 - busquem l'aresta més propera a l'orígen
// 3 - expandim en direcció a l'aresta (Support)
//     si el nou punt és més lluny que l'aresta
//       4 - retirem l'aresta i afegim 2 arestes noves amb el nou punt.
//		 goto 2
//     sino
//       la normal de la col·lisió serà la normal de l'aresta.
//       la profunditat serà la distància de l'aresta a l'orígen
//       el punt serà el Support(normal) del primer objecte
