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

	/* 1015 steht hier f�r eine Speicherbelegungsnummer, welche 
	 * in {}-Klammern im Memory-Report ausgegeben wurde. Bei erneuter 
	 * Ausf�hrung unterbricht das Programm dann an der Stelle, an dem 
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
	Mat cam_img_grey; //Graustufenkamerabild f�r autom. Startgesichtfestlegung
	Mat strElement; //Strukturelement f�r Dilatations-Funktion
	Mat3b rgb_scale;	// leerer Bildkontainer f�r RGB-Werte
	char *windowGameOutput = "camDemo"; // Name of window
	//double	scale = 1.0;				// Skalierung der Berechnungsmatrizen 
	unsigned int width, height, channels, stride;	// Werte des angezeigten Bildes
	int 
		key = 0,	// Tastatureingabe
		frames = 0, //frames z�hlen f�r FPS-Anzeige
		fps = 0;	//frames pro Sekunde
	int camNum = 2;
	bool fullscreen_flag = false; //Ist fullscreen aktivert oder nicht?
	bool median_flag = false;
	bool flip_flag = true;
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
	} while (camNum> 0 && !cap.isOpened()) ; /* solange noch andere Kameras verf�gbar sein k�nnten */

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
		printf( " - 'ESC' return to Start Screen \n");
	}
	{
		HWND console = GetConsoleWindow();
		RECT r;	
		GetWindowRect( console, &r); //stores the console's current dimensions

		//MoveWindow(window_handle, x, y, width, height, redraw_window);
		//MoveWindow( console, r.left, r.top, 800, 600, TRUE);
		MoveWindow( console, 10, 0, 850, 800, TRUE);
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

	//Handle f�r das Fenster vorbereiten
	namedWindow( windowGameOutput, WINDOW_NORMAL|CV_GUI_EXPANDED); //Erlauben des Maximierens
	resizeWindow( windowGameOutput, width, height); //Start Aufl�sung der Kamera
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
	//	//Einzeichnen des eigentlichen Strukturelements (wei�er Kreis)
	//	/* ( , Mittelpunkt, radius, wei�, thickness=gef�llt*/
	//	circle( strElement, Point( strElRadius, strElRadius), strElRadius, Scalar( 255), -1);
	//}

	state = START_SCREEN;

	// Setup zum Auswerten von Mausevents
	setMouseCallback( windowGameOutput, mouse_event, (void*)&mp);


	/*-------------------- main loop ---------------*/
	while (state != DEMO_STOP)
	{
		// ein Bild aus dem Video betrachten und in cam_img speichern
		if (cap.read(cam_img) == false)
		{ //falls nicht m�glich: Fehlermeldung
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

		if (flip_flag) flip(cam_img, cam_img, 1); // cam_img vertikal spiegeln
		//Strutz cvtColor( cam_img, rgb, CV_BGR2RGB); // Konvertierung BGR zu RGB

		//Runterskalierung des Bildes f�r weniger Rechaufwand (Faktor 1/2)
		//resize(cam_img, rgb_scale, Size(), scale, scale);

		/* smoothing of images */
		if (median_flag) /* can be toggled with key 'm'*/
		{
			medianBlur(cam_img, cam_img, 7);
		}

		/* example of accessing the image data */
		/* order is:
			BGRBGRBGR...
			BGRBGRBGR...
			:
			*/
		for (unsigned int y = 0; y < height; y += 10) /* all 10th rows */
		{
			for (unsigned int x = 0; x < width; x += 10)/* all 10th columns */
			{
				unsigned long pos = x * channels + y * stride; /* position of pixel (B component) */
				for (unsigned int c = 0; c < channels; c++) /* all components B, G, R */
				{
					cam_img.data[pos + c] = 0; /* set all components to black */
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
				//setzte fenster auf original-gr��e
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
			// state = DEMO_STOP;
			for ( int y = mp.mouse_pos.y-20; y < mp.mouse_pos.y+20; y++) /* all rows */
			{
				for ( int x = mp.mouse_pos.x-15; x < mp.mouse_pos.x + 15; x++)/* all columns */
				{
					unsigned long pos = x * channels + y * stride; /* position of pixel (R component) */
					for (unsigned int c = 0; c < channels; c++) /* all components */
					{
						cam_img.data[pos + c] = 0; /* set all components to black */
					}
				}
			}
		}

		/********************************************************************************************/
		/* show window with live video	*/		//Le-Wi: Funktionalit�ten zum Schlie�en (x-Button)
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