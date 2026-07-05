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
#include "path.hpp"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>
namespace pf {

size_t salta_virgolette_spazi(const std::string &linea, size_t inizio) {
  while (inizio < linea.size() &&
         (linea[inizio] == '"' || linea[inizio] == ' ')) {
    ++inizio;
  }
  return inizio;
}

// data una stringa: linea, se rappresenta un simbolo, lo trova. Trova tutti i
// simboli presenti sulla mappa
int trova_simbolo(const std::string &linea) {
  size_t inizio_del_numero_id{0};
  if (linea.find("symbol=") != std::string::npos) {
    inizio_del_numero_id =
        salta_virgolette_spazi(linea, 7 + linea.find("symbol="));
  } else {
    return -1;
  }
  if (isdigit(linea[inizio_del_numero_id])) {
    std::string numero;
    while (inizio_del_numero_id < linea.size() &&
           isdigit(linea[inizio_del_numero_id])) {
      numero += linea[inizio_del_numero_id];
      ++inizio_del_numero_id;
    }
    return stoi(numero);
  } else {
    return -1;
  }
}

// per la stringa: linea, se rappresenta un simbolo, trova id corrispondente, si
// crea così il vettore di simboli-id
Associazione_id_simbolo ids_a_simboli(const std::string &linea) {
  size_t inizio_id{0};
  size_t inizio_simbolo{0};
  if (linea.find("id=") != std::string::npos &&
      linea.find("code=") != std::string::npos) {
    inizio_id = salta_virgolette_spazi(linea, 3 + linea.find("id="));
    inizio_simbolo = salta_virgolette_spazi(linea, 5 + linea.find("code="));
  } else {
    return {-1, -1};
  }
  std::string numero_id;
  std::string numero_simbolo;
  while (inizio_id < linea.size() && isdigit(linea[inizio_id])) {
    numero_id += linea[inizio_id];
    ++inizio_id;
  }
  while (inizio_simbolo < linea.size() && isdigit(linea[inizio_simbolo])) {
    numero_simbolo += linea[inizio_simbolo];
    ++inizio_simbolo;
  }
  if (numero_id.empty() || numero_simbolo.empty()) {
    return {-1, -1};
  }
  return Associazione_id_simbolo{std::stoi(numero_id),
                                 std::stoi(numero_simbolo)};
}

// da simboli vietati/pieni(iof), trova id vietati/pieni, utilizzando il vettore
// ottenuto dai vari Associazione_id_simbolo
std::vector<int> trova_associazione_per_id_vietati_o_pieni(
    const std::vector<Associazione_id_simbolo> &ids_simboli,
    const std::vector<int> &vietati_o_pieni) {
  std::vector<int> risultato{};
  for (const auto &coppia : ids_simboli) {
    for (int simbolo_vietato : vietati_o_pieni) {
      if (coppia.simbolo == simbolo_vietato) {
        risultato.push_back(coppia.id);
      }
    }
  }
  return risultato;
}
void calcola_fattore_scala(Griglia &griglia) {
  std::cout << "Inserire scala della mappa (se è 1:4000, scrivere 4000)\n";
  double scala = 0.;
  if (!(std::cin >> scala)) {
    std::cout << "Input non valido\n";
    throw std::runtime_error{"Scala non numerica"};
  }
  assert(scala > 0);
  griglia.inserisci_scala(scala / 1000000);
}

// Funzione per trovare tutte le coordinate================================
std::vector<Punto> estrai_coordinate(const std::string &linea,
                                     const Griglia &griglia) {
  std::vector<Punto> punti{};
  std::string stringa_coordinate{};
  auto inizio = linea.find("<coords");
  if (inizio != std::string::npos) {
    inizio = linea.find(">", inizio) + 1;
    auto fine =
        linea.find("</coords>", inizio); // nelle righe di coordinate di punti
    if (fine != std::string::npos) {
      stringa_coordinate = linea.substr(inizio, fine - inizio);
    }
  }
  if (stringa_coordinate.empty()) {
    return punti;
  }

  // token
  std::vector<std::string> token;
  size_t pos = 0;
  while (pos < stringa_coordinate.size()) {
    size_t fine = stringa_coordinate.find(';', pos);
    if (fine == std::string::npos) {
      fine = stringa_coordinate.size();
    }
    std::string pezzo = stringa_coordinate.substr(pos, fine - pos);
    // rimuove spazi iniziali/finali
    size_t primo = pezzo.find_first_not_of(" \t");
    if (primo != std::string::npos) {
      size_t ultimo = pezzo.find_last_not_of(" \t");
      pezzo = pezzo.substr(primo, ultimo - primo + 1);
      if (!pezzo.empty()) {
        token.push_back(pezzo);
      }
    }
    pos = fine + 1;
  }

  // struct punto+flag
  struct PuntoFlag {
    double x, y;
    int flag;
  };

  std::vector<PuntoFlag> punti_flag;
  for (const auto &t : token) {
    std::vector<std::string> parti;
    size_t inizio = 0;
    while (inizio < t.size()) {
      size_t sp = t.find(' ', inizio);
      if (sp == std::string::npos) {
        sp = t.size();
      }
      std::string parte = t.substr(inizio, sp - inizio);
      if (!parte.empty()) {
        parti.push_back(parte);
      }
      inizio = sp + 1;
    }
    if (parti.size() >= 2) {
      double x = std::stod(parti[0]);
      double y = std::stod(parti[1]);
      int flag = 0;
      if (parti.size() >= 3) {
        flag = std::stoi(parti[2]);
      }
      punti_flag.push_back({x, y, flag});
    }
  }

  if (punti_flag.empty()) {
    return punti;
  }

  // applica fattore di scala
  for (auto &pf : punti_flag) {
    pf.x *= griglia.ottieni_fattore_scala();
    pf.y *= griglia.ottieni_fattore_scala();
  }

  // quanti nodi di curva ci sono
  std::vector<size_t> indici_nodi;
  for (size_t i = 0; i < punti_flag.size(); ++i) {
    if (punti_flag[i].flag == 1) {
      indici_nodi.push_back(i);
      i += 2; // salta i due punti di "controllo"
      continue;
    } else {
      indici_nodi.push_back(i);
    }
  }
  // se 0, normale
  if (indici_nodi.empty()) {
    for (const auto &pf : punti_flag) {
      punti.push_back({static_cast<int>(pf.x), static_cast<int>(pf.y)});
    }
    return punti;
  }

  // formula bezier cubica
  auto bezier_cubica = [](const PuntoFlag &p0, const PuntoFlag &p1,
                          const PuntoFlag &p2, const PuntoFlag &p3,
                          double t) -> PuntoFlag {
    double u = 1.0 - t;
    double x = u * u * u * p0.x + 3.0 * u * u * t * p1.x +
               3.0 * u * t * t * p2.x + t * t * t * p3.x;
    double y = u * u * u * p0.y + 3.0 * u * u * t * p1.y +
               3.0 * u * t * t * p2.y + t * t * t * p3.y;
    return {x, y, 0};
  };

  // divide in n =10 segmentini la curva tra due nodi
  auto aggiungi_segmento = [&](size_t inizio_idx, size_t fine_idx) {
    const auto &nodo_inizio = punti_flag[inizio_idx];
    const auto &nodo_fine = punti_flag[fine_idx];
    int num_nodi_di_curva = static_cast<int>(fine_idx - inizio_idx - 1);

    if (num_nodi_di_curva == 2) {
      // Curva cubica
      const auto &nodo_di_curva_1 = punti_flag[inizio_idx + 1];
      const auto &nodo_di_curva_2 = punti_flag[inizio_idx + 2];
      const int passi = 10; // messo 10 segmentini
      for (int s = 1; s <= passi; ++s) {
        double t = static_cast<double>(s) / passi;
        auto pt = bezier_cubica(nodo_inizio, nodo_di_curva_1, nodo_di_curva_2,
                                nodo_fine, t);
        punti.push_back({static_cast<int>(pt.x), static_cast<int>(pt.y)});
      }
    } else if (num_nodi_di_curva == 1) {
      // Curva quadraticas
      const auto &c = punti_flag[inizio_idx + 1];
      const int passi = 10;
      for (int s = 1; s <= passi; ++s) {
        double t = static_cast<double>(s) / passi;
        double u = 1.0 - t;
        double x =
            u * u * nodo_inizio.x + 2 * u * t * c.x + t * t * nodo_fine.x;
        double y =
            u * u * nodo_inizio.y + 2 * u * t * c.y + t * t * nodo_fine.y;
        punti.push_back({static_cast<int>(x), static_cast<int>(y)});
      }
    } else {
      // se problemi fa linea retta
      for (size_t i = inizio_idx + 1; i <= fine_idx; ++i) {
        punti.push_back({static_cast<int>(punti_flag[i].x),
                         static_cast<int>(punti_flag[i].y)});
      }
    }
  };

  // aggiunge primo nodo
  punti.push_back({static_cast<int>(punti_flag[indici_nodi[0]].x),
                   static_cast<int>(punti_flag[indici_nodi[0]].y)});

  // e segmenti
  for (size_t k = 0; k < indici_nodi.size() - 1; ++k) {
    aggiungi_segmento(indici_nodi[k], indici_nodi[k + 1]);
  }

  return punti;
}
void calcola_dimensioni_max_min(
    const std::vector<oggetto_poligonale_vietato> &oggetti, Punto partenza,
    Punto arrivo, Griglia &griglia) {

  double min_x_calcolato = std::min(partenza.x, arrivo.x);
  double max_x_calcolato = std::max(partenza.x, arrivo.x);
  double min_y_calcolato = std::min(partenza.y, arrivo.y);
  double max_y_calcolato = std::max(partenza.y, arrivo.y);

  for (const auto &oggetto : oggetti) {
    for (const auto &punto : oggetto.contorno) {
      min_x_calcolato = std::min(min_x_calcolato, static_cast<double>(punto.x));
      max_x_calcolato = std::max(max_x_calcolato, static_cast<double>(punto.x));
      min_y_calcolato = std::min(min_y_calcolato, static_cast<double>(punto.y));
      max_y_calcolato = std::max(max_y_calcolato, static_cast<double>(punto.y));
    }
  }

  double larghezza_calcolata = max_x_calcolato - min_x_calcolato;
  double altezza_calcolata = max_y_calcolato - min_y_calcolato;
  min_x_calcolato -= larghezza_calcolata * 0.05;
  max_x_calcolato += larghezza_calcolata * 0.05;
  min_y_calcolato -= altezza_calcolata * 0.05;
  max_y_calcolato += altezza_calcolata * 0.05;

  assert(max_x_calcolato >= min_x_calcolato);
  assert(max_y_calcolato >= min_y_calcolato);

  griglia.inserisci_min_x(min_x_calcolato);
  griglia.inserisci_max_x(max_x_calcolato);
  griglia.inserisci_min_y(min_y_calcolato);
  griglia.inserisci_max_y(max_y_calcolato);
  griglia.inserisci_altezza(altezza_calcolata);
  griglia.inserisci_larghezza(larghezza_calcolata);
}

void calcola_dimensioni_max_min(
    const std::vector<oggetto_poligonale_vietato> &oggetti, Griglia &griglia) {

  double min_x_calcolato =
      std::min(griglia.ottieni_arrivo().x, griglia.ottieni_partenza().x);
  double max_x_calcolato =
      std::max(griglia.ottieni_arrivo().x, griglia.ottieni_partenza().x);
  double min_y_calcolato =
      std::min(griglia.ottieni_arrivo().y, griglia.ottieni_partenza().y);
  double max_y_calcolato =
      std::max(griglia.ottieni_arrivo().y, griglia.ottieni_partenza().y);

  for (const auto &oggetto : oggetti) {
    for (const auto &punto : oggetto.contorno) {
      min_x_calcolato = std::min(min_x_calcolato, static_cast<double>(punto.x));
      max_x_calcolato = std::max(max_x_calcolato, static_cast<double>(punto.x));
      min_y_calcolato = std::min(min_y_calcolato, static_cast<double>(punto.y));
      max_y_calcolato = std::max(max_y_calcolato, static_cast<double>(punto.y));
    }
  }

  double larghezza_calcolata = max_x_calcolato - min_x_calcolato;
  double altezza_calcolata = max_y_calcolato - min_y_calcolato;
  min_x_calcolato -= larghezza_calcolata * 0.05;
  max_x_calcolato += larghezza_calcolata * 0.05;
  min_y_calcolato -= altezza_calcolata * 0.05;
  max_y_calcolato += altezza_calcolata * 0.05;

  assert(max_x_calcolato >= min_x_calcolato);
  assert(max_y_calcolato >= min_y_calcolato);

  griglia.inserisci_min_x(min_x_calcolato);
  griglia.inserisci_max_x(max_x_calcolato);
  griglia.inserisci_min_y(min_y_calcolato);
  griglia.inserisci_max_y(max_y_calcolato);
  griglia.inserisci_altezza(altezza_calcolata);
  griglia.inserisci_larghezza(larghezza_calcolata);
}

// converte coordinate del mondo a griglia
void mondo_a_griglia(double x, double y, int &gx, int &gy, Griglia &griglia) {
  double larghezza_mondo = griglia.ottieni_max_x() - griglia.ottieni_min_x();
  double altezza_mondo = griglia.ottieni_max_y() - griglia.ottieni_min_y();
  int larghezza_griglia = griglia.ottieni_larghezza();
  int altezza_griglia = griglia.ottieni_altezza();
  gx = static_cast<int>((x - griglia.ottieni_min_x()) / larghezza_mondo *
                        larghezza_griglia);
  gy = static_cast<int>((griglia.ottieni_max_y() - y) / altezza_mondo *
                        altezza_griglia);

  if (gx < 0) {
    gx = 0;
  }
  if (gx >= larghezza_griglia) {
    gx = larghezza_griglia - 1;
  }
  if (gy < 0) {
    gy = 0;
  }
  if (gy >= altezza_griglia) {
    gy = altezza_griglia - 1;
  }

  assert(gx >= 0 && gx < larghezza_griglia);
  assert(gy >= 0 && gy < altezza_griglia);
}

// trova intersezioni rispetto a retta orizzontale
bool calcola_intersezione(int x1, int y1, int x2, int y2, int y, int &x) {
  if (y2 != y1) {
    if ((y2 < y && y1 >= y) || (y2 >= y && y1 < y)) {
      x = x1 + (y - y1) * (x2 - x1) / (y2 - y1);
      return true;
    }
  }
  return false;
}

// per ogni poligono, per ogni riga, dopo che da un lato numero di intersezioni
// è dispari, riempie ====> controllo se è pieno in oggetti_a_griglia
void riempi_poligoni(Griglia &griglia,
                     const std::vector<std::pair<int, int>> &poligono) {
  assert(!poligono.empty());

  int min_x = poligono[0].first;
  int max_x = poligono[0].first;
  int min_y = poligono[0].second;
  int max_y = poligono[0].second;
  for (const auto &punto : poligono) {
    if (punto.first < min_x) {
      min_x = punto.first;
    } else if (punto.first > max_x) {
      max_x = punto.first;
    }
    if (punto.second < min_y) {
      min_y = punto.second;
    } else if (punto.second > max_y) {
      max_y = punto.second;
    }
  }

  for (int y = min_y; y <= max_y; ++y) {
    std::vector<int> intersezioni{};
    size_t n_vertici = poligono.size();
    for (size_t i = 0; i != n_vertici; ++i) {
      const auto &p1 = poligono[i];
      const auto &p2 = poligono[(i + 1) % n_vertici];
      int intersezione_x;
      if (calcola_intersezione(p1.first, p1.second, p2.first, p2.second, y,
                               intersezione_x)) {
        intersezioni.push_back(intersezione_x);
      }
    }
    if (!intersezioni.empty()) {
      std::sort(intersezioni.begin(), intersezioni.end());
      for (size_t i = 0; i + 1 < intersezioni.size(); i += 2) {
        for (int x = intersezioni[i]; x <= intersezioni[i + 1]; ++x) {
          if (x >= 0 && x < griglia.ottieni_larghezza() && y >= 0 &&
              y < griglia.ottieni_altezza()) {
            griglia.rendi_non_oltrepassabile(x, y);
          }
        }
      }
    }
  }
}

// user decide se verde 3(per anni è stato non oltrepassabile, ora è
// oltrepassabile)
void chiedi_verde3_oltrepassabile(std::vector<int> &ids_vietati,
                                  std::vector<int> &ids_pieni) {
  std::cout << "Il verde 3 è oltrepassabile? (y/n)\n";
  std::string risposta;
  std::cin >> risposta;
  if (risposta != "Y" && risposta != "y" && risposta != "yes" &&
      risposta != "Yes" && risposta != "S" && risposta != "s" &&
      risposta != "sì" && risposta != "si" && risposta != "Sì" &&
      risposta != "Si") {
    ids_vietati.push_back(410);
    ids_pieni.push_back(410);
  }
}

// per ogni oggetto lo disegna sulla griglia, linee con bresenham
void oggetti_a_griglia(const oggetto_poligonale_vietato &oggetto,
                       Griglia &griglia) {

  if (oggetto.contorno.size() < 2) {
    return;
  }

  auto disegna_linea = [&](int x1, int y1, int x2,
                           int y2) { // lambda per disegnare linee
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    int x = x1;
    int y = y1;

    while (true) {
      if (x >= 0 && x < griglia.ottieni_larghezza() && y >= 0 &&
          y < griglia.ottieni_altezza()) {
        griglia.rendi_non_oltrepassabile(x, y);
      }

      if (x == x2 && y == y2) {
        break;
      }

      int e2 = 2 * err;
      if (e2 > -dy) {
        err -= dy;
        x += sx;
      }
      if (e2 < dx) {
        err += dx;
        y += sy;
      }
    }
  };

  std::vector<std::pair<int, int>> punti_griglia;
  punti_griglia.reserve(oggetto.contorno.size());

  for (const auto &punto : oggetto.contorno) {
    int gx{0};
    int gy{0};
    // trasforma coordinate
    mondo_a_griglia(static_cast<double>(punto.x), static_cast<double>(punto.y),
                    gx, gy, griglia);
    punti_griglia.push_back({gx, gy});
  }

  for (size_t i = 0; i != punti_griglia.size() - 1; ++i) { // disegna
    disegna_linea(punti_griglia[i].first, punti_griglia[i].second,
                  punti_griglia[i + 1].first, punti_griglia[i + 1].second);
  }
  // se è pieno riempie
  if (oggetto.e_pieno && punti_griglia.size() > 2) {
    riempi_poligoni(griglia, punti_griglia);
  }
}



double costo_percorso_geometrico_mondo(
    const std::vector<Punto>
        &percorso, // per calcolare il costo effettivo serve ritrasformare le
                   // coordinate in quelle del mondo e poi calcolarlo
    const Griglia &griglia) {
  if (percorso.size() < 2)
    return 0.0;

  auto trasforma_a_mondo = [&](const Punto &p) {
    double x = griglia.ottieni_min_x() +
               (p.x + 0.5) *
                   (griglia.ottieni_max_x() - griglia.ottieni_min_x()) /
                   griglia.ottieni_larghezza();
    double y = griglia.ottieni_max_y() -
               (p.y + 0.5) *
                   (griglia.ottieni_max_y() - griglia.ottieni_min_y()) /
                   griglia.ottieni_altezza();

    return std::pair<double, double>{x / griglia.ottieni_fattore_scala(),
                                     y / griglia.ottieni_fattore_scala()};
  };

  double costo = 0.0;

  auto precedente = trasforma_a_mondo(percorso[0]);

  for (size_t i = 1; i != percorso.size(); ++i) {
    auto corrente = trasforma_a_mondo(percorso[i]);

    double dx = corrente.first - precedente.first;
    double dy = corrente.second - precedente.second;

    costo += std::hypot(dx, dy);

    precedente = corrente;
  }

  return costo * 4 / 1000;
}

// esporta in un altro file omap
void esporta_percorso_omap(
    const std::string &nome_file_input, const std::string &nome_file_output,
    const std::vector<Percorso> &percorsi,
    const std::vector<Associazione_id_simbolo> &ids_simboli,
    const Griglia &griglia) {

  assert(!percorsi.empty());
  assert(!ids_simboli.empty());

  std::ifstream in(nome_file_input);   // input
  std::ofstream out(nome_file_output); // output

  if (!in.is_open() || !out.is_open()) {
    std::cerr << "Errore apertura file\n";
    return;
  }

  int id_linea_disegno{0};
  for (const auto &coppia : ids_simboli) {
    if (coppia.simbolo == 704) {
      id_linea_disegno = coppia.id;
      break;
    }
  }

  std::string linea;

  bool oggetti_aggiunti = false;

  // per ogni linea
  while (std::getline(in, linea)) {
    // se trova </objects> che è quello alla fine degli oggetti di solito
    // inserisce i percorsi
    if (!oggetti_aggiunti &&
        linea.find("</objects>") !=
            std::string::npos) { // trova dove c'è l'ultimo oggetto
      out << "Qui iniziano i percorsi\n";
      for (const auto &percorso : percorsi) {
        auto punti_percorso = percorso.punti;
        out << "      <object type=\"1\" symbol=\""
            << id_linea_disegno // inserisce i percorsi
            << "\">";
        out << "        <coords count=\"" << punti_percorso.size() << "\">";

        for (size_t i = 0; i != punti_percorso.size(); ++i) {
          double x_mondo =
              griglia.ottieni_min_x() +
              (punti_percorso[i].x + 0.5) *
                  (griglia.ottieni_max_x() - griglia.ottieni_min_x()) /
                  griglia.ottieni_larghezza();
          double y_mondo =
              griglia.ottieni_max_y() -
              (punti_percorso[i].y + 0.5) *
                  (griglia.ottieni_max_y() - griglia.ottieni_min_y()) /
                  griglia.ottieni_altezza();

          int x_f = static_cast<int>(x_mondo / griglia.ottieni_fattore_scala());
          int y_f = static_cast<int>(y_mondo / griglia.ottieni_fattore_scala());

          out << (x_f) << " " << (y_f);
          if (i < (punti_percorso.size() - 1)) {
            out << ";";
          } else {
            out << " 16;"; // flag finale delle coordinate
          }
        }

        out << "</coords>";
        out << "<pattern rotation=\"0\"><coord x=\"0\" "
               "y=\"0\"/></pattern>";
        out << "</object>\n";
        out << "costo del percorso precedente:" << percorso.costo
            << "m\n"; // conversione lunghezza percorsi
      }
      out << "Qui finiscono i percorsi";
      oggetti_aggiunti = true;
    }
    // altrimenti copia la linea
    out << linea << "\n"; // copia le linee
  }

  in.close();
  out.close(); // chiude i file
  std::cout << "File OMAP con percorso salvato: " << nome_file_output << "\n";
}

std::vector<Associazione_id_simbolo> leggi_ids_simboli(std::ifstream &file) {

  std::vector<Associazione_id_simbolo> ids_simboli;
  std::string linea;

  while (std::getline(file, linea)) {

    if (linea.find("<symbol") == std::string::npos) {
      continue;
    }

    if (linea.find("id=") == std::string::npos) {
      continue;
    }

    Associazione_id_simbolo coppia = ids_a_simboli(linea);

    if (coppia.id != -1 && coppia.simbolo != -1) {
      ids_simboli.push_back(coppia);
    }
  }

  return ids_simboli;
}

Insiemi_simboli costruisci_insiemi_simboli(
    const std::vector<Associazione_id_simbolo> &ids_simboli) {

  std::vector<int> vietati{206, 301, 307, 411, 515, 518,
                           520, 521, 708, 709, 714};

  std::vector<int> pieni{206, 301, 307, 411, 520, 521, 709, 714};

  chiedi_verde3_oltrepassabile(vietati, pieni);

  std::vector<int> ids_vietati =
      trova_associazione_per_id_vietati_o_pieni(ids_simboli, vietati);

  std::vector<int> ids_pieni =
      trova_associazione_per_id_vietati_o_pieni(ids_simboli, pieni);

  return {{ids_vietati.begin(), ids_vietati.end()},
          {ids_pieni.begin(), ids_pieni.end()}};
}

Posizioni_partenza_arrivo
trova_partenza_arrivo(const std::vector<Associazione_id_simbolo> &ids_simboli) {

  Posizioni_partenza_arrivo risultato;

  for (const auto &coppia : ids_simboli) {

    if (coppia.simbolo == 701) {
      risultato.partenza = coppia.id;
    }

    if (coppia.simbolo == 706) {
      risultato.arrivo = coppia.id;
    }
  }

  return risultato;
}

Dati_mappa carica_oggetti_mappa(std::ifstream &file, int id_partenza,
                                int id_arrivo,
                                const std::unordered_set<int> &id_vietati,
                                const std::unordered_set<int> &id_pieni,
                                Griglia &griglia) {

  Dati_mappa dati;

  std::string linea;

  bool dentro_memoria = false;

  file.clear();
  file.seekg(0);

  while (std::getline(file, linea)) {

    if (linea.find("<undo>") != std::string::npos ||
        linea.find("<redo>") != std::string::npos) {
      dentro_memoria = true;
      continue;
    }

    if (linea.find("</undo>") != std::string::npos ||
        linea.find("</redo>") != std::string::npos) {
      dentro_memoria = false;
      continue;
    }

    if (dentro_memoria) {
      continue;
    }

    if (linea.find("<object") == std::string::npos) {
      continue;
    }

    std::string blocco = linea;

    while (blocco.find("</object>") == std::string::npos &&
           blocco.find("/>") == std::string::npos) {

      std::string extra;

      if (!std::getline(file, extra)) {
        break;
      }

      blocco += " " + extra;
    }

    int id = trova_simbolo(blocco);

    std::vector<Punto> coordinate = estrai_coordinate(blocco, griglia);

    if (coordinate.empty()) {
      continue;
    }

    if (id == id_partenza) {
      dati.partenza_mondo = coordinate[0];
      dati.partenza_trovata = true;
    }

    if (id == id_arrivo) {
      dati.arrivo_mondo = coordinate[0];
      dati.arrivo_trovato = true;
    }

    if (id_vietati.contains(id)) {

      bool pieno = id_pieni.contains(id);

      dati.oggetti_vietati.push_back({id, coordinate, pieno});
    }
  }

  return dati;
}

void costruisci_griglia(const Dati_mappa dati_mappa, Griglia &griglia) {

  calcola_dimensioni_max_min(dati_mappa.oggetti_vietati,
                             dati_mappa.partenza_mondo, dati_mappa.arrivo_mondo,
                             griglia);

  constexpr int RISOLUZIONE = 800;

  double larghezza_mondo = griglia.ottieni_max_x() - griglia.ottieni_min_x();

  double altezza_mondo = griglia.ottieni_max_y() - griglia.ottieni_min_y();

  griglia.inserisci_larghezza(RISOLUZIONE);

  griglia.inserisci_altezza(
      static_cast<int>(RISOLUZIONE * (altezza_mondo / larghezza_mondo)));

  if (griglia.ottieni_altezza() < 1) {
    griglia.inserisci_altezza(RISOLUZIONE);
  }

  griglia.forma_griglia_vettore(griglia.ottieni_altezza(),
                                griglia.ottieni_larghezza());

  for (const auto &oggetto : dati_mappa.oggetti_vietati) {

    oggetti_a_griglia(oggetto, griglia);
  }

  griglia.dilata_ostacoli();
}

void converti_partenza_arrivo_griglia(const Punto &partenza_mondo,
                                      const Punto &arrivo_mondo,
                                      Griglia &griglia) {

  int px = 0;
  int py = 0;
  int ax = 0;
  int ay = 0;

  mondo_a_griglia(partenza_mondo.x, partenza_mondo.y, px, py, griglia);

  mondo_a_griglia(arrivo_mondo.x, arrivo_mondo.y, ax, ay, griglia);

  griglia.inserisci_partenza(px, py);
  griglia.inserisci_arrivo(ax, ay);
}

void stampa_statistiche_percorsi(const std::vector<Percorso> &percorsi,
                                 const Griglia &griglia) {

  std::cout << "Trovati " << percorsi.size()
            << " percorsi entro +20% del migliore:\n\n";

  std::cout << "Percorsi di lunghezza:\n";

  for (size_t k = 0; k != percorsi.size(); k++) {
    std::cout << costo_percorso_geometrico_mondo(percorsi[k].punti, griglia)
              << "m\n";
  }
}

void esporta_griglia_csv(const std::string &nome_file, const Griglia &griglia) {

  std::ofstream csv(nome_file);

  if (!csv.is_open()) {
    std::cout << "Errore apertura CSV\n";
    return;
  }

  int altezza = static_cast<int>(griglia.ottieni_altezza());
  int larghezza = static_cast<int>(griglia.ottieni_larghezza());

  csv << altezza << "," << larghezza << "\n";

  for (int y = 0; y != altezza; ++y) {
    for (int x = 0; x != larghezza; ++x) {

      if (x == griglia.ottieni_partenza().x &&
          y == griglia.ottieni_partenza().y) {
        csv << "S";
      } else if (x == griglia.ottieni_arrivo().x &&
                 y == griglia.ottieni_arrivo().y) {
        csv << "A";
      } else {
        csv << (griglia.e_oltrepassabile(x, y) ? "0" : "1");
      }

      if (x < larghezza - 1)
        csv << ",";
    }
    csv << "\n";
  }
}

std::vector<Percorso> aiuto_main2(const std::string &nome_file) {

  std::ifstream file(nome_file);

  if (!file.is_open()) {
    std::cout << "File non trovato\n";
    return {};
  }
  Griglia griglia{};
  calcola_fattore_scala(griglia);

  auto ids_simboli = leggi_ids_simboli(file);

  auto insiemi_simboli = costruisci_insiemi_simboli(ids_simboli);

  auto ids_partenza_arrivo = trova_partenza_arrivo(ids_simboli);

  auto dati_mappa = carica_oggetti_mappa(
      file, ids_partenza_arrivo.partenza, ids_partenza_arrivo.arrivo,
      insiemi_simboli.vietati, insiemi_simboli.pieni, griglia);
  if (!dati_mappa.partenza_trovata || !dati_mappa.arrivo_trovato) {

    std::cout << "Partenza o arrivo non trovati\n";
    return {};
  }

  costruisci_griglia(dati_mappa, griglia);
  converti_partenza_arrivo_griglia(dati_mappa.partenza_mondo,
                                   dati_mappa.arrivo_mondo, griglia);
  Punto p = griglia.ottieni_partenza();
  Punto a = griglia.ottieni_arrivo();

  std::cout << "Partenza griglia: (" << p.x << ", " << p.y << ")\n";
  std::cout << "Arrivo griglia:   (" << a.x << ", " << a.y << ")\n";
  std::cout << "Larghezza: " << griglia.ottieni_larghezza()
            << ", Altezza: " << griglia.ottieni_altezza() << "\n";
  std::cout << "Partenza libera? "
            << (griglia.e_oltrepassabile(p) ? "SI" : "NO") << "\n";
  std::cout << "Arrivo libero?   "
            << (griglia.e_oltrepassabile(a) ? "SI" : "NO") << "\n";
            std::cout<<"min_x"<<griglia.ottieni_min_x()<<"\nmin_y"<<griglia.ottieni_min_y()<<"\n max_x"<<griglia.ottieni_max_x()<<"\nmax_y"<<griglia.ottieni_max_y()<<'\n';
              esporta_griglia_csv("griglia.csv", griglia);
  auto percorsi = trova_percorsi_con_algoritmo_completo(
      griglia.ottieni_partenza(), griglia.ottieni_arrivo(), griglia);

  stampa_statistiche_percorsi(percorsi, griglia);



  if (!percorsi.empty()) {
    esporta_percorso_omap(nome_file, "mappa_con_percorso.omap", percorsi,
                          ids_simboli, griglia);
  }

  return percorsi;
}

// tutto il main ma non nel main(per test su file omap)
/*std::vector<Percorso> aiuto_main(const std::string &nome_file) {

  std::vector<int> vietati{206, 301, 307, 411, 515, 518,
                           520, 521, 708, 709, 714}; // simboli generali vietati
  std::vector<int> pieni{206, 301, 307, 411,
                         520, 521, 709, 714}; // simboli generali pieni
  int id_arrivo{0};
  int id_partenza{0};
  bool dentro_memoria = false;
  std::unordered_set<int> id_vietati{};
  std::unordered_set<int> id_pieni{};
  std::vector<Associazione_id_simbolo> ids_simboli{};
  Punto partenza_mondo;
  Punto arrivo_mondo;
  bool partenza_trovata = false;
  bool arrivo_trovato = false;
  std::string linea;
  std::ifstream file(nome_file);
  if (!file.is_open()) {
    std::cout << "File non trovato\n";
    return {};
  }
  double fattore_scala = calcola_fattore_scala();
  std::vector<oggetto_poligonale_vietato> oggetti_vietati{};
  while (std::getline(file, linea)) {
    if (linea.find("id=") != std::string::npos &&
        linea.find("<symbol") != std::string::npos) {
      Associazione_id_simbolo coppia = ids_a_simboli(linea);
      if (coppia.id != -1 && coppia.simbolo != -1) {
        ids_simboli.push_back(coppia);
      }
    }
  }
  assert(!ids_simboli.empty()); // almeno un simbolo

  chiedi_verde3_oltrepassabile(vietati, pieni);
  std::vector<int> vettore_ids_vietati =
      trova_associazione_per_id_vietati_o_pieni(ids_simboli, vietati);
  std::vector<int> vettore_ids_pieni =
      trova_associazione_per_id_vietati_o_pieni(ids_simboli, pieni);
  id_vietati = {vettore_ids_vietati.begin(), vettore_ids_vietati.end()};
  id_pieni = {vettore_ids_pieni.begin(), vettore_ids_pieni.end()};
  for (const auto &coppia : ids_simboli) {
    if (coppia.simbolo == 706) {
      id_arrivo = coppia.id;
    } else if (coppia.simbolo == 701) {
      id_partenza = coppia.id;
    }
  }
  // ho trovato tutte associazioni id-simbolo, rileggo il file cercando gli
  // oggetti

  file.clear();
  file.seekg(0);
  while (std::getline(
      file, linea)) { // serve per non leggere come oggetti quelli degli undo
    if ((linea.find("<undo>") != std::string::npos) ||
        (linea.find("<redo>") != std::string::npos)) {
      dentro_memoria = true;
      continue;
    }
    if ((linea.find("</undo>") != std::string::npos) ||
        (linea.find("</redo>") != std::string::npos)) {
      dentro_memoria = false;
      continue;
    }
    if (dentro_memoria) {
      continue;
    }
    if (linea.find("<object") != std::string::npos) {
      std::string blocco = linea;

      // Se l'oggetto non è tutto su una riga, accumula
      while (blocco.find("</object>") == std::string::npos &&
             blocco.find("/>") == std::string::npos) {
        std::string riga_extra;
        if (!std::getline(file, riga_extra)) {
          break;
        }
        blocco += " " + riga_extra;
      }
      int id = trova_simbolo(blocco);
      if (id == id_partenza) {
        std::vector<Punto> coordinate =
            estrai_coordinate(blocco, fattore_scala);
        if (!coordinate.empty()) {
          partenza_mondo = coordinate[0];
          partenza_trovata = true;
        }
      }
      if (id == id_arrivo) {
        std::vector<Punto> coordinate =
            estrai_coordinate(blocco, fattore_scala);
        if (!coordinate.empty()) {
          arrivo_mondo = coordinate[0];
          arrivo_trovato = true;
        }
      }
      if (id != -1 && id_vietati.contains(id)) {
        std::vector<Punto> coordinate =
            estrai_coordinate(blocco, fattore_scala);
        bool pienezza = (id_pieni.contains(id));
        if (!coordinate.empty()) {
          oggetti_vietati.push_back({id, coordinate, pienezza});
        }
      }
    }
  }
  double min_x = 0, max_x = 0, min_y = 0, max_y = 0;
  calcola_dimensioni_max_min(oggetti_vietati,
                             arrivo_mondo, partenza_mondo);
  int risoluzione = 800; // risoluzione, buona via di mezzo, si potrebbe mettere
                         // in funzione scala-x_min, x_max
  double larghezza_mondo = max_x - min_x;
  double altezza_mondo = max_y - min_y;

  int larghezza_griglia = risoluzione;
  int altezza_griglia =
      static_cast<int>(risoluzione * (altezza_mondo / larghezza_mondo));
  if (altezza_griglia < 1) {
    altezza_griglia = risoluzione;
  }
  assert(larghezza_griglia > 0);
  assert(altezza_griglia > 0);

  std::vector<std::vector<TipoCella>> griglia(
      static_cast<size_t>(altezza_griglia),
      std::vector<TipoCella>(static_cast<size_t>(larghezza_griglia),
                             TipoCella::oltrepassabile));
  for (const auto &oggetto_vietato : oggetti_vietati) {
    oggetti_a_griglia(oggetto_vietato,
                      griglia); // prima gli oggetti
    //std::cout << "  Rasterizzato ID=" << ogg.id << " (" << ogg.contorno.size()
    //          << " punti, " << (ogg.e_pieno ? "pieno" : "linea") << ")\n";
  }
  dilata_ostacoli(griglia);
  int partenza_gx = 0, partenza_gy = 0;
  int arrivo_gx = 0, arrivo_gy = 0;
  if (partenza_trovata) {
    mondo_a_griglia(partenza_mondo.x, partenza_mondo.y, partenza_gx,
partenza_gy, griglia);
  }

  if (arrivo_trovato) {
    mondo_a_griglia(arrivo_mondo.x, arrivo_mondo.y,
                     griglia);
  }

  assert(partenza_trovata);
  assert(arrivo_trovato);

  Punto partenza(partenza_gx, partenza_gy);
  Punto arrivo(arrivo_gx, arrivo_gy);
  // Griglia rendi_griglia(griglia);

  std::vector<Percorso> percorsi =
      trova_percorsi_con_algoritmo_completo(partenza, arrivo, griglia);

  //std::cout << partenza_trovata << "\n";
  //std::cout << "coordinate partenza" << partenza_gx << "," << partenza_gy
  //          << "\n coordinate arrivo" << arrivo_gx << "," << arrivo_gy <<
  //"\n";
  std::cout << "Trovati " << percorsi.size()
            << " percorsi entro +20% del migliore:\n\n";
  std::cout << "Percorsi di lunghezza:\n";
  for (size_t k = 0; k != percorsi.size(); k++) {
    std::cout << costo_percorso_geometrico_mondo(percorsi[k].punti, griglia)
              << "m\n";
  }
  std::ofstream csv("griglia.csv");
  if (csv.is_open()) {
    csv << altezza_griglia << "," << larghezza_griglia << "\n";
    for (int y = 0; y != altezza_griglia; ++y) {
      for (int x = 0; x != larghezza_griglia; ++x) {
        if (partenza_trovata && x == partenza_gx && y == partenza_gy) {
          csv << "S";
        } else if (arrivo_trovato && x == arrivo_gx && y == arrivo_gy) {
          csv << "A";
        } else {
          csv << (griglia[static_cast<size_t>(y)][static_cast<size_t>(x)] ==
                          TipoCella::non_oltrepassabile
                      ? "0"
                      : "1");
        }
        if (x < larghezza_griglia - 1) {
          csv << ",";
        }
      }
      csv << "\n";
    }

    csv.close();
  } else {
    std::cout << "Problemi nella creazione del file csv \n";
  }
  if (!percorsi.empty()) {
    esporta_percorso_omap(nome_file, "mappa_con_percorso.omap", percorsi,
                          ids_simboli, griglia);
  }
  return percorsi;
}*/
} // namespace pf