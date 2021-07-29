
/* Proyecto GStreamer - Aplicaciones Multimedia (2019-2020)
 * Universidad Carlos III de Madrid
 * - Sergio Bernal Parrondo   100383441
 * - Oscar Campuzano Demetrio 100383523
 *
 * Versión implementada (TO DO: eliminar las líneas que no procedan):
 * - versión básica
 * - intervalo 
 * - texto
 * - bordes
 */

// Librerias usadas
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <gst/gst.h>
#include <glib.h>

// Estructura con los elementos y el pipeline
typedef struct _CustomData {
	GstElement
		*pipeline,
		*source,
		*decode,
		*video_convert,
		*video_barcode,
		*video_text,
		*video_analyse,
		*red_border,
		*video_sink,
		*audio_convert,
		*audio_resample,
		*audio_sink;
} CustomData;

// Bus del pipeline
GstBus *bus;

// Declaramos las funciones que se implementan más abajo
static void pad_added_handler(GstElement *src, GstPad *pad, CustomData *data);
static void cb_no_more_pads(GstElement *element, CustomData *data);


// Función principal
int main(int argc, char *argv[]) {

	// Declaracion de parametros
	float i = 0, f = 0, r = 1;
	int t = FALSE, b = FALSE;

	int opt;
	while ((opt = getopt(argc, argv, "hi:f:tbr:")) != -1) {

		switch (opt) {

			case 'h':
				// Mostramos la ayuda y terminamos el programa
				g_print(">>> Ayuda: Decodificacion de codigos de barras...\n");
				g_print("------------------------------------------------------------------------------------------\n");
				g_print("PARAMETROS:        \tDESCRIPCION:\n");
				g_print("<url>              \tRuta del video de entrada. Ejemplo: res/bardode3.mp4\n");
				g_print("[-i] <tiempo (seg)>\tInstante de inicio del video (default 0.0)\n");
				g_print("[-f] <tiempo (seg)>\tInstante de finalizacion del video (default video_len - i)\n");
				g_print("[-t]               \tSuperpone en imagen el codigo extraido\n");
				g_print("[-b]               \tAplica un borde rojo al detectar un codigo\n");
				g_print("[-r] <tiempo (seg)>\tRetencion de texto/borde al dejar de detectar codigo (default 1.0)\n");
				g_print("------------------------------------------------------------------------------------------\n");
				return 0;

			case 'i':
				// Lee el parámetro -i
				i = atof(optarg);
				break;

			case 'f':
				// Lee el parámetro -f
				f = atof(optarg);
				break;

			case 't':
				// Lee el parámetro -t
				t = TRUE;
				break;

			case 'r':
				// Lee el parámetro -r
				r = atof(optarg);
				break;

			case 'b':
				// Lee el parámetro -b
				b = TRUE;
				break;

			case '?':
				// Opción desconocida, se almacena en optopt
				if ((optopt == 'i') || (optopt == 'f') || (optopt == 'r'))
					fprintf(stderr, "Error: la opcion -%c requiere un argumento\n", optopt);
				else if (isprint(optopt))
					fprintf(stderr, "Error: argumento `-%c' no valido\n", optopt);
				else
					fprintf(stderr, "Error: argumento `\\x%x' no valido.\n", optopt);
				return 1;

			default:
				// Argumento no válido
				fprintf(stderr, "Error: argumento %d no valido\n", optind);
				return 1;
		}
	}

	int index;
	for (index = optind + 1; index < argc; index++) {
		printf("Error: argumento %s no valido\n", argv[index]);
		return 1;
	}

	// Leemos la ruta del vídeo
	char *filename = argv[optind];

	// Comprobación de filename, i y f
	if (filename == NULL) {
		g_printerr("Error: No se ha detectado ningun video.\n");
		return 1;
	} else if (i < 0 || f < 0) {
		g_printerr("Error: Los parametros 'i' y 'f' no pueden tener valores negativos\n");
		return 1;
	} else if (i > f && f != 0) {
		g_printerr("Error: El instante de final de video no puede ser menor que el de inicio\n");
		return 2;
	} else if (r < 0) {
		g_printerr("Error: El tiempo de retencion no puede tener un valor negativo\n");
		return 1;
	}
	


	/*** UNA VEZ LEÍDOS LOS PARÁMETROS, COMENZAMOS A CONSTRUIR EL PIPELINE Y LO ARRANCAMOS ***/

	// Inicializamos Gstreamer y declaramos las variables
	gst_init(&argc, &argv);
	CustomData data;
	GstMessage *msg;
	
	gboolean terminate = FALSE;

	// Creamos el pipeline y los elementos
	data.pipeline = gst_pipeline_new("test-pipeline");
	data.source = gst_element_factory_make("filesrc", "fuente-fichero");
	data.decode = gst_element_factory_make("decodebin", "decode");
	data.video_convert = gst_element_factory_make("videoconvert", "video_convert");
	data.video_barcode = gst_element_factory_make("zbar", "video_barcode");
	data.video_text = gst_element_factory_make("textoverlay", "video_text");
	data.video_analyse = gst_element_factory_make("videoanalyse", "video_analyse");
	data.red_border = gst_element_factory_make("videobox", "red_border");
	data.video_sink = gst_element_factory_make("autovideosink", "video_sink");
	data.audio_convert = gst_element_factory_make("audioconvert", "audio_convert");
	data.audio_resample = gst_element_factory_make("audioresample", "audio_resample");
	data.audio_sink = gst_element_factory_make("autoaudiosink", "audio_sink");

	// Comprobamos las creaciones
	if (!data.source || !data.pipeline || !data.decode || !data.video_convert || !data.video_barcode || !data.video_text ||
		!data.video_analyse || !data.red_border || !data.video_sink || !data.audio_convert || !data.audio_resample || !data.audio_sink) {
		g_printerr("No se pudo crear los elementos.\n");
		return 1;
	}

	// Añadimos al pipeline los elementos
	gst_bin_add_many(GST_BIN(data.pipeline), data.source, data.decode, data.video_convert, data.video_barcode, data.video_text,
					 data.video_analyse, data.red_border, data.video_sink, data.audio_convert, data.audio_resample, data.audio_sink, NULL);

	// Linkeamos el source al decode
	if (!gst_element_link_many(data.source, data.decode, NULL)) {
		g_printerr("ERROR: Source y decode no pudieron linkearse\n");
		gst_object_unref(data.pipeline);
		return -1;
	}

	// Linkeamos los elementos de vídeo, sin el decode
	if (!gst_element_link_many(data.video_convert, data.video_barcode, data.video_text, data.video_analyse, data.red_border, data.video_sink, NULL)) {
		g_printerr("ERROR: video_convert y video_sink no pudieron linkearse\n");
		gst_object_unref(data.pipeline);
		return -1;
	}

	// Linkeamos los elementos de audio, sin el decode
	if (!gst_element_link_many(data.audio_convert, data.audio_resample, data.audio_sink, NULL)) {
		g_printerr("ERROR: audio_convert, audio_resample y/o audio_sink no pudieron linkearse\n");
		gst_object_unref(data.pipeline);
		return -1;
	}

	// Linkeamos el decode mediante la funcion pad_added_handler, que salta cuando el decode crea un nuevo pad de salida
	g_signal_connect(data.decode, "pad-added", G_CALLBACK(pad_added_handler), &data);
	
	// Cuando ya no existan más pads por conectar, arrancamos el pipeline
	g_signal_connect(data.decode, "no-more-pads", G_CALLBACK(cb_no_more_pads), &data);
	
	// Source apunta al video que hemos pasado como argumento
	g_object_set(data.source, "location", filename, NULL);

	// Cambiamos el estado del pipeline a "pausado". En caso de error, desreferenciamos y terminamos el programa
	if (gst_element_set_state(data.pipeline, GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE) {
		g_printerr("Unable to set the pipeline to the paused state.\n");
		gst_object_unref(data.pipeline);
		return -1;
	}

	// Declaramos las constantes y variables de lectura del bus
	const GstStructure *s;
	const gchar *symbol;
	GstClockTime frame_timestamp, barcode_original_timestamp, barcode_refresh_timestamp;
	int barcode_detected = FALSE;

	// Creamos el bus y ejecutamos el loop
	bus = gst_element_get_bus(data.pipeline);
	
	do {
		// Filtrado de mensajes. Vale NULL en caso de no recibir ninguno
		msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS | GST_MESSAGE_ELEMENT | GST_MESSAGE_APPLICATION);

		if (msg != NULL) {
			GError *err;
			gchar *debug_info;
			
			// Actuación para cada tipo de mensaje
			switch (GST_MESSAGE_TYPE(msg)) {

				case GST_MESSAGE_ERROR:
					gst_message_parse_error(msg, &err, &debug_info);
					g_printerr("Error received from element %s: %s\n", GST_OBJECT_NAME(msg->src), err->message);
					g_printerr("Debugging information: %s\n", debug_info ? debug_info : "none");
					g_clear_error(&err);
					g_free(debug_info);
					terminate = TRUE;
					break;

				case GST_MESSAGE_EOS:
					g_print("End-Of-Stream reached.\n");
					terminate = TRUE;
					break;

				case GST_MESSAGE_STATE_CHANGED:
					if (GST_MESSAGE_SRC(msg) == GST_OBJECT(data.pipeline)) {
						GstState old_state, new_state, pending_state;
						gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
						g_print("Pipeline state changed from %s to %s:\n", gst_element_state_get_name(old_state), gst_element_state_get_name(new_state));
					}
					break;

				case GST_MESSAGE_ELEMENT:
					
					// Lectura del mensaje
					s = gst_message_get_structure(msg);

					// Mensaje recibido de zbar (Decodificación del código de barras)
					if (gst_message_has_name(msg, "barcode")) {
					
						// Registra el instante en que dicho mensaje es mandado al bus
						gst_structure_get_clock_time(s, "timestamp", &barcode_refresh_timestamp);

						if (!barcode_detected) {
							barcode_detected = TRUE;
							
							// Registra el instante en que se recibió el primer mensaje y recoge el texto
							gst_structure_get_clock_time(s, "timestamp", &barcode_original_timestamp);
							symbol = gst_structure_get_string(s, "symbol");

							// Lo mostramos por el terminal
							g_print("%s ", symbol);
							g_print("[%u" GST_TIME_FORMAT "]\n", GST_TIME_ARGS(barcode_original_timestamp));

							// En caso de -b o -t, aplicamos el borde o el texto superpuesto, respectivamente
							if (b) g_object_set(data.red_border, "top", -5, "left", -5, "right", -5, "bottom", -5, "fill", 3, NULL);
							if (t) g_object_set(data.video_text, "text", symbol, NULL);
						}
						
					// Mensaje recibido del analizador de vídeo
					} else if (gst_message_has_name(msg, "GstVideoAnalyse")) {
					
						// Registra el instante del frame
						gst_structure_get_clock_time(s, "timestamp", &frame_timestamp);

						// En caso de haber detectado un código anteriormente, y no detectarlo desde 'r' segundos, baja el flag
						if (barcode_detected && frame_timestamp - barcode_refresh_timestamp > r * pow(10, 9)) {
							barcode_detected = FALSE;
							
							// En caso de -b o -t, eliminamos el borde y el texto, respectivamente
							if (b) g_object_set(data.red_border, "top", 0, "left", 0, "right", 0, "bottom", 0, NULL);
							if (t) g_object_set(data.video_text, "text", "", NULL);
						}
					}
					
					break;
				
				case GST_MESSAGE_APPLICATION:
				
					if (gst_message_has_name(msg, "ExPrerolled")) {
						g_print("We are all prerolled, do seek\n");
						
						// Extrae la duración del vídeo original
						gint64 video_len;
						gst_element_query_duration(data.pipeline, GST_FORMAT_TIME, &video_len);
						
						// Compara el -f introducido con dicha duración
						if (f == 0 || f > video_len) f = video_len;
						else f *= GST_SECOND;
						
						// Ahora que conocemos la duración del vídeo, volvemos a comparar -i con -f por última vez
						if (i * GST_SECOND > f) {
							g_printerr("ERROR: El instante de inicio introducido excede la duracion del video\n");
							g_printerr("Eliminando pipeline y terminando el programa...\n");
							
							// Desreferenciamos
							gst_message_unref(msg);
							gst_object_unref(bus);
							gst_element_set_state(data.pipeline, GST_STATE_NULL);
							gst_object_unref(data.pipeline);
							
							return 2;
						}
						
						// Imprime la duración del intervalo seleccionado
						g_print(">>> Duracion del intervalo de video: %i segundos\n", (int)((f - i*GST_SECOND) * pow(10, -9)));
						
						// Arranca el pipeline para el intervalo establecido
						gst_element_seek(data.pipeline, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE, GST_SEEK_TYPE_SET, i*GST_SECOND, GST_SEEK_TYPE_SET, f);
						gst_element_set_state(data.pipeline, GST_STATE_PLAYING);
					}
					
					break;

				default:
					// No se debería llegar hasta aquí
					g_printerr("Unexpected message received.\n");
					break;
			}

			gst_message_unref(msg);
		}

	} while (!terminate);

	// Liberamos recursos y terminamos con éxito
	gst_object_unref(bus);
	gst_element_set_state(data.pipeline, GST_STATE_NULL);
	gst_object_unref(data.pipeline);
	
	return 0;
}




