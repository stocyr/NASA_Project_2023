# Serial Camera Live View

## Installation

1. Mit der Command prompt in den root folder des Projekts navigieren
2. `py -m venv venv` eingeben (erstellt das virtual environment "venv")
3. `call venv\Scripts\activate` eingeben (aktiviert das venv)
4. `pip install -r requirements.txt` eingeben (installiert die dependencies)

## Start

1. Virtual environment aktivieren (wenn nicht schon aktiviert)
2. Wenn Quest Hardware noch nicht verbunden: Verbinden
  - Wenn schon verbunden, aber noch nicht im interactive mode: reset drücken
  - Ansonsten: weiter zu 3.
2. Python script starten: `python serial_camera_live_view\download_image.py`
3. Zwei mal ENTER drücken
4. Warten (bei "Line 02:" geht es ca. 10 sekunden)
5. Das fertige Bild ist als `serial_camera_live_view\test.jpg` gespeichert.
6. Für ein weiteres Bild ENTER drücken.

Das bild kann z.B. im Visual Studio Code geöffnet sein, dann aktualisiert es sich automatisch jedes Mal, wenn es neu überschrieben wird.

## Informationen

Das Script verbindet sich automatisch mit einem Quest Hardware (einem ItsyBitsy Virtal ComPort), welcher per "Vendor ID" und "Product ID" auf dem USB Bus eindeutig erkennbar ist, so muss man den ComPort nicht noch suchen. Ausser, es sind mehrere Quests mit dem Computer verbunden, dann wird einfach der erste verbunden.