# energomonitor
Merac spotreby , pripojeny na kontakt S0 z pulzneho wattmetra
Ako to funguje:
Prvé spustenie:

ESP8266 sa pokúsi pripojiť k uloženej WiFi sieti.

Ak sa nepodarí, spustí režim AP s názvom ESP8266-Config.

Konfigurácia WiFi a MQTT:

Pripojíš sa k sieti ESP8266-Config (bez hesla).

V prehliadači otvoríš stránku 192.168.4.1.

Zobrazí sa zoznam dostupných WiFi sietí a formulár pre MQTT konfiguráciu.

Vyberieš si sieť, zadáš heslo a MQTT nastavenia.

Prístup cez mDNS:

Po pripojení k WiFi sieti môžeš pristupovať k ESP8266 pomocou hostname http://esp8266.local.

Príklad použitia:
Nahraj kód do ESP8266.

Pripoj sa k sieti ESP8266-Config (bez hesla).

Otvor prehliadač a zadaj 192.168.4.1.

Vyber si sieť, zadaj heslo a MQTT nastavenia.

Po pripojení k sieti otvor http://esp8266.local v prehliadači.

Poznámka:
Ak mDNS nefunguje, stále môžeš pristupovať k ESP8266 pomocou jeho IP adresy (zobrazí sa v sériovom monitore).
