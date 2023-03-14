*Adrian Pana 323CC*

# Tema 1 - Loader de executabile

## Calculare adresa + pagina
- se verifica ca semnalul primit este un Page Fault
- se calculeaza segmentul in care se afla Page Fault-ul
- daca nu s-a gasit in niciun segment, se va apela default handler-ul
- se calculeaza indexul paginii in care se afla Page Fault-ul

## Mapare

- campul data dintr-un segment este folosit pentru a marca pagina cu indexul
respectiv ca fiind marcata
- daca pagina e deja mapata, se apeleaza default handler-ul
- se marcheaza mapa ca fiind mapata
- se mapeaza pagina, se zeroizeaza

## Copiere date + permisiuni

- daca se afla in cadrul file size, se citesc din fisier atatia octecti cat are
o pagina sau cat a mai ramas pana la finalul file size (daca este cazul)
- se aplica permisiuni pe pagina mapata