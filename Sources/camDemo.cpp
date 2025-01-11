/*****************************************************************
*	File...:	camDemo.cpp
*	Purpose:	video processing
*	Date...:	30.09.2019
*	Changes:	16.10.2024 mouse events
*
*********************************************************************/

#include "camDemo.h"


//Struktur für einzelne Wellen
struct Wave{
	int wx; //Koordinaten der Welle
	int wy; 
	double startTime; //Startzeitpunkt der Welle
};



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
 * click_left()
 *----------------------------------------------------------------*/
bool click_left(MouseParams mp, char* folder)
{
	if (mp.evt == EVENT_LBUTTONDOWN)
	{
		{
			return true;
		}
	}
	return false;
}

/* --------------------------------------------------------------
 * click_in_rect()
 * Wurde in einen bestimmten Bereich mit links geklickt?
 *----------------------------------------------------------------*/
bool click_in_rect(MouseParams mp, Rect rect, char* folder)
{
	if (mp.evt == EVENT_LBUTTONDOWN)
	{
		if (mp.mouse_pos.x >= rect.x &&
				mp.mouse_pos.y >= rect.y &&
				mp.mouse_pos.x <= rect.x + rect.width &&
				mp.mouse_pos.y <= rect.y + rect.height)
		{
			/*char path[512];
			sprintf_s(path, "%s/waterdrop.mp3", folder);
			PlaySoundA(path, NULL, SND_ASYNC);*/
			return true;
		}
	}
	return false;
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

/*---------------------------------------------------------------
* main()
*---------------------------------------------------------------*/
int main( int, char**)
{
			/* check memory usage	
	 *  see https://msdn.microsoft.com/de-de/library/x98tx3cf.aspx
	 */
	//_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
	//_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
	//_CrtSetBreakAlloc( 1358);

	/* 1015 steht hier für eine Speicherbelegungsnummer, welche 
	 * in {}-Klammern im Memory-Report ausgegeben wurde. Bei erneuter 
	 * Ausführung unterbricht das Programm dann an der Stelle, an dem 
	 * der jeweilige Speicher allokiert wurde.
	 */

	/* check for two different folders: correct folder depends on where executable is started
	* either from IDE or bin folder */
	const char* folder1 = "Resources";
	const char* folder2 = "../Resources";
	char folder[15], path[512];

	MouseParams mp; // Le-Wi: Zur Auswertung von Mouse-Events
	Scalar colour;
	Mat	cam_img; //eingelesenes Kamerabild
	Mat cam_img_grey; //Graustufenkamerabild für autom. Startgesichtfestlegung
	Mat strElement; //Strukturelement für Dilatations-Funktion
	Mat3b rgb_scale;	// leerer Bildkontainer für RGB-Werte
	char *windowGameOutput = "camDemo"; // Name of window
	//double	scale = 1.0;				// Skalierung der Berechnungsmatrizen 
	unsigned int width, height, channels, stride;	// Werte des angezeigten Bildes
	int 
		key = 0,	// Tastatureingabe
		frames = 0, //frames zählen für FPS-Anzeige
		fps = 0;	//frames pro Sekunde
	int camNum = 2;
	bool fullscreen_flag = false; //Ist fullscreen aktivert oder nicht?
	bool mirror_flag = false; //Mirror-Flag gibt an, ob Bild gespiegelt ist oder nicht
	bool water_color = false; //Water-Color-Flag gibt an, ob Wasserfarben verwendet werden
	bool water_effect = false;//Water-Effect-Flag gibt an, ob Wassereffekte verwendet werden
	bool radial_flag = false; //Radial-Flag gibt an, ob Radialwellen verwendet werden
	bool play_flag = false;   //Play-Flag um alle Effekte gleichzeitig zu aktivieren
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
		printf(" - 's'   toggle mirror img\n");
		printf(" - 'w'   toggle water color\n");
		printf(" - 'e'   toggle water effect\n");
		printf(" - 'r'   toggle radial wave\n");
		printf(" - 'p'   toggle all effects\n");
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

	/* find folder for ressources	*/
	{
		FILE* in = NULL;
		strcpy_s(folder, folder1);/* try first folder */
		sprintf_s(path, "%s/waterdrop.mp3", folder);
		in = fopen(path, "r");
		if (in == NULL)
		{
			strcpy_s(folder, folder2); /* try other folder */
			sprintf_s(path, "%s/waterdrop.mp3", folder);
			in = fopen(path, "r");
			if (in == NULL)
			{
				printf("Ressources cannot be found\n");
				exit(99);
			}
		}
		fclose(in);
	}

	start_time = clock();

	state = START_SCREEN;

	// Setup zum Auswerten von Mausevents
	setMouseCallback( windowGameOutput, mouse_event, (void*)&mp);

	//Vektoren für Wassereffekt:
	vector<unsigned char> smooth_img(width * height * channels);
	vector<unsigned char> edge_data(width * height * channels);
	
	//Wellenparameter:
	vector<Wave> waves; //Liste der aktuell aktiven Wellen

	const double amplitude = 255.0; // Maximale Amplitude
	const double wavelength = 50.0; // Wellenlänge
	const double speed = 10.0;      // Geschwindigkeit der Welle
	const double damping = 0.5;		// räumliche Dämpfung
	const double alpha = 1.0;		// Parameter zur Berechnung der zeitlichen Dämpfung
	const double MaxWaveTime = 2.0;	// lebensdauer einer Welle

	double globalTime = 0.0;
	double TimeDiv = 1.0 / 30.0;	

	/*-------------------- main loop ---------------*/
	while (state != DEMO_STOP)
	{
		// ein Bild aus dem Video betrachten und in cam_img speichern
		if (cap.read(cam_img) == false)
		{ //falls nicht möglich: Fehlermeldung
			destroyWindow("camDemo"); //Ausgabefenster ausblenden

			AllocConsole(); //Konsole wieder einschalten
			printf("Verbindung zur Kamera verloren!\n");
			printf("Zum Beenden 'Enter' druecken\n");
			cap.release(); //Freigabe der Kamera
#if defined _DEBUG || defined LOGGING
			fprintf(log, "Verbindung zur Kamera verloren!\n");
			fclose(log);
#endif
			while (getchar() == NULL); //Warten auf Eingabe
			break; //Beende die Endlosschleife
		}

		//Aktuelle Zeit berechnen
		globalTime += TimeDiv;

		//Runterskalierung des Bildes für weniger Rechaufwand (Faktor 1/2)
		//double scale = 0.5;
		//resize(cam_img, rgb_scale, Size(), scale, scale);

		//Spiegelung
		if (mirror_flag) {
			// horizontale Spiegelung der oberen Bildhälfte
			for (int y = height/2; y < height; y++) {
				for (int x = 0; x < width; x++) {
					unsigned long pos = x * channels + y * stride;
					unsigned long posNew = x * channels + (height - y) * stride;
					for (unsigned int c = 0; c < channels; c++) /* all components B, G, R */
					{
						cam_img.data[pos + c] = cam_img.data[posNew + c]; //Daten der aktuellen Position werden an neue Position gegeben
					}
				}
			}
		}

		//Wasserfarben in der unteren Bildhälfte
		if (water_color) {
			//BGR Farbkanäle werden in RGB Farbkanäle konvertiert
			for (int y = height / 2; y < height; y++) {
				for (int x = 0; x < width; x++) {
					unsigned long pos = x * channels + y * stride;
					int tmp = cam_img.data[pos];
					cam_img.data[pos] = cam_img.data[pos + 2];
					cam_img.data[pos + 2] = tmp;
				}
			}
		}

		if(water_effect){
			//Durchschnittswerte der nachbarn berechnen (Glättung)
			for (int y = height / 2 + 1; y < height - 59; ++y) {
				for (int x = 1; x < width-1; ++x){
					int avg[3] {0, 0, 0};

					//Nachbarwerte für RGB addieren
					for (int iy = -1; iy <= 1; ++iy) {
						for (int ix = -1; ix <= 1; ++ix) {
							unsigned int neighbor_pos = (x + ix) * channels + (y + iy) * stride;
							for (int c = 0; c < channels; ++c) {
								avg[c] += cam_img.data[neighbor_pos + c];
							}
						}
					}

					//Mittelwert berechnen
					unsigned int pos = x * channels + y * stride;
					for (int c = 0; c < channels; ++c) {
						avg[c] /= 9; // Durchschnitt
						smooth_img[pos + c] = avg[c];
					}
				}
			}
			//Kantenerkennung (Sobel-Operator)
			for (int y = height / 2 + 1; y < height - 59; ++y) {
				for (int x = 1; x < width - 1; ++x) {
					int gradient[3] = { 0, 0, 0 };

					for (int c = 0; c < channels; ++c) {
						int gx = 0;
						int gy = 0;

						for (int iy = -1; iy <= 1; ++iy) {
							for (int ix = -1; ix <= 1; ++ix) {
								int weight_x = (ix == -1) ? -1 : (ix == 1) ? 1 : 0;
								int weight_y = (iy == -1) ? -1 : (iy == 1) ? 1 : 0;

								unsigned int neighbor_pos = (x + ix) * channels + (y + iy) * stride;
								gx += smooth_img[neighbor_pos + c] * weight_x;
								gy += smooth_img[neighbor_pos + c] * weight_y;
							}
						}

						// Gradientenberechnung
						gradient[c] = sqrt(gx * gx + gy * gy);
					}

					unsigned int pos = x * channels + y * stride;
					for (int c = 0; c < channels; ++c) {
						edge_data[pos + c] = min(255, gradient[c]); // Begrenzung
					}
				}
			}
			// 3. Farbtiefen-Reduzierung (Posterization)
			for (int y = height / 2; y < height-60; ++y) {
				for (int x = 0; x < width; ++x) {
					unsigned int pos = x * channels + y * stride;
					for (int c = 0; c < channels; ++c) {
						// Reduzierte Farbtiefe, z. B. 16 Stufen
						cam_img.data[pos + c] = (cam_img.data[pos + c] / 16) * 16;

						// Kombinieren von Kantendaten mit dem Originalbild
						cam_img.data[pos + c] = min(255, cam_img.data[pos + c] + edge_data[pos + c] / 2);
					}
				}
			}
		}

		//radiale Wellen im Bild
		//Zunächst Wellenwert Berechnung an Punkt (x, y) durch Sinusfunktion (Abstand zum Quellpunkt berechnen => Radius r)
		if (radial_flag) {

			//Koordinaten der Wellenquelle
			const int cx = width / 2;
			const int cy = height / 3;
			
			for (int y = height / 2; y < height - 60; ++y) {
				for (int x = 0; x < width; ++x) {
					// Abstand zur Quelle
					double r = sqrt(pow(x - cx, 2) + pow(y - cy, 2));

					// Wassertropfen-Welle berechnen
					double wave = amplitude * std::sin(2.0 * CV_PI * r / wavelength - speed * globalTime) / (1.0 + damping * r);

					//Farbwerte setzen
					unsigned long pos = x * channels + y * stride;
					for (unsigned int c = 0; c < channels; c++) {
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
		char fps_char[3];
		sprintf ( fps_char, "%d", fps);
		const string& fps_string = (string) fps_char;
		putText( cam_img, fps_string, Point( width - 40, 25), FONT_HERSHEY_SIMPLEX, 
			0.5 /*fontScale*/, Scalar( 0, 255, 255), 2);
#endif

		//Height-Ausgabe oben links
		char info_char[5];
		sprintf(info_char, "%d", height);
		const string& info_string = (string)info_char;
		putText(cam_img, info_string, Point(40, 25), FONT_HERSHEY_SIMPLEX,
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
		if (key == 's') //toggle mirror_flag (s -> spiegeln)
		{
			mirror_flag = 1 - mirror_flag;
		}
		else if (key == 'w') { // toggle water_color (w -> wasserfarbe)
			water_color = 1 - water_color;
		}
		else if (key == 'e') { // toggle water_effect (e -> effekt)
			water_effect = 1 - water_effect;
		}
		else if (key == 'r') { // toggle radial_wave (r -> radiale Wellen)
			radial_flag = 1 - radial_flag;
		}
		else if (key == 'p') { // toggle play_flag (p -> play)
			mirror_flag = 1 - mirror_flag;
			water_color = 1 - water_color;
			water_effect = 1 - water_effect;
			radial_flag = 1 - radial_flag;
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

		//Mouseklick-in-Rechteck-Event
		Rect rect(0, height/2, width, ((height/2)-60));
		if (click_in_rect(mp, rect, folder)) {
			//Welle hinzufügen
			waves.push_back({mp.mouse_pos.x, mp.mouse_pos.y, globalTime});
		}

		//Wellen verarbeiten
		for (auto it = waves.begin(); it != waves.end();) {
			double elapsedTime = globalTime - it->startTime;

			//Welle entfernen, wenn die max. Lebensdauer überschritten wurde
			if (elapsedTime > MaxWaveTime) {
				it = waves.erase(it);
				continue;
			}

			//Zeitliche Dämpfung
			double timeDamping = exp(-alpha * elapsedTime);

			// Wellen berechnen
			for (int y = height / 2; y < height - 60; ++y) {
				for (int x = 0; x < width; ++x) {
					//Abstand zum Mittelpunkt der aktuellen Welle
					double r = sqrt(pow(x - it->wx, 2) + pow(y - it->wy, 2));

					// Wassertropfen-Welle berechnen
					double wave = amplitude * timeDamping * sin(2.0 * CV_PI * r / wavelength - speed * elapsedTime) / (1.0 + damping * r);

					//Farbwerte setzen
					unsigned long pos = x * channels + y * stride;
					for (unsigned int c = 0; c < channels; c++) {
						cam_img.data[pos + c] = saturate_cast<uchar>(cam_img.data[pos + c] + wave);
					}
				}
			}
			//Nächste Welle
			++it;
		}


		std::this_thread::sleep_for(std::chrono::milliseconds(30));
		/********************************************************************************************/
		/* show window with live video	*/		//Le-Wi: Funktionalitäten zum Schließen (x-Button)
		if (!IsWindowVisible( cvHwnd)) 
		{
			break;
		}
		imshow(windowGameOutput, cam_img); //Ausgabefenster darstellen	
	}	// Ende der Endlos-Schleife

	//Freigabe aller Matrizen
	if (cap.isOpened()) cap.release(); //Freigabe der Kamera
	if (cam_img.data) cam_img.release();
	if (cam_img_grey.data) cam_img_grey.release();
	if (rgb_scale.data) rgb_scale.release();
	if (strElement.data) strElement.release();


#if defined _DEBUG || defined LOGGING
	fclose( log );
#endif
	//FreeConsole(); //Konsole ausschalten
	cvDestroyAllWindows();
	//_CrtDumpMemoryLeaks();
	exit( 0);
}