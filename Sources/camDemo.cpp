/*****************************************************************
*	File...:	camDemo.cpp
*	Purpose:	video processing
*	Date...:	30.09.2019
*	Changes:	16.10.2024 mouse events
*
*********************************************************************/

#include "camDemo.h"

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
			char path[512];
			sprintf_s(path, "%s/click_on_button.wav", folder);
			PlaySoundA(path, NULL, SND_ASYNC);
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
			char path[512];
			sprintf_s(path, "%s/click_on_button.wav", folder);
			PlaySoundA(path, NULL, SND_ASYNC);
			const double amplitude = 255.0;   // Maximale Amplitude
			const double wavelength = 50.0;  // Wellenlänge
			const double speed = 1.0;        // Geschwindigkeit der Welle
			const double damping = 0.3;     // Dämpfung
			double time = 0.0;               // Zeitparameter
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
	bool median_flag = false;
	bool flip_flag = false;
	bool mirror_flag = false; //Mirror-Flag gibt an, ob Bild gespiegelt ist oder nicht
	bool water_color = false; //Water-Flag gibt an, ob Wasserfarben verwendet werden
	bool radial_wave = false; //Radial-Flag gibt an, ob Radialwellen verwendet werden
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
		/* some infos on console	*/
		printf( "==> Program Control <==\n");
		printf( "==                   ==\n");
		printf( "* Start Screen\n");
		printf( " - 'ESC' stop the program \n");
		printf( " - 'p'   open the camera-settings panel\n");
		printf( " - 't'   toggle window size\n");
		printf( " - 'f'   toggle fullscreen\n");
		printf( " - 'l'   toggle horizontal flip\n");
		printf( " - 's'   toggle mirror img\n");
		printf( " - 'w'   toggle water color\n");
		printf( " - 'r'   toggle radial wavve\n");
		printf( " - 'ESC' return to Start Screen \n");
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
	printf("\nwidth = " + width);
	printf("\nheight = " + height);
	
		

	//Handle für das Fenster vorbereiten
	namedWindow( windowGameOutput, WINDOW_NORMAL|CV_GUI_EXPANDED); //Erlauben des Maximierens
	resizeWindow( windowGameOutput, width, height); //Start Auflösung der Kamera
	HWND cvHwnd = (HWND )cvGetWindowHandle( windowGameOutput); //window-handle to detect window-states

	srand( (unsigned) time(NULL));//seeds the random number generator

	/* find folder for ressources	*/
	{
		FILE* in = NULL;
		strcpy_s(folder, folder1);/* try first folder */
		sprintf_s(path, "%s/click_on_button.wav", folder);
		in = fopen(path, "r");
		if (in == NULL)
		{
			strcpy_s(folder, folder2); /* try other folder */
			sprintf_s(path, "%s/click_on_button.wav", folder);
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


	/* structure element for dilation of binary image */
	//{
	//	int strElRadius = 3;
	//	int size = strElRadius * 2 + 1;
	//	strElement = Mat( size, size, CV_8UC1, Scalar::all( 0)); 
	//	//Einzeichnen des eigentlichen Strukturelements (weißer Kreis)
	//	/* ( , Mittelpunkt, radius, weiß, thickness=gefüllt*/
	//	circle( strElement, Point( strElRadius, strElRadius), strElRadius, Scalar( 255), -1);
	//}

	state = START_SCREEN;

	// Setup zum Auswerten von Mausevents
	setMouseCallback( windowGameOutput, mouse_event, (void*)&mp);


	// Initialisierungen für radiale Wellen
	// Wellenparameter
	const double amplitude = 255.0;   // Maximale Amplitude
	const double wavelength = 50.0;  // Wellenlänge
	const double speed = 2.0;        // Geschwindigkeit der Welle
	const double damping = 0.2;     // Dämpfung
	double time = 0.0;               // Zeitparameter


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

		if (flip_flag) flip(cam_img, cam_img, 0); // cam_img horizontal spiegeln
		//cvtColor( cam_img, cam_img, CV_BGR2RGB); // Konvertierung BGR zu RGB

		//Runterskalierung des Bildes für weniger Rechaufwand (Faktor 1/2)
		//double scale = 0.5;
		//resize(cam_img, rgb_scale, Size(), scale, scale);

		/* smoothing of images */
		if (median_flag) /* can be toggled with key 'm'*/
		{
			medianBlur(cam_img, cam_img, 3);
		}

		/* example of accessing the image data */
		/* order is:
			BGRBGRBGR...
			BGRBGRBGR...
			:
			*/

		//Spiegelung
		if (mirror_flag) {
			//Schwarze TrennLinie zum gespiegelten Bild
			//for (int x = 0; x < width; x++) {
			//	unsigned long pos = x * channels + height / 2 * stride; /* position of pixel (B component) */
			//	for (unsigned int c = 0; c < channels; c++) /* all components B, G, R */
			//	{
			//		cam_img.data[pos + c] = 0; /* set all components to black */
			//	}
			//}
			


			// horizontal Spiegelung der oberen Bildhälfte
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
			/*for (int y = height/2; y < height; y++) {
				for (int x = 0; x < width; x++) {
					unsigned long pos = x * channels + y * stride;
					int tmp = cam_img.data[pos];
					cam_img.data[pos] = cam_img.data[pos + 2];
					cam_img.data[pos + 2] = tmp;
				}
			}*/

			//Blauwerte erhöhen
			/*for (int y = height / 2; y < height-60; y++) {
				for (int x = 0; x < width; x++) {
					unsigned long pos = x * channels + y * stride;
					(((255) < ((cam_img.data[pos] + 55)) ? (255) : ((cam_img.data[pos] + 55))
				}
			}*/

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

					//Mittelwert berechnen und auf Pixel anwenden
					unsigned int pos = x * channels + y * stride;
					for (int c = 0; c < channels; ++c) {
						avg[c] /= 9; // Durchschnitt
						cam_img.data[pos + c] = (((255) < ((cam_img.data[pos + c] + avg[c]) / 2)) ? (255) : ((cam_img.data[pos + c] + avg[c]) / 2)); // Begrenzung auf 255
					}
				}
			}

			for (int y = height/2; y < height-60; ++y) {
				for (int x = 0; x < width; ++x) {
					unsigned int pos = x * channels + y * stride;
					for (int c = 0; c < channels; ++c) {
						// Reduzierte Farbtiefe, z. B. 8 Stufen
						cam_img.data[pos + c] = (cam_img.data[pos + c] / 32) * 32;
					}
				}
			}
		}

		//radiale Wellen im Bild
		//Zunächst Wellenwert Berechnung an Punkt (x, y) durch Sinusfunktion (Abstand zum Mittelpunkt|Startpunkt berechnen => Radius r)
		//Danach Phasenverschiebung => Welle
		if (radial_wave) {
			
			// Punkt der Wellen Quelle
			const int cx = width / 2;
			const int cy = height / 3;

			for (int y = height/2; y < height-60; ++y) {
				for (int x = 0; x < width; ++x) {
					// Abstand vom Mittelpunkt
					double r = std::sqrt((x - cx) * (x - cx) + (y - cy) * (y - cy));

					// Wassertropfen-Welle berechnen
					double wave = amplitude * std::sin(2.0 * CV_PI * r / wavelength - speed * time) / (1.0 + damping * r);

					//Farbwerte setzen (Achtung BGR nicht RGB!)
					unsigned long pos = x * channels + y * stride;
					for (unsigned int c = 0; c < channels; c++) {
						cam_img.data[pos + c] = saturate_cast<uchar>(cam_img.data[pos + c] + wave);
					}
				}
			}
			// Zeitparameter anpassen
			time += 0.1;
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
		
		if (key == 'm') /* toggle flag */
		{
			if (median_flag) median_flag = false;
			else median_flag = true;
		}
		else if (key == 'l')
		{
			flip_flag = 1 - flip_flag; /* toggle left-right flipping of image	*/
		}
		else if (key == 's') //toggle mirror_flag (s -> spiegeln)
		{
			mirror_flag = 1 - mirror_flag;
		}
		else if (key == 'w') { // toggle water_color (w -> wasserfarbe)
			water_color = 1 - water_color;
		}
		else if (key == 'r') { // toggle radial_wave (r -> radiale Wellen)
			radial_wave = 1 - radial_wave;
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

		/* example for using the mouse events */
		if (click_left(mp, folder))
		{
			
		}

		Rect rect(0, height/2, width, ((height/2)-60));
		if (click_in_rect(mp, rect, folder)) {
			// Wellenquelle
			const int cx = mp.mouse_pos.x;
			const int cy = mp.mouse_pos.y;

			for (int y = height / 2; y < height-60; ++y) {
				for (int x = 0; x < width; ++x) {
					// abstand vom mittelpunkt
					double r = std::sqrt((x - cx) * (x - cx) + (y - cy) * (y - cy));

					// wassertropfen-welle berechnen
					double wave = amplitude * std::sin(2.0 * CV_PI * r / wavelength - speed * time) / (1.0 + damping * r);

					//farbwerte mit wellenwert addieren
					unsigned long pos = x * channels + y * stride;
					for (unsigned int c = 0; c < channels; c++) {
						cam_img.data[pos + c] = saturate_cast<uchar>(cam_img.data[pos + c] + wave);
					}
				}
			}
			// zeitparameter anpassen
			time += 0.1;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(30));
		/********************************************************************************************/
		/* show window with live video	*/		//Le-Wi: Funktionalitäten zum Schließen (x-Button)
		if (!IsWindowVisible( cvHwnd)) 
		{
			break;
		}
		imshow( windowGameOutput, cam_img); //Ausgabefenster darstellen		
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