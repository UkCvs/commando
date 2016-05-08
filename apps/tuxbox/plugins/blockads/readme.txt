Werbe-Zapper Plugin für DBox 2 Version 0.78

Das Plugin kann gestartet werden, wenn auf einem Sender Werbung läuft. Anschließend kann auf einen anderen
Sender gezappt werden. Bei den von dem Socket-Server der "TC Unterhaltungselektronik-AG" unterstützten 
Sendern wird nun bei Änderung des Werbestatus eines Senders ein Datenpaket mit der Info für alle Sender
vom Server an die Box geschickt. Ist die Werbung des Senders, von welchem weggeschaltet wurde, wird auto-
matisch auf diesen Sender zurückgeschaltet Bei nicht unterstützten Sendern, wenn auf einem unterstützten 
Kanal keine Werbung gemeldet wird oder ohne Internetanbindung wird zunächst ein Menü eingeblendet, in 
welchem die Zeit bis zum Zurückzappen zwischen 1 und 10 Minuten ausgewählt werden kann. Die Aktivierung 
des Plugins wird im LCD-Display durch ein blinkendes "WZ" bei Internetabfrage beziehungsweise die Anzeige 
der Restzeit in Minutem und Sekunden bis zum Zurückzappen bei nichtunterstützten Sendern oder ohne Inter-
netverbindung angezeigt.
Erster Aufruf des Plugins aktiviert es. Bei Programmen vom Werbeserver und mit Internet wird nachgeschaut,
ob wirklich gerade Werbung läuft. Wenn nicht, wird die Zeitabfrage für den Zap-Timer aktiviert, wenn der
Parameter "ZapAlways=" auf "1" steht. Ist er "0", wird das Plugin mit der Meldung, daß auf diesem Sender
gerade keine Werbung gemeldet wird, beendet. Bei nicht unterstützten Programmen oder ohne Internet wird 
automatisch nach der ausgewählten RezapTime zurückgeschaltet. Ein erneuter Aufruf des Plugins während es 
läuft, deaktiviert es
Thanx to Marxx (for Idea), Mailman (for 1. zap_timer-Plugin, UMP), cAsTeR (Boxcracker)

Dateiorte ergeben sich aus den Verzeichnisnamen. 00_blockads.so, blockad und blockads brauchen Rechte 755
Bei SqashFS-Images können die .so und .cfg auch nach /var/tuxbox/plugins/ gelegt werden.

blockads.conf:

RezapTime=07                        // Position in der Zeitliste für Defaulteintrag in der angezeigten 
                                       Auswahlliste für die zeitgesteuerte Rückschaltung bei Kanälen, die
                                       nicht auf dem Werbeserver unterstützt werden oder ohne Internet

TimeX=nn                            // Eintrag Nummer X in der Zeitliste X=1..0 
									   nn=Zeit in Minuten bis zum Zurückschalten

Internet=DSL                        // DSL, ISDN oder ANALOG, alles andere bedeutet "kein Internet"

ZapAlways=1                         // auch zappen, wenn bei unterstützten Sendern gerade keine Werbung
                                       gemeldet wird
                                       
Debounce=15							// Zeit in Sekunden, die der Werbeserber für einen Kanal mindestens
									   "keine Werbung" melden muß, ehe zurückgeschaltet wird

Programm=ARD,Das Erste              // Programm=Name des Programms auf dem Server,Name des Programms in 
                                       der Neutrino-Kanalliste

ZapChanX=Das Erste                  // Programmname aus der Neutrino-Kanalliste, auf welchen bei Ver-
                                       wendung der augewählten Rezap-Zeit bei nicht unterstützen Kanälen
                                       oder nicht vorhandener Internetanbindung bei Pluginstart selbst-
                                       ständig umgeschaltet werden soll. "X" entpricht dabei den Ziffern
                                       "9" bis "0", entsprechend der ausgwählten Rezap-Zeit. 1 min="1" ...
                                       10 min="0". Fehlt der Eintrag bei der gewählten Rezap-Zeit, wird
                                       nicht automatisch weggeschaltet
                                       
KeepVolume=1						// stellt beim Zurückzappen bei Werbungsende automatisch die Lautstärke
									   und den Mutestatus des Senders vor dem Wegzappen wieder her

Während der Werbezapper läuft, erzeugt er ein Statusfile /tmp/blockads.sts , welches von anderen Program-
men für eine Statusanzeige verwendet werden kann. In der ersten Ziele des Files steht der Kanalname, von
welchem wegen Werbung weggeschaltet wurde, in Klartext, in der zweiten Zeile bei Verwendung des Zaptimers
die Zeit bis zum Zurückzappen im Format "m:ss". Wird der Werbeserver abgefragt, steht in dieser Zeile "Auto".

Der im Archiv enthaltene Ordner "Konfiguration über FlexMenü" enthält alle nötigen Dateien für die
Konfiguration von blockads mittels des "Flexiblen Menü-Plugins". Die Orte der Dateien werden durch
die Ordnerstruktur bereits vorgegeben. Das Script "wzqops" benötigt die Rechte 755. Die Datei 
"in shellexec.conf einfuegen" wird, wie der Name bereits sagt, in eine bestehende Konfigurations-
Datei des FlexMenüs eingefügt. Wer den Editor "input" noch nicht im Image hat, kopiert bitte den
im Archiv enthaltenen mit den Rechten 755 nach /var/bin/.

Grüße vom New-Tuxwetter-Team
Hilfe und Infos gibt's unter: http://www.dbox2-tuning.net/forum/viewforum.php?f=27&start=0
