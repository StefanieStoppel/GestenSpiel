#include <opencv2/opencv.hpp>
#include "ViewController.h"
#include "Obstacle.h"
#include "Bee.h"

using namespace cv;
using namespace std;

ViewController::ViewController()
	:bee(180, 530, 400, 600, "bee"),
	obstacles(30)
	{
	bg = imread("img/bg.png", 1);
	createFlowers();
	namedWindow( "Bienchen & Bl�mchen", 1 ); 
	}

ViewController::~ViewController(){};


void ViewController::createFlowers(){
	for (int i = 0; i < obstacles.size(); i++){
		int x = (rand() % (int)(bg.cols - 50 + 1)); // 50 = flower width
		int y = (rand() % (int)(bg.rows + 400 + 1));
		//min + (rand() % (int)(max - min + 1))
		obstacles[i] = Obstacle(x, -y, bg.cols, bg.rows, "flower", 5);
	}
}


void ViewController::moveBG(int delta){
	Mat temp; 
	bg.copyTo(temp);
	for (int x = 0; x < (bg.rows-delta); x++){
		temp.row(x).copyTo(bg.row(x+delta));
	}
	for (int j = 0; j < delta; j++){
		temp.row(bg.rows-delta+j).copyTo(bg.row(j));
	}
}

void ViewController::moveAndDrawFlowers(int delta){
	for (int i = 0; i < obstacles.size(); i++){
		obstacles[i].addToY(delta);
		if (obstacles[i].getCollable()){
			obstacles[i].insertInto(viewImage);
		}
	}
}

void ViewController::checkCollision(){
	for (int i = 0; i < obstacles.size(); i++){
		bee.collidesWith(obstacles[i]);
	}
}

void ViewController::draw(int delta){
	checkCollision();
	moveBG(delta);
	bg.copyTo(viewImage);
	moveAndDrawFlowers(delta);
	bee.insertInto(viewImage);
	imshow( "Bienchen & Bl�mchen", viewImage);
}