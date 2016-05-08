####################################################################################
####                       Clock / Ssaver Version 0.15
####                   Uhrzeit im Fernsehbild anzeigen oder
####                       als Bildschirmschoner nutzen
####                    Bugreport und Anregungen im Board:
####       http://www.dbox2-tuning.net/forum/viewforum.php?f=27&start=0
####      Das New-Tuxwetter-Team: SnowHead, Worschter, Seddi, Sanguiniker
####################################################################################

Dieses Plugin blendet die Uhrzeit in das laufende Fernsehbild ein. Das ist zunächst erst
einmal eine Testversion. Durch Verzicht auf farbige Darstellung beeinflußt das Plugin
bereits angezeigte Menüs nicht in deren Farbdarstellung. Ab Version 0.15 ist farbige
Darstellung möglich.

History:
---------
v.015
- Fix Grafikbug in Screensaver
- Nur noch ein Binary, Screensaver Aufruf mit clock -ss
- 25 mögliche Vordergrundfarben, davon 8 Menüfarben (FCOL=17-24).
- FCOL=99 für Screensaver, damit wechselt die Farbe bei jedem Aufprall.
- zufällige Startposition- und Richtung des Screensavers.
- Boxgröße des Hintergrundes der Bildschirmuhr angepasst.
- Farbpalette:
0  - transparent
1  - schwarz
2  - weiß
3  - rot(maroon/Weinrot)
4  - grün
5  - oliv
6  - dunkelblau
7  - lila
8  - dunkeltürkis
9  - silber
10 - grau
11 - hellrot
12 - hellgrün
13 - hellgelb
14 - blau
15 - pink
16 - türkis
17 - Fensterinhalt selektiert - Textfarbe
18 - Fensterinahlt selektiert - Hintergrundfarbe
19 - Fensterinahlt  - Textfarbe
20 - Fensterinhalt  - Hintergrundfarbe
21 - Fensterinhalt deaktiviert - Textfarbe
22 - Fensterinahlt deaktiviert - Hintergrundfarbe
23 - Titelleiste - Textfarbe
24 - Titelleiste - Hintergrundfarbe

Benutzer des Flexmenü(Shellexec) müssen die Werte FCOL und BCOL ab v0.15 nun 2-stellig
in die jeweilige Konfigurationsdatei eintragen!!!

######################################################################################

Funktion und Installation:
--------------------------
Das eigentlich aktive Plugin ist die Datei "clock". Sie kommt mit den Rechten 755 nach
/var/bin/. Um die Uhrzeit anzuzeigen, wird clock über die Kommandozeile (optional mit 
Parametern) gestartet. Zum Beenden von clock muß in /tmp/ die Datei .clock_kill erzeugt 
werden. Vor dem Beenden löscht clock noch den verwendeten Anzeigebereich. Das Starten 
und Beenden von clock kann entweder über das Menü der blauen Taste oder über das Flex-
Menü erfolgen. Für beide Varianten sind die entsprechenden Unterverzeichnisse mit allen 
benötigten Dateien im Archiv vorhanden.
Wer die Art der Anzeige noch konfigurieren möchte, kann zusätzlich die Datei clock.conf
nach /var/tuxbox/config/ kopieren. Deren Einträge haben folgende Bedeutung:

  X= legt die horizontale Position der linken unteren Ecke des Anzeigefeldes fest. Die
     Werte können im Bereich von 0 bis 540 liegen. (0 ist ganz links)

  Y= legt die vertikale Position der Oberkante des Anzeigefeldes fest. Die Werte können
     im Bereich von 0 bis 500 liegen. (0 ist ganz oben)

  DATE= Steht hier eine 1, wird unterhalb der Uhrzeit noch das Datum mit angezeigt. Bei
        0 wird nur die Uhrzeit angezeigt

  BIG= Wer die Anzeige gern größer haben möchte, trägt hier eine 1 ein. Aber bitte dann
       auch die Anzeigepositionen anpassen, damit die Anzeige noch auf den Bildschirm passt.

  SEC= Legt fest, ob die Sekunden in der Uhrzeit angezeigt werden sollen. Bei 1 werden die
       Sekunden angezeigt, bei 0 nicht. Werden die Sekunden nicht angezeigt, blinkt statt
       dessen der Doppelpunkt zwischen Stunden und Minuten.(siehe BLINK)

  BLINK= Legt fest, ob der Doppelpunkt zwischen Stunden und Minuten blinken soll,
         wenn die Uhrzeit ohne Sekunden angezeigt wird. 1=blinken, 0=aus

  FCOL= Legt die Ziffernfarbe fest, 0=Transparent, 1=Schwarz, 2=Weiss

  BCOL= Legt die Hintergrundfarbe fest, 0=Transparent, 1=Schwarz, 2=Weiss

  MAIL= legt fest, ob die von Tuxmail erzeugte Benachrichtigung über neue Mails ausgewertet
        und angezeigt werden soll, in diesem Fall blinkt in der Anzeige das Mailsymbol im
        Wechsel mit der Anzahl der neuen Mails. Nur bei Bildschirmuhr.

  SLOW= steuert die Geschwindigkeit des Bildschirmschoners. Zulässig sind Werte von 0..10.
        Je höher die Zahl, desto langsamer bewegt sich die Anzeige über den Schirm.

