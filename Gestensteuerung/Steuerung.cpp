#include <opencv2/opencv.hpp>
#include <vector>
#include "Steuerung.h"

using namespace cv;
using namespace std;

//Konstruktor
Steuerung::Steuerung() 
	: xPosition(180.0)
	, xPositionMax(340.0)
	, xPositionPrev(0.0)
{}

//Destruktor (gibt Ressourcen wieder frei)
Steuerung::~Steuerung(){

}

bool Steuerung::initialize(){
	videoCapture.open(0); //Default-Kamera �ffnen

	if (videoCapture.isOpened()){
		frameWidth = videoCapture.get(CV_CAP_PROP_FRAME_WIDTH); //Bildbreite des Webcam-Frames auslesen
		frameHeight = videoCapture.get(CV_CAP_PROP_FRAME_HEIGHT); //Bildh�he des Webcam-Frames auslesen

		namedWindow("Originalvideo");
		return true;
	} else {
		return false;
	}
}

float Steuerung::getXPosition(){
	//Breite Spielfeld: 400px, Breite Videocapture: 640 px, Verh�ltnis:  1 : 1,6
	xPosition /= 1.6;
	if(xPosition <= 0){ //Minimale xPosition der Biene
		xPosition = 0;
	} else if(xPosition >= xPositionMax ){ //Maximale xPosition der Biene (Breite Spielfeld - Breite Biene = 350)
		xPosition = xPositionMax;
	}
	return xPosition;
}


//Gibt den "Mittelpunkt" der wei�en Pixel im Bin�rbild als Point-Objekt zur�ck.
//Wenn keine wei�en Pixel vorhanden sind, wird (-1,-1) zur�ckgegeben.
Point Steuerung::centroidOfWhitePixels(const cv::Mat& image){
	int sumx = 0;//Summe aller x-Koordinaten der wei�en Pixel
    int sumy = 0;//Summe aller y-Koordinaten der wei�en Pixel
	int count = 0;//Anzahl wei�e Pixel
    for(int x = 0; x < image.cols; x++){
        for (int y = 0; y < image.rows; y++){
			//Wenn das betrachtete Pixel wei� ist, addiere seine x- bzw. y-Koordinaten zu sumx bzw. sumy und z�hle count hoch.
			if (image.at<uchar>(y,x) == 255){
				sumx += x;
				sumy += y;
				count++;
			}
		}
	}
    if (count > 0){
		return Point(sumx/count, sumy/count);
    }
    else {
		return Point(-1,-1);
    }
}

void Steuerung::eliminateFlawedAreas(cv::Mat videoFrameBin){
		//Alle wei�en Fl�chen im Bin�rbild bestimmen und in einem Vector speichern.
		//Alle Areas bis auf die gr��te (maxArea) schwarz einf�rben
		
		//Es muss eine Kopie der Bin�rmaske erstellt werden, da findContours() das untersuchte Bild zerst�rt
		Mat copyOfVideoFrameBin(frameWidth, frameHeight, CV_8UC1);
		//Im Vektor contours werden alle Konturen gespeichert
		vector<vector<Point>> contours;
		
		videoFrameBin.copyTo(copyOfVideoFrameBin);
		//Konturen finden und in contours speichern
		findContours(copyOfVideoFrameBin, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);
		
		vector<int> areaSizes; //Vektor mit allen Areagr��en
		int maxArea = 0;
		for(int i = 0; i < contours.size(); i++){
			vector<Point> contour = contours[i];
			double area = contourArea(contour); //K�nnte es hier und in Zeile 98 zu Problemen kommen wegen typecast bzw Ungenauigkeit von double?
			//Pr�fen, ob Area gr��er als ein vorgegebener Wert, da sonst auch kleinste Reflektionen das Spiel beeinflussen k�nnen
			cout << "Area: " << area << endl;
			//if(area > 5){
				if(area > maxArea){
					maxArea = area;
				}
				areaSizes.push_back(area);//area ans Ende von areaSizes hinzuf�gen
			//}
		}

		//Jetzt alle Areas < maxArea aus areaSizes schwarz f�rben
		if(areaSizes.size() >= 2){ //gibt es mehr als eine Areagr��e in areaSizes?
			for(int j = 0; j < areaSizes.size(); j++){
				if(areaSizes[j] < maxArea){
					drawContours(videoFrameBin, contours, j, Scalar(0,0,0), CV_FILLED);
				}
			}
		}

		//imshow("BinaerCopy", copyOfVideoFrameBin); 
}

void Steuerung::drawGreenCross(Mat videoFrame, Point centroid){
	//gr�ne Linie vertikal in Originalvideo zeichnen (mit centroid als Mittlepunkt)
	Point startPunktVert(centroid.x, centroid.y-10);
	Point endPunktVert(centroid.x, centroid.y+10);
	line(videoFrame, startPunktVert, endPunktVert, Scalar(0,255,0), 2);

	//gr�ne Linie horizontal in Originalvideo zeichnen
	Point startPunktHor(centroid.x-10, centroid.y);
	Point endPunktHor(centroid.x+10, centroid.y);
	line(videoFrame, startPunktHor, endPunktHor, Scalar(0,255,0), 2);
}

boolean Steuerung::process(){ 
		Mat videoFrame; //Originalvideo, in dem der Spieler zu sehen ist
		Mat videoFrameBin; //Bin�rmaske

		if (videoCapture.read(videoFrame) == false){ 
			return false;
		} 

		videoFrameBin = Mat(frameWidth, frameHeight, CV_8UC1);  //Bin�rmaske

		flip(videoFrame,videoFrame,1); //Spiegelt den Frame an der X-Achse (letzter Parameter = 1 bedeutet X-Achsenspiegelung)

		Scalar white(255,255,255);
		inRange(videoFrame, white, white, videoFrameBin); //Bin�rmaske vom Originalvideo erzeugen, in der nur wei�e Pixel als wei� dargestellt werden

		//Jetzt Opening (reduziert die wei�e Fl�che) zur Behebung von Pixelfehlern
		Mat binaryMaskOpened(frameWidth, frameHeight, CV_8UC1);
		erode(videoFrameBin, binaryMaskOpened, MORPH_RECT);
		dilate(binaryMaskOpened, videoFrameBin, MORPH_RECT);

		//Durch den Median Blur werden kleinere (wei�) reflektierende Gebiete im Bild eliminiert
		medianBlur(videoFrameBin, videoFrameBin, 3);

		//Alle wei�en Fl�chen im Bin�rbild bestimmen und in einem Vektor speichern.
		//Alle Areas bis auf die gr��te schwarz einf�rben.
		eliminateFlawedAreas(videoFrameBin); 

		//Zentralen Punkt der wei�en Pixel finden:
		Point centroid = centroidOfWhitePixels(videoFrameBin);
		//XPositionPrev auf aktuelle XPosition setzen
		xPositionPrev = xPosition;
		xPosition = centroid.x;
		
		//Gr�nes Kreuz mit "centroid" als Mittelpunkt in Originalvideo zeichnen
		drawGreenCross(videoFrame, centroid);

		imshow("Originalvideo", videoFrame);
		//imshow("Binaer", videoFrameBin);

		return true;
}