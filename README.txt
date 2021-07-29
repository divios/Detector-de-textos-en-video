# proy-gst-2020
Aplicaciones Multimedia (2019-2020): Proyecto GStreamer - Esqueleto

Equipo:
- Sergio Bernal Parrondo    NIA: 100383441
- Óscar Campuzano Demetrio  NIA: 100383523

Versión implementada (dejar sólo las líneas que procedan):
- versión básica	SÍ (<ruta>, -h)
- mejora intervalo	SÍ (-i <tiempo>, -f <tiempo>)
- mejora texto		SÍ (-t)
- mejora borde		SÍ (-b)
- EXTRA: (-r) Tiempo que se mantienen el texto superpuesto y el borde rojo una vez desaparece de la imagen el código de barras. Por defecto 1 seg.

>> Reparto de tareas:

Óscar:

- Corregí el barcode.c original para que 'getopt' leyese los parámetros -i, -f, -t, -b y -r correctamente. También me encargué de la lectura y comprobación de los argumentos introducidos, asegurándome de que fuesen correctos y no rompiesen el programa posteriormente.
- Implementé la mejora -f mediante 'videoanalyze'. Si bien acabamos implementando dicha mejora de otra manera, 'videoanalyze' permitió implementar posteriormente -t, -b y -r.
- Implementé -h y la mejora extra -r.


Sergio:

- Diseñé un decodificador de audio/video universal con conexión de pads dinámicos.
- Implementé la mejora -b.
- Implementé la mejora de intervalo de tiempos para -i y -f mediante gst_element_seek.


Comunes:

- Implementación del zbar para la decodificación del código de barras
- Implementación de la mejora -t
- Ideas, arreglo de bugs y simplificación del código