Für den Start über Kommandozeile können diese Parameter in der gleichen Syntax auch als
Kommandozeilenparameter übergeben werden. Eventuell vorher aus der clock.conf ausgelesene
Werte werden dabei von den Kommandozeilenparametern überstimmt. Zum Beispiel:

  /var/bin/clock X=540 Y=0 DATE=1 BIG=1 SEC=0

Es müssen nicht alle Parameter über Kommandozeile eingegeben werden. Nicht in der Kommando-
zeile übergebene oder fehlerhafte Kommandozeilen-Parameter werden zunächst aus der clock.conf
bzw. aus der ssaver.confübernommen oder, wenn diese nicht existiert, mit Defaultwerten vorbelegt.
Diese Defaultwerte sind:
X=540, Y=0, DATE=0, BIG=0, SEC=1, BLINK=1, FCOL=02, BCOL=01, MAIL=0 SLOW=1

!!!!
Benutzer des Flexmenü(Shellexec) müssen die Werte FCOL und BCOL ab v0.15 nun 2-stellig
in die jeweilige Konfigurationsdatei eintragen.

Steuerung über FlexMenü:
------------------------
Die Datei clock mit den Rechten 755 nach /var/bin/ kopieren und den Text aus der Datei
"einfuegen in shellexec.conf" in die Datei /var/tuxbox//shellexec.conf einfügen.
Dabei eine Unix-konformen Editor verwenden. Fertig.

Steuerung über blaue Taste
--------------------------
Die Datei clock mit den Rechten 755 nach /var/bin/ kopieren, clock.cfg und clock.so nach
/var/tuxbox/plugins/. clock.so mit den Rechten 755. Das Script sclock kommt mit den Rech-
ten 755 nach /var/plugins/. Anschließend Box neu starten.
Der erste Aufruf des Menüeintrages "Uhrzeit" startet bei Bedarf clock und zeigt die Uhr
an. Jeder weitere Aufruf schaltet zwischen Anzeigen und Verbergen der Uhrzeit hin und her.

Konfiguration über FlexMenü
---------------------------
Wer die Einstellungen für die Uhrzeit-Anzeige über das FlexMenü machen möchte, findet die
dafür benötigten Dateien in diesem Unterverzeichnis. Die notwendigen Ergänzungen für die
shellexec.conf müssen übernommen und das Script "cops" mit den Rechten 755 nach /var/plugins/
kopiert werden. Wer den Editor "input" noch nicht in seinem Image hat, findet auch diesen im
Archiv. Er kommt mit den Rechten 755 nach /var/bin.

Screensaver
-----------
Auf der Basis des Clock-Plugins liegt noch ein kleines Spielzeug mit bei, der Screensaver
"ssaver". Wird er gestartet (was übrigens mit den gleichen Kommandozeilenoptionen wie bei
"clock" geschehen kann), bewegt sich die Uhrzeitanzeige regellos auf dem Bildschirm. Dabei
wird der gesamte Bildschirm in der Hintergrundfarbe gefüllt und nicht nur der Bereich unter
den Zahlen. Start- und Konfigurationsmöglichkeiten sind gleich denen des Clock-Plugins. Alle
benötigten Dateien liegen dem Archiv bei. Die Eingabe von X- oder Y-Position macht beim Screen-
saver keinen Sinn und wird daher sowohl auf der Kommandozeile als auch in der ssaver.conf ig-
noriert. Der zusätzliche Parameter "SLOW=" steuert die Geschwindigkeit. Zulässig sind Werte
von 0..10. Je höher die Zahl, desto langsamer bewegt sich die Anzeige über den Schirm.
Beendet wird der Screensaver entweder durch Anlegen der Datei "/tmp/.ssaver_kill" oder durch
Drücken einer beliebigen Taste auf der Fernbedienung.


Also, viel Spaß und viel Erfolg

Das New-Tuxwetter-Team
SnowHead und Worschter
