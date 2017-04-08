

// Estructura "update" físic:

// 1 - Update posicions / velocitats
// 2 - Generació de colisions
//   2.1 - Broad-Phase
//   2.2 - Fine-Grained
//   2.3 - Sort / Generació de grups de col·lisions
// 3 - Resolució de colisions
//   3.1 - Per cada grup 
//       3.1.1 - Trobar la colisió més profunda
//       3.1.2 - Resoldre velocitat
//       3.1.3 - Resoldre posició
//       3.1.4 - Recalcular colisions del grup
//       3.1.5 - Tornar a 3.1.1 (amb un màxim d'iteracions)


// Broad-Phase: "Sort & Sweep"
//   1 - Trobar els extrems de cada objecte respecte a un eix. (p.e. les la x min i max)
//   2 - Crear una llista ordenada amb cada extrem anotat ( o1min , o1max, o2min, o3min, o2max, o3max )
//   3 - Des de cada "min" anotar tots els "min" que es trobin abans de trobar el "max" corresponent a aquest objecte.
//        aquests son les possibles colisions.

struct GameObject
{
	vec2 GetExtreme(vec2);
};

struct PossibleCollission
{
	GameObject *a, *b;
}

std::vector<PossibleCollission> SortAndSweep(const std::vector<const GameObject*>& gameObjects)
{
	struct Extreme
	{
		GameObject *o;
		float p;
		bool min;
	};
	
	std::vector<Extreme> list;
	
	for(const GameObject* go : gameObjects)
	{
		list.push_back( { go, dot(go->GetExtreme({-1,0}), {1, 0}), true  } );
		list.push_back( { go, dot(go->GetExtreme({+1,0}), {1, 0}), false } );
	}
	
	std::sort(list); // TODO
	
	std::vector<PossibleCollission> result;
	
	for(int i = 0; i < list.size(); ++i)
	{
		if(list[i].min)
		{
			for(int j = i + 1; list[i].go != list[j].go; ++j)
			{
				if(list[j].min)
				{
					result.push_back( { list[i].go, list[j].go } );
				}
			}
		}
	}
	return result;
}

struct ContactData
{
	GameObject *a, *b;
	dvec2 point, normal;
	double penetatrion;
	// --
	double restitution, friction;
};


struct ContactGroup
{
	std::vector<ContactData> contacts;
};

std::vector<ContactGroup> GenerateContactGroups(std::vector<ContactData> contactData)
{
	// si ordenem podem "simplificar" la segona part.
	// sense probar-ho no podem saber si és més optim o no.
	
	//std::sort(contactData, [](const ContactData& a, const ContactData& b) // aquesta lambda serveix per ordenar i és equivalent a "a < b"
	//{
	//	// ens assegurem que el contacte estigui ben generat
	//	assert(a.a < a.b || a.b == nullptr);
	//	assert(b.a < b.b || b.b == nullptr); // contactes amb l'element "b" a null son contactes amb objectes de massa infinita (parets, pex)
	//	
	//	if(a.a < b.a)
	//		return true;
	//	else if(a.a > b.a)
	//		return false;
	//	else if(a.b < b.b)
	//		return true;
	//	else
	//		return false;
	//});
	
	
	std::vector<ContactGroup> result;
	std::unordered_map<GameObject*, ContactGroup*> createdGroups;
	
	for(int i = 0; i < contactData.size(); ++i)
	{
		auto it = createdGroups.find(contactData[i].a); // busquem si ja tenim alguna colisió amb l'objecte "a"
		if(it == createdGroups.end())
		{
			it = createdGroups.find(contactData[i].b); // busquem si ja tenim alguna colisió amb l'objecte "b"
			
			if(it == createdGroups.end())
			{
				result.push_back( {contactData[i]} ); // creem la llista de colisions nova
				createdGroups[contactData[i].a] = &result[result.size()]; // guardem referència a aquesta llista
				createdGroups[contactData[i].b] = &result[result.size()]; // per cada objecte per trobarla
			}
			else
			{
				it->second->contacts.push_back(contactData[i]); // afegim la colisió a la llista
				createdGroups[contactData[i].a] = it->second; // guardem referència de l'objecte que no hem trobat abans
			}
		}
		else
		{
			it->second->contacts.push_back(contactData[i]);
			createdGroups[contactData[i].b] = it->second;
		}
	}
	
	return result;
}

