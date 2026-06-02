
// Il file è fatto appositamente per leggere file .omap che non sono altro che
// degli XML che hanno tutta una parte precedente sui simboli. Ogni file usa
// numeri diversi per chiamare lo stesso simbolo ma all'inizio dei file vengono
// chiamati tutti i simboli e c'è una corrispondenza individuabile tra i numeri
// utilizzati in quel file e gli id generici del mondo dell'orienteering. Dunque
// il programma crea un corrispettivo tra questi due numeri.

// C'è un vettore di id\_generici vietati, grazie al quale si possono ottenere i
// numeri specifici utilizzati dal file per gli oggetti vietati. Viene
// utilizzato lo stesso metodo per gli oggetti pieni.

// Tra i simboli dell'orienteering c'è un simbolo che è diventato oltrepassabile
// da pochi anni quindi può capitare che sia considerato come non oltrepassabile
// in alcune mappe vecchie, perciò nel programma è lasciata la possibilità di
// scelta all'utente.

// Una volta trovati i numeri si possono cercare gli "object"(solo quelli con id
// vietati) nel file, e si salvano le coordinate. Una volta trovati tutti gli
// oggetti vietati si trovano il massimo e il minimo in x e in y e si calcolano
// quindi i limiti della griglia che viene creata con risoluzione fissa di 800.
// La risoluzione corrisponde al numero di bool della larghezza mentre la
// lunghezza viene calcolata in base alle proporzione dei massimi-minimi in x e
// y.

// La griglia parte libera e prima vengono segnati i bordi usando l'algoritmo di
// Bresenham, o le curve di Bezier poi, se il simbolo è pieno, viene riempito
// orizzontalmente: per ogni riga si parte dal margine e la riga si riempie dopo
// aver attraversato un numero dispari di volte il bordo fino a che non si
// incontra il bordo successivo di quell'oggetto. Si ottiene così la griglia di
// bool che viene stampata in CSV per essere vista meglio. La griglia, la
// posizione di partenza e arrivo passano al pathfinding  che restituirà i
// percorsi.

// Si crea un file .omap uguale all'originale a cui vengono aggiunti i percorsi.

#include "parser.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>

int main() {
  std::cout << "Inserire nome della mappa:\n";
  std::string nome_file;
  std::cin >> nome_file;
  auto risultato = pf::helper_main(nome_file);
}