/*****************************************************************
*	File...:	camDemo.cpp
*	Purpose:	video processing
*	Date...:	30.09.2019
*	Changes:	16.10.2024 mouse events
*
*********************************************************************/

#include "camDemo.h"
#include <cmath>
#include <vector>
#include <algorithm>


//Struktur für einzelne Wellen
struct Wave{
	int wx; //Koordinaten der Welle
	int wy; 
	double startTime; //Startzeitpunkt der Welle
};

vector<Wave> waves; //Liste der aktuell aktiven Wellen


/* --------------------------------------------------------------
 * mouse_event()
 * openCV-Funktion um MouseEvents auszuwerten  
 *----------------------------------------------------------------*/
void mouse_event( int evt, int x, int y, int, void* param)
{
	MouseParams* mp = (MouseParams*) param;
	mp->evt = evt; //Mouse-Event
	mp->mouse_pos.x = x; //Mouse x-Position
	mp->mouse_pos.y = y; //Mouse y-Position
}

/* --------------------------------------------------------------
 * click_in_rect()
 * Wurde in einen bestimmten Bereich mit links geklickt?
 *----------------------------------------------------------------*/
void click_in_rect(MouseParams mp, Rect rect)
{
	if (mp.evt == EVENT_LBUTTONDOWN)
	{
		if (mp.mouse_pos.x >= rect.x &&
				mp.mouse_pos.y >= rect.y &&
				mp.mouse_pos.x <= rect.x + rect.width &&
				mp.mouse_pos.y <= rect.y + rect.height)
		{
			
		}
	}
}

/* --------------------------------------------------------------
 * mouse_in_rect()
 * Wurde Mauszeiger in einen bestimmten Bereich bewegt?
 *----------------------------------------------------------------*/
bool mouse_in_rect(MouseParams mp, Rect rect)
{
	if (mp.evt == EVENT_MOUSEMOVE)
	{
		if (mp.mouse_pos.x >= rect.x &&
				mp.mouse_pos.y >= rect.y &&
				mp.mouse_pos.x <= rect.x + rect.width &&
				mp.mouse_pos.y <= rect.y + rect.height)
		{
			return true;
		}
	}
	return false;
}

Mat applyWaveDistortion(const Mat& frame, const vector<Wave>& waves,
	double currentTime, double amplitude, double wavelength,
	double speed, double damping, double alpha, double maxWaveTime) {

	Mat waveFrame = frame.clone();
	int height = frame.rows;
	int width = frame.cols;
	int channels = frame.channels();
	int stride = width * channels;
	//...
	
	// Wellen berechnen
	for (int y = height/2; y < height - 60; y++) {
		for (int x = 0; x < width; x++) {

			double totalWaveShift = 0.0; // Summe der Verschiebungen

			// Berechne die Gesamtverschiebung durch alle Wellen
			for (const auto& wave : waves) {
				double elapsedTime = currentTime - wave.startTime;
				double timeDamping = exp(-alpha * elapsedTime);
				double rsqrt = (x - wave.wx) * (x - wave.wx) + (y - wave.wy) * (y - wave.wy);
				double r = sqrt(rsqrt);
				double waveShift = amplitude * timeDamping * sin(2.0 * CV_PI * r / wavelength - speed * elapsedTime) / (1.0 + damping * r);
				totalWaveShift += waveShift;  

			}

			// Berechne die neue Position mit der Gesamtverschiebung
			for(const auto& wave : waves){	
				double rsqrt = (x - wave.wx) * (x - wave.wx) + (y - wave.wy) * (y - wave.wy);
				double r = sqrt(rsqrt);
				int newX = static_cast<int>(x + totalWaveShift * (x - wave.wx) / r);
				int newY = static_cast<int>(y + totalWaveShift * (y - wave.wy) / r);

				// Überprüfen, ob die neuen Koordinaten innerhalb des Bildes liegen
				if (newX >= 0 && newX < width && newY >= 0 && newY < height - 60) {
					int pos = x * channels + y * stride;
					int newPos = newX * channels + newY * stride;
					for (int c = 0; c < channels; c++) {
						waveFrame.data[pos + c] = frame.data[newPos + c];
					}
				}
			}
			
		}
	}
	return waveFrame;
}
 
