#ifndef BEE_H
#define BEE_H
#include <string>
#include "Obstacle.h"
#include "Entity.h"

class Bee : public Entity{
	public:
		Bee(int posx, int posy, int maximumX, int maximunY, std::string name);
		void addPoints(int p);
		void collidesWith(Obstacle &ob);
	private:
		int points;
};
#endif