/*** FUNCIONES AUXILIARES ***/

// Esta función salta cuando están todos los pads linkeados; manda al bus el mensaje de arranque
static void cb_no_more_pads(GstElement *element, CustomData *data) {
	g_print("All pads linked\n");
	gst_bus_post(bus, gst_message_new_application(GST_OBJECT_CAST(data->pipeline), gst_structure_new_empty("ExPrerolled")));
}


// Esta función salta cuando se crea un nuevo pad en el decode; mira el tipo de pad y si hace falta conectarlo
static void pad_added_handler(GstElement *src, GstPad *new_pad, CustomData *data) {
	GstPad *sink_video_pad = gst_element_get_static_pad(data->video_convert, "sink");
	GstPad *sink_audio_pad = gst_element_get_static_pad(data->audio_convert, "sink");

	GstPadLinkReturn ret;
	GstPadLinkReturn ret2;
	GstCaps *new_pad_caps = NULL;
	GstStructure *new_pad_struct = NULL;
	const gchar *new_pad_type = NULL;

	g_print("Received new pad '%s' from '%s':\n", GST_PAD_NAME(new_pad), GST_ELEMENT_NAME(src));

	// Miramos de qué tipo es ese pad
	new_pad_caps = gst_pad_get_current_caps(new_pad);
	new_pad_struct = gst_caps_get_structure(new_pad_caps, 0);
	new_pad_type = gst_structure_get_name(new_pad_struct);

	// Comprobamos si es de audio o vídeo y que no este linkeado ya
	if (g_str_has_prefix(new_pad_type, "video/x-raw") && !gst_pad_is_linked(sink_video_pad))
		ret = gst_pad_link(new_pad, sink_video_pad);
	else if (g_str_has_prefix(new_pad_type, "audio/x-raw") && !gst_pad_is_linked(sink_audio_pad))
		ret2 = gst_pad_link(new_pad, sink_audio_pad);
	else
		goto exit;

	// Miramos si se han conectado bien
	if (GST_PAD_LINK_FAILED(ret) || GST_PAD_LINK_FAILED(ret2))
		g_print("Type is '%s' but link failed.\n", new_pad_type);
	else
		g_print("Link succeeded (type '%s').\n", new_pad_type);

	exit:
	// Liberamos recursos
	if (new_pad_caps != NULL) gst_caps_unref(new_pad_caps);
	gst_object_unref(sink_video_pad);
	gst_object_unref(sink_audio_pad);
}