/*---------------------------------------------------------------
* main()
*---------------------------------------------------------------*/
int main( int, char**)
{
	MouseParams mp; // Le-Wi: Zur Auswertung von Mouse-Events
	Scalar colour;
	Mat	cam_img; //eingelesenes Kamerabild
	char *windowGameOutput = "camDemo"; // Name of window
	unsigned int width, height, channels, stride;	// Werte des angezeigten Bildes
	int 
		key = 0,	// Tastatureingabe
		frames = 0, //frames zählen für FPS-Anzeige
		fps = 0;	//frames pro Sekunde
	int camNum = 2;
	bool fullscreen_flag = false; //Ist fullscreen aktivert oder nicht?
	bool mirror_flag = false; //Mirror-Flag gibt an, ob Bild gespiegelt ist oder nicht
	bool water_color = false; //Water-Color-Flag gibt an, ob Wasserfarben verwendet werden
	bool strand_flag = false; //Strand-Flag gibt an, ob Strandwellen verwendet werden
	bool freeze_flag = false; //Freeze-Flag
	DemoState state; //Aktueller Zustand des Spiels

#if defined _DEBUG || defined LOGGING
		FILE *log = NULL;
		log = fopen( "log_debug.txt", "wt");
#endif

	clock_t start_time, finish_time;

	VideoCapture cap;
	do 
	{
		camNum--; /* try next camera */
		cap.open( camNum );
	} while (camNum> 0 && !cap.isOpened()) ; /* solange noch andere Kameras verfügbar sein könnten */

	if (!cap.isOpened())	// ist die Kamera nicht aktiv?
	{
		AllocConsole();
		printf( "Keine Kamera gefunden!\n");
		printf( "Zum Beenden 'Enter' druecken\n");
#if defined _DEBUG || defined LOGGING
		fprintf( log, "Keine Kamera gefunden!\n");
		fclose( log);
#endif
		while (getchar() == NULL);
		return -1;
	}
	else
	{
		//Infos auf der Konsole
		printf("==> Program Control <==\n");
		printf("==                   ==\n");
		printf("* Start Screen\n");
		printf(" - 'ESC' stop the program \n");
		printf(" - 'f'   toggle fullscreen\n");
		printf(" - 'g'   toggle freeze\n");
		printf(" - 'p'   toggle effects\n");
		printf(" - 'Spacebar' in Water to add Wave\n");
	}
	{
		HWND console = GetConsoleWindow();
		RECT r;

		GetWindowRect( console, &r); //stores the console's current dimensions

		//MoveWindow(window_handle, x, y, width, height, redraw_window);
		MoveWindow( console, r.left, r.top, 800, 600, TRUE);
		//MoveWindow( console, 10, 0, 850, 800, TRUE);
	}

#ifndef _DEBUG
	FreeConsole(); //Konsole ausschalten
#endif
	
	/* capture the image */
	cap >> cam_img;	

	/* get format of camera image	*/
	width = cam_img.cols;
	height = cam_img.rows;
	channels = cam_img.channels();
	stride = width * channels;
	
	//Handle für das Fenster vorbereiten
	namedWindow( windowGameOutput, WINDOW_NORMAL|CV_GUI_EXPANDED); //Erlauben des Maximierens
	resizeWindow( windowGameOutput, width, height); //Start Auflösung der Kamera
	HWND cvHwnd = (HWND )cvGetWindowHandle( windowGameOutput); //window-handle to detect window-states

	srand( (unsigned) time(NULL));//seeds the random number generator	

	start_time = clock();

	state = START_SCREEN;

	// Setup zum Auswerten von Mausevents
	setMouseCallback( windowGameOutput, mouse_event, (void*)&mp);
	
	//Wellenparameter:
	double amplitude = 100.0;	// Maximale Amplitude
	double wavelength = 50.0;	// Wellenlänge
	double speed = 10.0;		// Geschwindigkeit der Welle
	double maxWaveTime = 5.0;	// lebensdauer einer Welle
	double damping = 0.1;		// räumliche Dämpfung
	double alpha = 0.5;			// Parameter zur Berechnung der zeitlichen Dämpfung

	double globalTime = 0.0;

	/*-------------------- main loop ---------------*/
	while (state != DEMO_STOP)
	{
		cap >> cam_img;

		//Zeit aktualisieren
		if(!freeze_flag) globalTime += 0.1;

		//Spiegelung
		if (mirror_flag) {
			// horizontale Spiegelung der oberen Bildhälfte
			for (int y = height / 2; y < height-60; y++) {
				for (int x = 0; x < width; x++) {
					int pos = x * channels + y * stride;
					int newPos = x * channels + (height - y) * stride;

					for (int c = 0; c < channels; c++) {
						cam_img.data[pos + c] = cam_img.data[newPos + c];
					}
				}
			}
		}

		//Wasserfarben in der unteren Bildhälfte
		if (water_color) {
			//BGR Farbkanäle werden in RGB Farbkanäle konvertiert
			for (int y = height / 2; y < height-60; y++) {
				for (int x = 0; x < width; x++) {
					int pos = x * channels + y * stride;

					//Swap channels
					int tmp = cam_img.data[pos];
					cam_img.data[pos] = cam_img.data[pos + 2];
					cam_img.data[pos + 2] = tmp;
				}
			}
		}

		//Strand-Wellen
		if (strand_flag) {
			for (int y = height / 2; y < height-60; y++) {
				for (int x = 0; x < width; x++) {
					// Strand-Welle berechnen
					double wave = amplitude * sin(2.0 * CV_PI * y / wavelength - globalTime) / (1.0 + damping * y);

					//Farbwerte setzen
					int pos = x * channels + y * stride;
					for (int c = 0; c < channels; c++) {
						cam_img.data[pos + c] = saturate_cast<uchar>(cam_img.data[pos + c] + wave);
					}
				}
			}
		}
		

		/* determination of frames per second	*/
		frames++;
		finish_time = clock();
		if ((double)(finish_time - start_time) / CLOCKS_PER_SEC >= 1.0)
		{
			fps = frames;
			frames = 0;
			start_time = clock();
		}

#define FPS_OUTPUT
#if defined FPS_OUTPUT || defined _DEBUG
		//FPS-Ausgabe oben rechts
		char fps_char[10];
		sprintf ( fps_char, "FPS:      %d", fps);
		const string& fps_string = (string) fps_char;
		putText( cam_img, fps_string, Point( width - 140, 15), FONT_HERSHEY_SIMPLEX, 
			0.5 /*fontScale*/, Scalar( 0, 255, 255), 2);
#endif
		//Info-Ausgabe
		char info_char[50];

		//Anzahl Wellen-Ausgabe
		int waveCount = waves.size();
		sprintf(info_char, "WaveCount: %.d", waveCount);
		putText(cam_img, (string)info_char, Point(width - 140, 30), FONT_HERSHEY_SIMPLEX,
			0.5 /*fontScale*/, Scalar(0, 255, 255), 2);


		//Amplitude-Ausgabe
		sprintf(info_char, "Amplitude(s/w): %.0f", amplitude);
		putText(cam_img, (string)info_char, Point(40, 15), FONT_HERSHEY_SIMPLEX,
			0.5 /*fontScale*/, Scalar(0, 255, 255), 2);

		//Wavelength-Ausgabe
		sprintf(info_char, "Wellenlaenge(a/d): %.0f", wavelength);
		putText(cam_img, (string)info_char, Point(40, 30), FONT_HERSHEY_SIMPLEX,
			0.5 /*fontScale*/, Scalar(0, 255, 255), 2);

		//Speed-Ausgabe
		sprintf(info_char, "Geschwindigkeit(-/+): %.0f", speed);
		putText(cam_img, (string)info_char, Point(40, 45), FONT_HERSHEY_SIMPLEX,
			0.5 /*fontScale*/, Scalar(0, 255, 255), 2);

		//MaxWaveTime-Ausgabe
		sprintf(info_char, "Wellendauer(1/2): %.0f", maxWaveTime);
		putText(cam_img, (string)info_char, Point(250, 15), FONT_HERSHEY_SIMPLEX,
			0.5 /*fontScale*/, Scalar(0, 255, 255), 2);

		//Dämpfung
		sprintf(info_char, "Daempfung(3/4): %.2f", alpha);
		putText(cam_img, (string)info_char, Point(250, 30), FONT_HERSHEY_SIMPLEX,
			0.5 /*fontScale*/, Scalar(0, 255, 255), 2);
		

		/* input from keyboard */
		key = tolower( waitKey(1)); /* Strutz  convert to lower case */
		// Vollbildschirm ein- bzw. ausschalten
		if (key == 'f')
		{
			if (!fullscreen_flag)
			{
				//skaliere fenster auf vollbild
				cvSetWindowProperty( windowGameOutput, WND_PROP_FULLSCREEN, WINDOW_FULLSCREEN);
				fullscreen_flag = true;
			}
			else
			{
				//setzte fenster auf original-größe
				cvSetWindowProperty( windowGameOutput, WINDOW_NORMAL, WINDOW_NORMAL);	
				fullscreen_flag = false;
			}
		}

		if (key == 'p') { // toggle all Effects
			mirror_flag = 1 - mirror_flag;
			water_color = 1 - water_color;
			strand_flag = 1 - strand_flag;
		}
		if (key == 'g') { // toggle Freeze
			freeze_flag = 1 - freeze_flag;
		}

		if (key == 'w') {  // Amplitude erhöhen
			if(amplitude < 255.0) amplitude += 5.0;
		}
		if (key == 's') {  // Amplitude verringern
			if(amplitude > 5.0) amplitude -= 5.0;
		}
		if (key == 'd') {  // Wellenlänge erhöhen
			if(wavelength < 100.0) wavelength += 5.0;
		}
		if (key == 'a') {  // Wellenlänge verringern
			if (wavelength > 5.0) wavelength -= 5.0;
		}
		if (key == '+') {  // Speed erhöhen
			if (speed < 20.0) speed += 1.0;
		}
		if (key == '-') {  // Speed verringern
			if (speed > 1.0) speed -= 1.0;
		}
		if (key == '2') {  // Wellendauer erhöhen
			if (maxWaveTime < 10.0) maxWaveTime += 1.0;
		}
		if (key == '1') {  // Wellendauer verringern
			if (maxWaveTime > 1.0) maxWaveTime -= 1.0;
		}
		if (key == '4') {  // Dämpfung erhöhen
			if (alpha < 1.0) alpha += 0.05;
		}
		if (key == '3') {  // Dämpfung verringern
			if (alpha > 0.01) alpha -= 0.05;
		}
		

		if (state == START_SCREEN)
		{
			if (key == 27 ) // Abbruch mit ESC
			{
				state = DEMO_STOP; /* leave loop	*/
				continue;
			}
			else if (key == 'p')
			{
				/* show properties of camera	*/
				if (cap.set(CAP_PROP_SETTINGS, 0) != 1 )
				{
#if defined _DEBUG || defined LOGGING
					fprintf( log, "\nlocal webcam > Webcam Settings cannot be opened!\n" );
#endif
				}
			}
		}

		//Mouse-in-Rechteck-Event
		Rect mouseRect(0, height / 2, width, ((height / 2) - 60));
		if (mouse_in_rect(mp, mouseRect)) {
			//Welle hinzufügen wenn LeerTaste gedrückt wird
			if (key == ' ') {
				waves.push_back({ mp.mouse_pos.x, mp.mouse_pos.y, globalTime });
			}
		}


		Mat waveFrame = applyWaveDistortion(cam_img, waves, globalTime, amplitude, wavelength, speed, damping, alpha, maxWaveTime);
		

		/********************************************************************************************/
		/* show window with live video	*/		//Le-Wi: Funktionalitäten zum Schließen (x-Button)
		if (!IsWindowVisible( cvHwnd)) 
		{
			break;
		}

		imshow(windowGameOutput, waveFrame); //Ausgabefenster darstellen	

		//abgelaufene Wellen aus Vektor entfernen mit Lamda-Ausdruck
		waves.erase(remove_if(waves.begin(), waves.end(), [globalTime, maxWaveTime](const Wave& wave) {
			return globalTime - wave.startTime > maxWaveTime;

		}), waves.end());

		waveFrame.release();

	}	// Ende der Endlos-Schleife

	//Freigabe aller Matrizen
	if (cap.isOpened()) cap.release(); //Freigabe der Kamera
	if (cam_img.data) cam_img.release();


#if defined _DEBUG || defined LOGGING
	fclose( log );
#endif
	//FreeConsole(); //Konsole ausschalten
	cvDestroyAllWindows();
	//_CrtDumpMemoryLeaks();
	exit( 0);
}